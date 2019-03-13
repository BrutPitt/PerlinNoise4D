#define CLOUDS
#define GLH_EXT_SINGLE_FILE

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

#include <GL/glew.h>

#include <gl/glut.h>
#include <gl/glext.h>
#include "defs.h"

#define GAP(X) ((X)>255 ? 255 : (X)<0 ? 0 : (X))

/* display list */
#define Z_FRONT 1
#define Z_BACK  2
#define Y_FRONT 3
#define Y_BACK  4
#define X_FRONT 5
#define X_BACK  6

/* palette */
#define PALETTE_SIZE    32
#define PALETTE_XOFFSET  5
#define PALETTE_YOFFSET  5

GLuint pboIds[2];
int index = 0;

/* mouse mode */
enum { ROTATION, COLOR_SELECTION, CLIPPLANE };

float noise(float x, float y, float z, float w);
float fabsnoise(float x, float y, float z, float w);


GLubyte *tex3ddata;
GLuint clouds_tex;

enum {X, Y, Z, W};
enum {R, G, B, A};
enum {OVER, ATTENUATE, NONE, LASTOP};
enum {OBJ_ANGLE, SLICES, CUTTING};


int FTcount=0;
/* globals */
int   xs=512,ys=512;
/* BLEND and DEPTH_TEST */
int   alpha=1,depth=0;
int   drawCube=1,projection=1;
int   s1,s2,s3;
/* rotation & mouse */
float scaleFactor=1.;
int   oldx=0,oldy=0,xrot=0,yrot=0;
int   inverseZ=1;
int   mode=ROTATION;
int   color_1,color_2;
/* texture */
int dimX, dimY, dimZ;
int    px,py,pz;
GLuint *XTextureId;
GLuint *YTextureId;
GLuint *ZTextureId;
unsigned char *tex_buf;
/* palette */
GLuint checkerId;
unsigned char *pal;
unsigned char *pal_copy;
int is_multiThread = 1;


/* profiling */
int frame=0;
int oldtime=0;
static int nise=0;

typedef unsigned long DWORD;
typedef unsigned char BYTE;


int inPause=0;

float FilterAny=1.0f;

int nextPower(int t)
{
	int i = 0;
    while((1<<i++)<t);

    return (1<<(i-1));
}


/* load a nice default palette */
void load_default_palette(unsigned char *pal){
	int i,j;

	for(i=0;i<128;i++){
		pal[4*i+0]=i*2;
		pal[4*i+1]=128+i;
		pal[4*i+2]=255;
		pal[4*i+3]=i/2;//16;
    }
	for(j=255;i<256;i++,j--){
		pal[4*i+0]=j;
		pal[4*i+1]=j;
		pal[4*i+2]=j;
		pal[4*i+3]=i/2;//16;
    }
}
float Quad=-1;

/* make the palette from c1 to c2 transparent */
void zeroAlpha(unsigned char *c,int c1,int c2){
	int i;
	if (c1>c2){
		i=c1;
		c1=c2;
		c2=i;
	}
	for(i=c1;i<=c2;i++){
		c[i*4+3]=0;
	}
}

/* reset the palette from c1 to c2 */
void resetAlpha(unsigned char *c,int c1,int c2){
	int i;
	if (c1>c2){
		i=c1;
		c1=c2;
		c2=i;
	}
	for(i=c1;i<=c2;i++){
		c[i*4+3]=pal_copy[i*4+3];
	}
}

void init_palette(){
	/* try to load the palette */
	pal=(unsigned char*)malloc(256*4);
	pal_copy=(unsigned char *)malloc(256*4);
    load_default_palette(pal);
	memcpy(pal_copy,pal,4*256);
}

