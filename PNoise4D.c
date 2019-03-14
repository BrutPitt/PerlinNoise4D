////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2019 Michele Morrone
//  All rights reserved.
//
//  mailto:me@michelemorrone.eu
//  mailto:brutpitt@gmail.com
//  
//  https://github.com/BrutPitt
//
//  https://michelemorrone.eu
//  https://BrutPitt.com
//
//  This software is distributed under the terms of the BSD 2-Clause license:
//  
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//      * Redistributions of source code must retain the above copyright
//        notice, this list of conditions and the following disclaimer.
//      * Redistributions in binary form must reproduce the above copyright
//        notice, this list of conditions and the following disclaimer in the
//        documentation and/or other materials provided with the distribution.
//   
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
//  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
//  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
//  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
//  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF 
//  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
////////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include <gl/glut.h>
#include <gl/glext.h>

#ifdef MULTITHREAD_ON
#include <omp.h>
#endif

#include "defs.h"

#define NOISE_WRAP_INDEX	128
#define NOISE_MOD_MASK		127

// A large power of 2, we'll go for 4096, to add to negative numbers
// in order to make them positive

#define NOISE_LARGE_PWR2	4096

#define easeCurve(t)		( t * t * (3.0 - (t + t)) )
#define linearInterp(t, a, b)	( a + t * (b - a) )
#define dot2(rx, ry, q)		( rx * q[0] + ry * q[1] )
#define dot3(rx, ry, rz, q)     ( rx * *(q) + ry * *(q+1) + rz * *(q+2) )
#define dot4(rx, ry, rz, rt, q)     ( rx * *(q) + ry * *(q+1) + rz * *(q+2) + rt * *(q+3))

#define setupValues(pos, g0, g1, d0, d1) { \
const float t = *(pos) + NOISE_LARGE_PWR2; \
            g0 = ((int)t) & NOISE_MOD_MASK; \
            g1 = (g0 + 1) & NOISE_MOD_MASK; \
            d0 = ((float)t) - (int)t; \
            d1 = d0 - 1.0; \
} \

#ifndef FALSE
    #define FALSE 0
#endif

#ifndef TRUE
    #define TRUE 1
#endif

static float gradientTable4d[NOISE_WRAP_INDEX*2 + 2][4] = { 0 };
static unsigned permutationTable[NOISE_WRAP_INDEX*2 + 2];	// permutation table
static int initialized=FALSE;

/////////////////////////////////////////////////////////////////////
// return a random float in [-1,1]

float randNoiseFloat() { 
  return (float)((rand() % (NOISE_WRAP_INDEX + NOISE_WRAP_INDEX)) - 
                 NOISE_WRAP_INDEX) / NOISE_WRAP_INDEX;
};


void normalize4d(float *vector) {
  float length = 1/sqrt((vector[0] * vector[0]) + 
                        (vector[1] * vector[1]) +
                        (vector[2] * vector[2]) +
                        (vector[3] * vector[3]));
  *vector++ *= length;  
  *vector++ *= length;
  *vector++ *= length;
  *vector   *= length;
}

/////////////////////////////////////////////////////////////////////
// initialize everything during constructor or reseed -- note
// that space was already allocated for the gradientTable
// during the constructor

void generateLookupTables() {
  unsigned i, j, temp;

  for (i=0; i<NOISE_WRAP_INDEX; i++) {
    permutationTable[i] = i;
//4DGradient for all Dimension (1d, 2d, 3d 4d)

    for (j=0; j<4; j++) { gradientTable4d[i][j] = randNoiseFloat(); }
    normalize4d(gradientTable4d[i]);
  }

  // Shuffle permutation table up to NOISE_WRAP_INDEX
  for (i=0; i<NOISE_WRAP_INDEX; i++) {
    j = rand() & NOISE_MOD_MASK;
    temp = permutationTable[i];
    permutationTable[i] = permutationTable[j];
    permutationTable[j] = temp;
  }

  // Add the rest of the table entries in, duplicating 
  // indices and entries so that they can effectively be indexed
  // by unsigned chars.
  //
  for (i=0; i<NOISE_WRAP_INDEX+2; i++) {
    permutationTable[NOISE_WRAP_INDEX + i] = permutationTable[i];

//4DGradient for all Dimension (1d, 2d, 3d 4d)
    for (j=0; j<4; j++) 
      gradientTable4d[NOISE_WRAP_INDEX + i][j] = gradientTable4d[i][j]; 
  }

  // And we're done. Set initialized to true
  initialized = TRUE;
}