void init_tex(){
	unsigned char *checker_buf;
	int i,j,k;
	float x,y,z;
    unsigned char *ptr=tex_buf;
    char fname[128];
    unsigned char *ptrTX;

	init_palette();
	/* load it */
	//	if (load_raw(&tex,name,128,128,62)){
    dimX = dimY = dimZ = PRJ_DIM;

 	px=nextPower(dimX);
	py=nextPower(dimY);
	pz=nextPower(dimZ);
	printf("done (%dx%dx%d)->(%dx%dx%d)\n",dimX,dimY,dimZ,px,py,pz);

   	glGenTextures(1,&checkerId);
    glGenTextures(1, &clouds_tex);

    tex3ddata=(GLubyte *)calloc(px*py*pz*4,1);
    memset(tex3ddata,0, px*py*pz*4);


#ifdef TEXT3D
    {
        DWORD *ptrD=(DWORD *) tex3ddata;
        int slicesZ=pz;
        int slicesX=px;
        int slicesY=py;

        glEnable(GL_TEXTURE_3D);


        glGenTextures(1,&clouds_tex);

   		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        glBindTexture(GL_TEXTURE_3D, clouds_tex);

	    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP);
	    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, px,py,pz,0,GL_RGBA,GL_UNSIGNED_BYTE,tex3ddata);

        	glNewList(Z_FRONT,GL_COMPILE);
            glBegin(GL_QUADS);
   		    glBindTexture(GL_TEXTURE_3D,clouds_tex);
            for(z=0;z<slicesZ;z++) {
                // front
                glTexCoord3f(0.0, 0.0, z/slicesZ); glVertex3f(-1.0, -1.0, -1.f + 2*z/(slicesZ - 1.f));
                glTexCoord3f(1.0, 0.0, z/slicesZ); glVertex3f( 1.0, -1.0, -1.f + 2*z/(slicesZ - 1.f));
                glTexCoord3f(1.0, 1.0, z/slicesZ); glVertex3f( 1.0,  1.0, -1.f + 2*z/(slicesZ - 1.f));
                glTexCoord3f(0.0, 1.0, z/slicesZ); glVertex3f(-1.0,  1.0, -1.f + 2*z/(slicesZ - 1.f));
            }
    		glEnd();
        	glEndList();

        	glNewList(Z_BACK,GL_COMPILE);
            glBegin(GL_QUADS);
   		    glBindTexture(GL_TEXTURE_3D,clouds_tex);
            for(z=0;z<slicesZ;z++) {
                 //back
                glTexCoord3f(0.0, 0.0, 1.0 - z/slicesZ); glVertex3f(-1.0, -1.0,  1.0 - 2*z/(slicesZ - 1.f));
                glTexCoord3f(1.0, 0.0, 1.0 - z/slicesZ); glVertex3f( 1.0, -1.0,  1.0 - 2*z/(slicesZ - 1.f));
                glTexCoord3f(1.0, 1.0, 1.0 - z/slicesZ); glVertex3f( 1.0,  1.0,  1.0 - 2*z/(slicesZ - 1.f));
                glTexCoord3f(0.0, 1.0, 1.0 - z/slicesZ); glVertex3f(-1.0,  1.0,  1.0 - 2*z/(slicesZ - 1.f));
            }
    		glEnd();
        	glEndList();

        	glNewList(X_FRONT,GL_COMPILE);
            glBegin(GL_QUADS);
   		    glBindTexture(GL_TEXTURE_3D,clouds_tex);
            for(x=0;x<slicesX;x++) {
                 //left
                glTexCoord3f(x/slicesX, 0.0, 0.0); glVertex3f(-1.f + 2*x/(slicesX - 1.f), -1.0, -1.0);
                glTexCoord3f(x/slicesX, 0.0, 1.0); glVertex3f(-1.f + 2*x/(slicesX - 1.f), -1.0,  1.0);
                glTexCoord3f(x/slicesX, 1.0, 1.0); glVertex3f(-1.f + 2*x/(slicesX - 1.f),  1.0,  1.0);
                glTexCoord3f(x/slicesX, 1.0, 0.0); glVertex3f(-1.f + 2*x/(slicesX - 1.f),  1.0, -1.0);
            }
       		glEnd();
        	glEndList();

        	glNewList(X_BACK,GL_COMPILE);
            glBegin(GL_QUADS);
   		    glBindTexture(GL_TEXTURE_3D,clouds_tex);
            for(x=0;x<slicesX;x++) {
                 //right
                glTexCoord3f(1.0 - x/slicesX, 1.0, 0.0); glVertex3f(1.0 - 2*x/(slicesX - 1.f),  1.0, -1.0);
                glTexCoord3f(1.0 - x/slicesX, 1.0, 1.0); glVertex3f(1.0 - 2*x/(slicesX - 1.f),  1.0,  1.0);
                glTexCoord3f(1.0 - x/slicesX, 0.0, 1.0); glVertex3f(1.0 - 2*x/(slicesX - 1.f), -1.0,  1.0);
                glTexCoord3f(1.0 - x/slicesX, 0.0, 0.0); glVertex3f(1.0 - 2*x/(slicesX - 1.f), -1.0, -1.0);
            }
       		glEnd();
        	glEndList();
                  
        	glNewList(Y_FRONT,GL_COMPILE);
            glBegin(GL_QUADS);
   		    glBindTexture(GL_TEXTURE_3D,clouds_tex);
            for(y=0;y<slicesY;y++) {
                 //bottom
                glTexCoord3f(0.0, y/slicesY, 0.0); glVertex3f(-1.0, -1.f + 2*y/(slicesY - 1.f), -1.0);
                glTexCoord3f(1.0, y/slicesY, 0.0); glVertex3f( 1.0, -1.f + 2*y/(slicesY - 1.f), -1.0);
                glTexCoord3f(1.0, y/slicesY, 1.0); glVertex3f( 1.0, -1.f + 2*y/(slicesY - 1.f),  1.0);
                glTexCoord3f(0.0, y/slicesY, 1.0); glVertex3f(-1.0, -1.f + 2*y/(slicesY - 1.f),  1.0);
            }
       		glEnd();
        	glEndList();

        	glNewList(Y_BACK,GL_COMPILE);
            glBegin(GL_QUADS);
   		    glBindTexture(GL_TEXTURE_3D,clouds_tex);
            for(y=0;y<slicesY;y++) {
                // top
                glTexCoord3f(0.0, 1.0 - y/slicesY, 0.0); glVertex3f(-1.0,  1.0 - 2*y/(slicesY - 1.f), -1.0);
                glTexCoord3f(1.0, 1.0 - y/slicesY, 0.0); glVertex3f( 1.0,  1.0 - 2*y/(slicesY - 1.f), -1.0);
                glTexCoord3f(1.0, 1.0 - y/slicesY, 1.0); glVertex3f( 1.0,  1.0 - 2*y/(slicesY - 1.f),  1.0);
                glTexCoord3f(0.0, 1.0 - y/slicesY, 1.0); glVertex3f(-1.0,  1.0 - 2*y/(slicesY - 1.f),  1.0);
            }
       		glEnd();
        	glEndList();


        glDisable(GL_TEXTURE_3D);

        //free(tex.texture);

        glGenBuffersARB(2, pboIds);
        glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pboIds[0]);
        glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, PRJ_DIM*PRJ_DIM*PRJ_DIM*4, 0, GL_STREAM_DRAW_ARB);
        glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pboIds[1]);
        glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, PRJ_DIM*PRJ_DIM*PRJ_DIM*4, 0, GL_STREAM_DRAW_ARB);
        glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);



    }

#else // TEXT2D
    glEnable(GL_TEXTURE_2D);

	/* ask for some textures */
	XTextureId=(GLuint *)malloc(sizeof(GLuint)*dimX);
	YTextureId=(GLuint *)malloc(sizeof(GLuint)*dimY);
	ZTextureId=(GLuint *)malloc(sizeof(GLuint)*dimZ);

    if (XTextureId==NULL || YTextureId==NULL || ZTextureId==NULL){
		fprintf(stderr," Out of memory\n");
		exit(-1);
	}
	/* generate textures */
	glGenTextures(dimX,XTextureId);
	glGenTextures(dimY,YTextureId);
	glGenTextures(dimZ,ZTextureId);

	/* create slices */
	/* xy slices */

	/* xy-->z */
	for (k=0;k<dimZ;k+=1){
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glBindTexture(GL_TEXTURE_2D,ZTextureId[k]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,GL_LINEAR);
        
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, px,py,0,GL_RGBA,GL_UNSIGNED_BYTE,tex3ddata);
	}

	/* yz slices */
	/* allocate a new texture buffer */

	/* zy-->x */
	for (i=0;i<dimX;i+=1){
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glBindTexture(GL_TEXTURE_2D,XTextureId[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,GL_LINEAR);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, pz,py,0,GL_RGBA,GL_UNSIGNED_BYTE,tex3ddata);

    }

	/* xz slices */
	/* allocate a new texture buffer */

	/* xz-->y */

	for (j=0;j<dimY;j+=1){
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glBindTexture(GL_TEXTURE_2D,YTextureId[j]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,GL_LINEAR);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, px,pz,0,GL_RGBA,GL_UNSIGNED_BYTE,tex3ddata);
	}

	/* display lists */

	glNewList(Z_BACK,GL_COMPILE);
	for(k=pz-dimZ;k<pz;k++){
		z=1.-2.*k/pz;
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D,ZTextureId[pz-k-1]);
		glBegin(GL_QUADS);
		glTexCoord2f(0,1);
		glVertex3f(-1,1,z);
		glTexCoord2f(1,1);
		glVertex3f(1,1,z);
		glTexCoord2f(1,0);
		glVertex3f(1,-1,z);
		glTexCoord2f(0,0);
		glVertex3f(-1,-1,z);
		glEnd();
	}
	glEndList();

	glNewList(Z_FRONT,GL_COMPILE);
	for(k=pz;k>(pz-dimZ);k--){
		z=1.-2.*k/pz;
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D,ZTextureId[pz-k]);
		glBegin(GL_QUADS);
		glTexCoord2f(0,1);
		glVertex3f(-1,1,z);
		glTexCoord2f(1,1);
		glVertex3f(1,1,z);
		glTexCoord2f(1,0);
		glVertex3f(1,-1,z);
		glTexCoord2f(0,0);
		glVertex3f(-1,-1,z);
		glEnd();
	}
	glEndList();

	glNewList(X_BACK,GL_COMPILE);
	for(i=px;i>(px-dimX);i--){
		x=-1.+2.*i/px;
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D,XTextureId[dimX-px+i-1]);
		glBegin(GL_QUADS);
		glTexCoord2f(0,0);
		glVertex3f(x,-1,-1);
		glTexCoord2f(0,1);
		glVertex3f(x,1,-1);
		glTexCoord2f(1,1);
		glVertex3f(x,1,1);
		glTexCoord2f(1,0);
		glVertex3f(x,-1,1);
		glEnd();
	}
	glEndList();

	glNewList(X_FRONT,GL_COMPILE);
	for(i=px;i>(px-dimX);i--){
		x=1.-2.*i/px;
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D,XTextureId[px-i]);
		glBegin(GL_QUADS);
		glTexCoord2f(0,0);
		glVertex3f(x,-1,-1);
		glTexCoord2f(0,1);
		glVertex3f(x,1,-1);
		glTexCoord2f(1,1);
		glVertex3f(x,1,1);
		glTexCoord2f(1,0);
		glVertex3f(x,-1,1);
		glEnd();
	}
	glEndList();

	glNewList(Y_BACK,GL_COMPILE);
	for(j=py;j>(py-dimY);j--){
		y=-1.+2.*j/py;
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D,YTextureId[dimY-py+j-1]);
		glBegin(GL_QUADS);
		glTexCoord2f(0,0);
		glVertex3f(-1,y,-1);
		glTexCoord2f(0,1);
		glVertex3f(-1,y,1);
		glTexCoord2f(1,1);
		glVertex3f(1,y,1);
		glTexCoord2f(1,0);
		glVertex3f(1,y,-1);
		glEnd();
	}
	glEndList();

	glNewList(Y_FRONT,GL_COMPILE);
	for(j=py;j>(py-dimY);j--){
		y=1.-2.*j/py;
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D,YTextureId[py-j]);
		glBegin(GL_QUADS);
		glTexCoord2f(0,0);
		glVertex3f(-1,y,-1);
		glTexCoord2f(0,1);
		glVertex3f(-1,y,1);
		glTexCoord2f(1,1);
		glVertex3f(1,y,1);
		glTexCoord2f(1,0);
		glVertex3f(1,y,-1);
		glEnd();
	}
	glEndList();
    glDisable(GL_TEXTURE_2D);