/////////////////////////////////////////////////////////////////////
// reinitialize with new, random values.

void reseed() {
  srand((unsigned int) (time(NULL) + rand()));
  generateLookupTables();
}

/////////////////////////////////////////////////////////////////////
// reinitialize using a user-specified random seed.

void reseedVal(unsigned int rSeed) {
  srand(rSeed);
  generateLookupTables();
}

float noise4d(float *pos) {
  int gridPointL, gridPointR, gridPointD, gridPointU, gridPointB, gridPointF, gridPointV, gridPointW;
  float distFromL, distFromR, distFromD, distFromU, distFromB, distFromF, distFromV, distFromW;
  float sX, sY, sZ, sT, a, b, c, d, e, f, t, u, v;

 
  // find out neighboring grid points to pos and signed distances from pos to them.
  setupValues(pos  , gridPointL, gridPointR, distFromL, distFromR);
  setupValues(pos+1, gridPointD, gridPointU, distFromD, distFromU);
  setupValues(pos+2, gridPointB, gridPointF, distFromB, distFromF);
  setupValues(pos+3, gridPointV, gridPointW, distFromV, distFromW);

  {
  
    const int indexL = permutationTable[ gridPointL ];
    const int indexR = permutationTable[ gridPointR ];

    const int indexLD = permutationTable[ indexL + gridPointD ];
    const int indexRD = permutationTable[ indexR + gridPointD ];
    const int indexLU = permutationTable[ indexL + gridPointU ];
    const int indexRU = permutationTable[ indexR + gridPointU ];

    const int indexLDB = permutationTable[ indexLD + gridPointB ];
    const int indexRDB = permutationTable[ indexRD + gridPointB ];
    const int indexLUB = permutationTable[ indexLU + gridPointB ];
    const int indexRUB = permutationTable[ indexRU + gridPointB ];
    const int indexLDF = permutationTable[ indexLD + gridPointF ];
    const int indexRDF = permutationTable[ indexRD + gridPointF ];
    const int indexLUF = permutationTable[ indexLU + gridPointF ];
    const int indexRUF = permutationTable[ indexRU + gridPointF ];

    float *ptrV = (float *) &gradientTable4d[gridPointV];
    float *ptrW = (float *) &gradientTable4d[gridPointW];

    sX = easeCurve(distFromL);
    sY = easeCurve(distFromD);
    sZ = easeCurve(distFromB);
    sT = easeCurve(distFromV);


    u = dot4(distFromL, distFromD, distFromB, distFromV, ptrV+indexLDB);
    v = dot4(distFromR, distFromD, distFromB, distFromV, ptrV+indexRDB);
    a = linearInterp(sX, u, v);                                     
                                                                
    u = dot4(distFromL, distFromU, distFromB, distFromV, ptrV+indexLUB);
    v = dot4(distFromR, distFromU, distFromB, distFromV, ptrV+indexRUB);
    b = linearInterp(sX, u, v);               
    c = linearInterp(sY, a, b);               
                                        
    u = dot4(distFromL, distFromD, distFromF, distFromV, ptrV+indexLDF);
    v = dot4(distFromR, distFromD, distFromF, distFromV, ptrV+indexRDF);
    a = linearInterp(sX, u, v);                                     
                                                                
    u = dot4(distFromL, distFromU, distFromF, distFromV, ptrV+indexLUF);
    v = dot4(distFromR, distFromU, distFromF, distFromV, ptrV+indexRUF);
    b = linearInterp(sX, u, v);
    d = linearInterp(sY, a, b);
    e = linearInterp(sZ, c, d);


    u = dot4(distFromL, distFromD, distFromB, distFromW, ptrW+indexLDB);
    v = dot4(distFromR, distFromD, distFromB, distFromW, ptrW+indexRDB);
    a = linearInterp(sX, u, v);                                      
                                                    
    u = dot4(distFromL, distFromU, distFromB, distFromW, ptrW+indexLUB);
    v = dot4(distFromR, distFromU, distFromB, distFromW, ptrW+indexRUB);
    b = linearInterp(sX, u, v);               
    c = linearInterp(sY, a, b);               
                                        
    u = dot4(distFromL, distFromD, distFromF, distFromW, ptrW+indexLDF);
    v = dot4(distFromR, distFromD, distFromF, distFromW, ptrW+indexRDF);
    a = linearInterp(sX, u, v);                                      
                                                    
    u = dot4(distFromL, distFromU, distFromF, distFromW, ptrW+indexLUF);
    v = dot4(distFromR, distFromU, distFromF, distFromW, ptrW+indexRUF);
    b = linearInterp(sX, u, v);
    d = linearInterp(sY, a, b);
    f = linearInterp(sZ, c, d);
  }

  return linearInterp(sT, e, f);
}