#endif

    glEnable(GL_TEXTURE_2D);

	/* a nice checker texture for the palette's background */
	checker_buf=(unsigned char *)malloc(4*PALETTE_SIZE*256);
	if (checker_buf==NULL){
		fprintf(stderr,"load_tex() : Out of memory\n");
		exit(-1);
	}
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glBindTexture(GL_TEXTURE_2D,checkerId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	for(i=0;i<PALETTE_SIZE;i++){
		for(j=0;j<256;j++){
			int a,b,c,step;
			step=8;
			a=!((i/step)%2*255);
			b=((j/step)%2*255);
			c=a?(b?255:0):
			(b?0:255);
			checker_buf[(i+j*PALETTE_SIZE)*4+0]=c;
			checker_buf[(i+j*PALETTE_SIZE)*4+1]=c;
			checker_buf[(i+j*PALETTE_SIZE)*4+2]=c;
			checker_buf[(i+j*PALETTE_SIZE)*4+3]=(!c)*255;
		}
	}
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,PALETTE_SIZE,256,0,GL_RGBA,GL_UNSIGNED_BYTE,checker_buf);
	free(checker_buf);

    glDisable(GL_TEXTURE_2D);


}

void build3Dtex(int Dim, float w, unsigned int *tex3ddata, unsigned int *pal);

/* my nvidia tnt2 doesn't have the paletted texture extension */
void update_tex(){
	int i,j,k;
	float x,y,z;
    DWORD *ptr;
    int mulK,mulX;
    int offset;



    if(!inPause) { 
        Quad+=0.005;
    }

#ifdef TEXT3D
    {

        GLubyte* ptr;

        glEnable(GL_TEXTURE_3D);
        glBindTexture(GL_TEXTURE_3D, clouds_tex);
        glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pboIds[index]);

        glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, PRJ_DIM, PRJ_DIM, PRJ_DIM, GL_RGBA, GL_UNSIGNED_BYTE, 0);

        index ^= 1;

        glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pboIds[index]);

        glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, PRJ_DIM*PRJ_DIM*PRJ_DIM*4, 0, GL_STREAM_DRAW_ARB);
        ptr = (GLubyte*)glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB);        

        build3Dtex(PRJ_DIM, (float)nise, (unsigned int *)ptr, (unsigned int *)pal);

        glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB);

        glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
        glDisable(GL_TEXTURE_3D);

    }
#else
    FTcount++;
    FTcount%=PRJ_DIM;
    offset=dimX*dimY*FTcount;

    glEnable(GL_TEXTURE_2D);



	// xy-->z
	tex_buf=(unsigned char *)calloc(pz*px*py*4,1);
    build3Dtex(PRJ_DIM, (float)nise, (unsigned int *)tex_buf, (unsigned int *)pal);

	for (k=0;k<dimZ;k+=1){
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glBindTexture(GL_TEXTURE_2D,ZTextureId[k]);
        
        mulK=dimY*k;
		for(i=0;i<dimX;i++){
            ptr=tex_buf+(i<<2);
			for(j=0, mulX=dimX*mulK+i; j<dimY; j++, mulX+=dimX){
                *ptr = *((DWORD*)tex_buf+mulX);
                ptr+=px;
			}
		}
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, px,py,0,GL_RGBA,GL_UNSIGNED_BYTE,tex_buf);
	}

	// zy-->x
	for (i=0;i<dimX;i+=1){
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glBindTexture(GL_TEXTURE_2D,XTextureId[i]);

        for(k=0;k<dimZ;k++){
            ptr=(DWORD *) (tex_buf+(k<<2));            
			for(j=0, mulX=dimX*dimY*k+i;j<dimY;j++, mulX+=dimX){
                *ptr = *((DWORD*)tex_buf+mulX);
                ptr+=pz;
			}
		}
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, pz,py,0,GL_RGBA,GL_UNSIGNED_BYTE,tex_buf);
	}
	// xz-->y
	for (j=0;j<dimY;j+=1){
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glBindTexture(GL_TEXTURE_2D,YTextureId[j]);
        ptr=tex_buf;
        
		for(k=0;k<dimZ;k++){
            mulX=dimX*(j+dimY*k);
			for(i=0;i<dimX;i++){
                *ptr++ = *((DWORD*)tex_buf+i+mulX);
			}
		}
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, px,pz,0,GL_RGBA,GL_UNSIGNED_BYTE,tex_buf);
	}
	free(tex_buf);
    glDisable(GL_TEXTURE_2D);