static float tScale=12000.f;
static float xyzScale=1.5f;

float noise(float x, float y, float z, float w) { 
//static float tScale=1500.f;
//static float xyzScale=15.f;
  float p[4] = { x*xyzScale, y*xyzScale, z*xyzScale, w/tScale};
  return noise4d(p);
}

static float noisePersistance=0.5;
static int noiseMaxOctaves=7;

float fabsnoise(float x, float y, float z, float t) 
{
  int i;
  float amplitude = 1.f;
  float result = 0.0;
  x*=xyzScale;
  y*=xyzScale;
  z*=xyzScale;
  t/=tScale;
  for (i=0; i<noiseMaxOctaves; i++) {
      float p[4] = { x, y, z, t };
    result += fabs(noise4d(p)) * amplitude;
    amplitude *= noisePersistance;
    x *= 2.0; y *= 2.0; z *= 2.0; t *= 2.0;
  }
  return result;
}

#define CLOUDS
#define GAP256(v) (v)>255 ? 255 : ((v)<0 ? 0 : (v))



extern is_multiThread;


void build3Dtex(int Dim, float w, unsigned int *tex3ddata, unsigned int *pal)
{
float x,y,z;
int ix, iy, iz;

//int a;


//int proc = omp_get_num_procs();
#ifdef MULTITHREAD_ON
    const int numThreads = omp_get_max_threads();
    omp_set_num_threads(numThreads);
#endif




#pragma omp parallel default(shared) private(iz, z)
{                
        #pragma omp for schedule(dynamic) nowait
        for (iz=0; iz<Dim; iz++) {
            z=(float)iz/(float)Dim;

#pragma omp parallel private(iy, y) shared(z, iz, Dim, tex3ddata, pal, w)
{                
            #pragma omp for schedule(dynamic) nowait
	        for(iy=0; iy<Dim; iy++) {
                y=(float)iy/(float)Dim;

#pragma omp parallel private(ix) shared(iz, z, iy, y, Dim, tex3ddata, pal, w)
{                
                #pragma omp for schedule(dynamic) nowait
		        for(ix=0; ix<Dim; ix++) {
                    //int val=noise((float)x/tex.x,(float)y/tex.y,(float)z/tex.z,nise)*512.0;
                    //val=GAP256(val);
#ifdef CLOUDS         
                    
                    const float f=fabsnoise((float)ix/(float)Dim,y,z,w);
                    int val= 127+((2.0*f-.55)*128.f);
                    val=GAP256(val);                    
                    tex3ddata[(iz<<(BITS<<1))+(iy<<BITS)+ix] = *(pal+val);
#else
                    tex3ddata[(iz<<(BITS<<1))+(iy<<BITS)+ix] = *((DWORD*)pal+calc_l(PosX+StartX+(EndX-StartX)*x/Dim,
                                                                          PosY+StartY+(EndY-StartY)*y/Dim,
                                                                          PosZ+StartZ+(EndZ-StartZ)*z/Dim,w));  
#endif
                    //*((DWORD*)pal+*ptrB++);
                    

		        }
}
	        }
}
        }

}

}