#endif
}

/* draw the palette */
void drawPalette(unsigned char *pal){
	int i;
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0,xs,0,ys,0.001,1000);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glColor3f(0.5,0.5,0.5);
	/* background */
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D,checkerId);
	glBegin(GL_QUADS);
	glTexCoord2f(0,1);
	glVertex3f(xs-PALETTE_SIZE-PALETTE_XOFFSET,PALETTE_YOFFSET,-1);
	glTexCoord2f(1,1);
	glVertex3f(xs-PALETTE_XOFFSET,PALETTE_YOFFSET,-1);
	glTexCoord2f(1,0);
	glVertex3f(xs-PALETTE_XOFFSET,256+PALETTE_YOFFSET,-1);
	glTexCoord2f(0,0);
	glVertex3f(xs-PALETTE_SIZE-PALETTE_XOFFSET,256+PALETTE_YOFFSET,-1);
	glEnd();
	glDisable(GL_TEXTURE_2D);
	/* palette */
	for(i=0;i<256;i++){
		glColor4ub(pal[i*4+0],pal[i*4+1],pal[i*4+2],pal[i*4+3]*2.0);
		glBegin(GL_LINES);
		glVertex3f(xs-PALETTE_SIZE-PALETTE_XOFFSET,i+PALETTE_YOFFSET,-1);
		glVertex3f(xs-PALETTE_XOFFSET,i+PALETTE_YOFFSET,-1);
		glEnd();
	}
	glColor3f(0,0,0);
	glBegin(GL_LINE_LOOP);
	glVertex3f(xs-PALETTE_SIZE-PALETTE_XOFFSET-3,PALETTE_YOFFSET-3,-1);
	glVertex3f(xs-PALETTE_XOFFSET+3,PALETTE_YOFFSET-3,-1);
	glVertex3f(xs-PALETTE_XOFFSET+3,256+PALETTE_YOFFSET+3,-1);
	glVertex3f(xs-PALETTE_SIZE-PALETTE_XOFFSET-3,256+PALETTE_YOFFSET+3,-1);
	glEnd();
	/* selection */
	if (mode==COLOR_SELECTION){
		glColor3f(0,0,0);
		glBegin(GL_LINE_LOOP);
		glVertex3f(xs-PALETTE_SIZE-PALETTE_XOFFSET,color_1+PALETTE_YOFFSET,-1);
		glVertex3f(xs-PALETTE_XOFFSET,color_1+PALETTE_YOFFSET,-1);
		glVertex3f(xs-PALETTE_XOFFSET,color_2+PALETTE_YOFFSET,-1);
		glVertex3f(xs-PALETTE_SIZE-PALETTE_XOFFSET,color_2+PALETTE_YOFFSET,-1);
		glEnd();
	}
}


/* define a cutting plane */
GLdouble cutplane[] = {0.f, 0.f, 1.f, 0.f};

void draw(){
	float z;
	int k;

	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);


	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	if (projection)
		gluPerspective(40.,(float)(xs-PALETTE_SIZE)/(float)ys,1.1,1000);
	else
		glOrtho(-2,2,-2,2,0,1000);
	glMatrixMode(GL_MODELVIEW);

	glLoadIdentity();
	gluLookAt(0,0,-5,0,0,0,0,1,0);

	glRotatef(yrot,-1,0,0);
	glRotatef(xrot,0,1,0);
	glScalef(scaleFactor,(inverseZ?-1:1)*scaleFactor,scaleFactor);



#ifdef TEXT3D
    {
	    glEnable(GL_TEXTURE_3D);
	    glColor3f(1,1,1);

        if (yrot>45&&yrot<(45+90))              { glCallList(Y_BACK);   }
        else if(yrot<315  && yrot>(315-90))		{ glCallList(Y_FRONT);  }
        else if(xrot>=315 || xrot<=45)			{ glCallList(Z_BACK);   }
        else if(xrot>=135 && xrot<=225)			{ glCallList(Z_FRONT);  }
        else if(xrot>=45  && xrot<=135)			{ glCallList(X_FRONT);  }
        else            						{ glCallList(X_BACK);   }

        glDisable(GL_TEXTURE_3D);

    }
#else
	glEnable(GL_TEXTURE_2D);
	glColor3f(1,1,1);

    if (yrot>45&&yrot<(45+90))              { glCallList(Y_BACK);   }
    else if(yrot<315  && yrot>(315-90))		{ glCallList(Y_FRONT);  }
    else if(xrot>=315 || xrot<=45)			{ glCallList(Z_BACK);   }
    else if(xrot>=135 && xrot<=225)			{ glCallList(Z_FRONT);  }
    else if(xrot>=45  && xrot<=135)			{ glCallList(X_FRONT);  }
    else            						{ glCallList(X_BACK);   }

    glDisable(GL_TEXTURE_2D);


#endif


	/* cube */
	if (drawCube){
		glDisable(GL_TEXTURE_2D);
		glColor3f(0,0,0);
		glLineWidth(1);
		glutWireCube(2);
	}

	/* palette */
	drawPalette(pal);


	glutSwapBuffers();
	frame++;


}


void reshape(int w, int h){
	xs=w;
	ys=h;
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	if (projection)
		gluPerspective(40.,(float)(w-PALETTE_SIZE)/(float)h,0.1,1000);
	else
		glOrtho(-2,2,-2,2,0,1000);
	glEnable(GL_TEXTURE_2D);
}

void mouse(int b,int state,int x,int y){
	if(state == GLUT_DOWN) {
		color_1=ys-y-PALETTE_YOFFSET;
		if (x>(xs-PALETTE_SIZE-PALETTE_XOFFSET)&&x<(xs-PALETTE_XOFFSET)){
			if (color_1>=0&&color_1<256){
				mode=COLOR_SELECTION;
				color_2=color_1;
				return;
			}
		}
        mode= b==GLUT_RIGHT_BUTTON ? CLIPPLANE : ROTATION;
		oldx = x;
		oldy = y;
	}
	if (state==GLUT_UP){
		if (mode==COLOR_SELECTION){
			color_2=ys-y-PALETTE_YOFFSET;
			if (color_2<0)
				color_2=0;
			else
			    if (color_2>255)
				color_2=255;
			/* make the selection transparent */
			if (b==GLUT_LEFT_BUTTON)
				zeroAlpha(pal,color_1,color_2);
			if (b==GLUT_RIGHT_BUTTON)
				resetAlpha(pal,color_1,color_2);
            mode= b==GLUT_RIGHT_BUTTON ? CLIPPLANE : ROTATION;
			update_tex();
		}
	}
	draw();
}

void motion(int x, int y) {
int   dx,dy;
	
    if (mode==ROTATION){
		dx = x - oldx;
		dy = y - oldy;
		oldx = x;
		oldy = y;
		xrot+=dx/2;
		yrot+=dy/2;
		xrot=xrot<0?(xrot%360)+360:xrot%360;
		yrot=yrot<0?(yrot%360)+360:yrot%360;
        if(yrot>90  && yrot<180) yrot=90; 
        if(yrot<270 && yrot>180) yrot=270;
    } 

	if (mode==COLOR_SELECTION){
		color_2=ys-y-PALETTE_YOFFSET;
		if (color_2<0)
			color_2=0;
		else
		    if (color_2>255)
			color_2=255;
	}
	draw();
}

void idle(){

    {
    char title[256];
	if (clock()-oldtime>CLOCKS_PER_SEC){
		sprintf(title,"Volume rendering with gl (%3.2ffps) - x%d - y%d",1000.0*frame/(float)(clock()-oldtime),xrot,yrot);
		glutSetWindowTitle(title);
		frame=0;
		oldtime=clock();
	}
    }
    if(!inPause) {
        nise+=100;
        update_tex();
    }
    draw();
}

void keyboard(unsigned char key, int x, int y){
	unsigned char *buf;
	char fn[256];
	static int id=0;

	switch (key) {
	case 'h':
		printf(" [+|-] - zoom \n");
		printf(" [c]   - toggle cube \n");
		printf(" [p]   - toggle projection mode\n");
		printf(" [z]   - inverse z axis\n");
		printf(" [r]   - reset palette\n");
		break;
	case '+':
		scaleFactor+=0.2;
		glutPostRedisplay();
		break;
	case '-':
		scaleFactor-=0.2;
		glutPostRedisplay();
		break;
	case 'a':
		if (alpha=!alpha)
			glEnable(GL_BLEND);
		else
		    glDisable(GL_BLEND);
		glutPostRedisplay();
		printf("alpha [%s]\n",alpha?"on":"off");
		break;
	case 'd':
		if (depth=!depth)
			glEnable(GL_DEPTH_TEST);
		else
		    glDisable(GL_DEPTH_TEST);
		glutPostRedisplay();
		printf("depth [%s]\n",depth?"on":"off");
		break;
	case 'c':
		printf("cube [%s]\n",(drawCube=!drawCube)?"on":"off");
		glutPostRedisplay();
		break;
	case 'p':
		printf("projection [%s]\n",(projection=!projection)?"perspective":"orthogonal");
		reshape(xs,ys);
		glutPostRedisplay();
		break;
	case 'v':
        nise-=100;
        update_tex();
		break;
	case 'V':
        nise+=100;
        update_tex();
		break;
    case ' ':
        inPause^=1;
        break;

	case 'r':
		printf("reset palette\n");
		memcpy(pal,pal_copy,256*4);
		update_tex();
		break;
    case 'F':
        FilterAny+=1.0;
        printf("Ani %f2.1",FilterAny);
        break;
    case 'f':
        FilterAny-=1.0;
        printf("Ani %f2.1",FilterAny);
        break;
	case 13:
        is_multiThread ^= 1;
        break;
	case 27:
        if (tex3ddata) free(tex3ddata);
		exit(0);
		break;
	}
}

void reseed();

int main(int argc, char** argv){

	glutInit(&argc, argv);
	glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutInitWindowSize (512,512);
	glutInitWindowPosition (0,0);
	glutCreateWindow("Volume rendering with gl");
	glutReshapeFunc(reshape);
	glutDisplayFunc(draw);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);
	glutKeyboardFunc (keyboard);
	glutIdleFunc(idle);
	/* enable transparency */
	glEnable(GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	/* disable depth test */
	glDisable(GL_DEPTH_TEST);

    glewInit();

    reseed();
	init_tex();

	//glClearColor(1.0,.5,.0,.60);
	glClearColor(.5,.75,1.0,.60);

	glutMainLoop();
	return 0;
}

