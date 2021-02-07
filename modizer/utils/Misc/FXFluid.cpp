#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include "FXFluid.h"

#import <OpenGLES/ES1/gl.h>
#import <OpenGLES/ES1/glext.h>


int initPaletteData(char *dataFile,unsigned char **palette);

#define RENDER_TO_TEXT 1


#define K_LOOP_VAL (N>M?N/4:M/4) //20
#define FPP 14
#define X1_0 (1<<FPP)
#define I2X(x) ((freal)((x)<<FPP))
#define F2X(x) ((freal)((x)*X1_0))
#define X2I(x) ((int)((x)>>FPP))
#define X2F(x) ((float)(x)/X1_0)
#define XM(x,y) ((freal)(((long long)(x)*(long long)(y))>>FPP))
#define XD(x,y) ((freal)((((long long)(x))<<FPP)/(y)))

#define IX(i,j) ((i)+(fluid_N+2)*(j))
#define SWAP(x0,x) {freal *tmp=x0;x0=x;x=tmp;}
#define FOR_EACH_CELL for ( i=1 ; i<=N ; i++ ) { for ( j=1 ; j<=M ; j++ ) {
#define END_FOR }}

static unsigned char *paletteDataL,*paletteDataR;
static int palLbpp,palRbpp;
/* global variables */
static GLuint  fluidText;
static unsigned char *fluidTextdata;
static int fluid_N,fluid_M;
static freal dt, diff, visc;
static freal force, source;
static int dvel;

static freal * u, * v, * u_prev, * v_prev;
static freal * dens, * dens_prev;


////////////////////////////////////
// Init stuff
////////////////////////////////
static void free_data ( void )
{
	if ( u ) {free ( u );u=NULL;}
	if ( v ) {free ( v );v=NULL;}
	if ( u_prev ) {free ( u_prev ); u_prev=NULL;}
	if ( v_prev ) {free ( v_prev ); v_prev=NULL;}
	if ( dens ) {free ( dens ); dens=NULL;}
	if ( dens_prev ) {free ( dens_prev ); dens_prev=NULL;}
    if (fluidTextdata) {free(fluidTextdata);fluidTextdata=NULL;}
    if ( paletteDataL ) {free ( paletteDataL );paletteDataL=NULL;}
    if ( paletteDataR ) {free ( paletteDataR );paletteDataR=NULL;}
}

void fluidClear( void )
{
	int i, size=(fluid_N+2)*(fluid_M+2);
    
	/*for ( i=0 ; i<size ; i++ ) {
		u[i] = v[i] = u_prev[i] = v_prev[i] = dens[i] = dens_prev[i] = 0.0f;
	}*/
    memset(u,0,size*sizeof(freal));
    memset(v,0,size*sizeof(freal));
    memset(u_prev,0,size*sizeof(freal));
    memset(v_prev,0,size*sizeof(freal));
    memset(dens,0,size*sizeof(freal));
    memset(dens_prev,0,size*sizeof(freal));
}

static int allocate_data ( void )
{
	int size = (fluid_N+2)*(fluid_M+2);
    
	u			= (freal *) malloc ( size*sizeof(freal) );
	v			= (freal *) malloc ( size*sizeof(freal) );
	u_prev		= (freal *) malloc ( size*sizeof(freal) );
	v_prev		= (freal *) malloc ( size*sizeof(freal) );
	dens		= (freal *) malloc ( size*sizeof(freal) );	
	dens_prev	= (freal *) malloc ( size*sizeof(freal) );
    
	if ( !u || !v || !u_prev || !v_prev || !dens || !dens_prev ) {
		fprintf ( stderr, "cannot allocate data\n" );
		return ( 0 );
	}
    
	return ( 1 );
}

void closeFluid() {
    free_data();
}

int initFluid(int N,int M) {
    palLbpp=initPaletteData("palL.png",&paletteDataL);
    palRbpp=initPaletteData("palR.png",&paletteDataR);
    
    
    fluid_N = N;
    fluid_M = M;

	dt = (1<<FPP)*0.1f;
	diff = (1<<FPP)*0.00f;
	visc = (1<<FPP)*0.000f;
	force = (1<<FPP)*5.0f;
	source = (1<<FPP)*300;
    
	dvel = 0;
    
	if ( !allocate_data () ) return 1;
	fluidClear ();
    
    ///////////
//    [EAGLContext setCurrentContext:m_oglContext];
//	[glView bind];
#if RENDER_TO_TEXT    
	glGenTextures(1, &fluidText);               /* Create 1 Texture */
    fluidTextdata=(unsigned char*)malloc(256*256*4);
    glBindTexture(GL_TEXTURE_2D, fluidText);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 256,
				 0, GL_RGBA, GL_UNSIGNED_BYTE, fluidTextdata);
    glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
#endif	


    
    return 0;
}

int initPaletteData(char *dataFile,unsigned char **palette) {
/*    printf("path: %s\n",dataFile);
    NSData *file = [[NSFileManager defaultManager] contentsAtPath:path];
    if ( !file ) printf("file failed\n");
        CGDataProviderRef src = CGDataProviderCreateWithCFData((CFDataRef)file);
        if ( !src ) printf("image failed\n");
            CGImageRef img = CGImageCreateWithPNGDataProvider(src, NULL, NO, kCGRenderingIntentDefault);
            if ( !img ) printf("image failed\n");
                
                printf("Color space model: %d, indexed=%d\n",
                       CGColorSpaceGetModel(CGImageGetColorSpace(img)),
                       kCGColorSpaceModelIndexed);
                
                CGDataProviderRef data = CGImageGetDataProvider(img);
                NSData *nsdata = (NSData *)(CGDataProviderCopyData(data));
                
                *palette = (unsigned char*)malloc([nsdata length]);
                if ( !(*palette) ) printf("cannot alloc levelData\n");
                    [nsdata getBytes:(*palette)];
    
    int palw = CGImageGetWidth(img);
    int palh = CGImageGetHeight(img);
    int bpl = CGImageGetBytesPerRow(img);
  */
    //BUILD a simple grayscale palette
    *palette = (unsigned char*)malloc(256*3);
    if ( !(*palette) ) printf("cannot alloc levelData\n");
    
    static int tmphack=0;
    tmphack++;
    for (int i=0;i<256;i++) {
        int cr,cg,cb;
        if (tmphack&1) {
            cb=i*3;
            cg=i*2;
            cr=i;
        } else {
            cr=i*3;
            cb=i*2;
            cg=i;
        }
        if (cr>255) cr=255;
        if (cg>255) cg=255;
        if (cb>255) cb=255;
        (*palette)[i*3+0]=cr;
        (*palette)[i*3+1]=cg;
        (*palette)[i*3+2]=cb;
    }
    int bpl=3;
    
//    printf("w%d h%d b%d\n",palw,palh,bpl);
    
    /*    for (int i=0;i<palh;i++) {
     NSLog(@"%X %X %X",paletteData[i*bpl],paletteData[i*bpl+1],paletteData[i*bpl+2]);
     }*/
    
    return bpl;
}


//Rendering stuff
void draw_velocity() {
    static GLfloat *vertices;
    
	int i, j,index;
	float x, y, hi,hj;
    
	hi = 1.0f/fluid_N;
    hj = 1.0f/fluid_M;
    
	glColor4ub(255,255,255,255);
	glLineWidth ( 1.0f );
    
    vertices=(GLfloat*)malloc(fluid_N*fluid_M*4*sizeof(GLfloat));
    index=0;
	for ( i=1 ; i<=fluid_N ; i++ ) {
        x = (i-0.5f)*hi;
        for ( j=1 ; j<=fluid_M ; j++ ) {
            y = (j-0.5f)*hj;
            
            //glVertex2f ( x, y );           
            vertices[index++]=x;
            vertices[index++]=y;
            
            //glVertex2f ( x+u[IX(i,j)], y+v[IX(i,j)] );
            vertices[index++]=x+(float)u[IX(i,j)]/(float)(1<<FPP);
            vertices[index++]=y+(float)v[IX(i,j)]/(float)(1<<FPP);        
        }
    }
    glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, vertices);	
    glDrawArrays(GL_LINES,0,index/2);
    free(vertices);
    glDisableClientState(GL_VERTEX_ARRAY);
}

void draw_density(unsigned char alpha) {
	int ax, i, j, indexV,indexC,cr,cg,cb;
	float x, y, hi,hj;
    freal d00, d01, d10, d11;
    static GLfloat *vertices;
    static GLubyte *vertColor;
    static GLfloat *texcoords;    
	hi = 1.0f/fluid_N;
    hj = 1.0f/fluid_M;
    
#define INIT_COL(x) \
ax=(int)(x>>FPP); \
if (ax>=0) { \
if (ax>255) ax=255; \
cr=paletteDataL[ax*palLbpp+0]; \
cg=paletteDataL[ax*palLbpp+1]; \
cb=paletteDataL[ax*palLbpp+2]; \
} else { \
ax=-ax; if (ax>255) ax=255; \
cr=paletteDataR[ax*palRbpp+0]; \
cg=paletteDataR[ax*palRbpp+1]; \
cb=paletteDataR[ax*palRbpp+2]; \
}
    
#define CHECK_COL \
if (cr>255) cr=255;if (cr<0) cr=0;            \
if (cg>255) cg=255;if (cg<0) cg=0;            \
if (cb>255) cb=255;if (cb<0) cb=0;
    
#if RENDER_TO_TEXT
    for ( i=0 ; i<=fluid_N ; i++ ) {
        //x = (i-0.5f)*hi;
        for ( j=0 ; j<=fluid_M ; j++ ) {
            //y = (j-0.5f)*hj;
            
            d00 = dens[IX(i,j)];
/*            d01 = dens[IX(i,j+1)];
            d10 = dens[IX(i+1,j)];
            d11 = dens[IX(i+1,j+1)];*/
            INIT_COL(d00)
            fluidTextdata[(j*256+i)*4+0]=cr;
            fluidTextdata[(j*256+i)*4+1]=cg;
            fluidTextdata[(j*256+i)*4+2]=cb;
            fluidTextdata[(j*256+i)*4+3]=((alpha+((cr+cg+cb)>>1))>255?255:alpha+((cr+cg+cb)>>1));
            /*            INIT_COL(d01)
            fluidTextdata[((j+1)*256+i)*4+0]=cr;
            fluidTextdata[((j+1)*256+i)*4+1]=cg;
            fluidTextdata[((j+1)*256+i)*4+2]=cb;
            fluidTextdata[((j+1)*256+i)*4+3]=(cb+cg+cb)/3;

            
            INIT_COL(d10)
            fluidTextdata[(j*256+i+1)*4+0]=cr;
            fluidTextdata[(j*256+i+1)*4+1]=cg;
            fluidTextdata[(j*256+i+1)*4+2]=cb;
            fluidTextdata[(j*256+i+1)*4+3]=(cb+cg+cb)/3;

            
            INIT_COL(d11)
            fluidTextdata[((j+1)*256+i+1)*4+0]=cr;
            fluidTextdata[((j+1)*256+i+1)*4+1]=cg;
            fluidTextdata[((j+1)*256+i+1)*4+2]=cb;
            fluidTextdata[((j+1)*256+i+1)*4+3]=(cb+cg+cb)/3;
            fluidTextdata[((j+1)*256+i+1)*4+3]=(cb+cg+cb)/3;*/


            
        }
    }
    
    vertices=(GLfloat*)malloc(sizeof(GLfloat)*4*2);
    texcoords=(GLfloat*)malloc(sizeof(GLfloat)*4*2);
    indexV=indexC=0;
    
//	glEnable(GL_BLEND);
//	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_2D);    
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    
    glBindTexture(GL_TEXTURE_2D, fluidText);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 256,
				 0, GL_RGBA, GL_UNSIGNED_BYTE, fluidTextdata);
    glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    
    glTexCoordPointer(2, GL_FLOAT, 0, texcoords);
    glVertexPointer(2, GL_FLOAT, 0, vertices);
    
    glColor4f(1.0f, 1.0f, 1.0f, alpha*1.0f/255.0f);
    
    vertices[indexV++]=0;vertices[indexV++]=0;
    vertices[indexV++]=1;vertices[indexV++]=0;
    vertices[indexV++]=0;vertices[indexV++]=1;
    vertices[indexV++]=1;vertices[indexV++]=1;
    
    texcoords[indexC++]=2.0f/256.0f;texcoords[indexC++]=2.0f/256.0f;
    texcoords[indexC++]=(float)(fluid_N-1)/256.0f;texcoords[indexC++]=2.0f/256.0f;
    texcoords[indexC++]=2.0f/256.0f;texcoords[indexC++]=(float)(fluid_M-1)/256.0f;
    texcoords[indexC++]=(float)(fluid_N-1)/256.0f;texcoords[indexC++]=(float)(fluid_M-1)/256.0f;
    
    glDrawArrays(GL_TRIANGLE_STRIP,0,indexV/2);
    
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisable(GL_TEXTURE_2D);
//    glDisable(GL_BLEND);
    
    free(vertices);
    free(texcoords);
    
#else    
    vertices=(GLfloat*)malloc(sizeof(GLfloat)*2*3*(fluid_N+1)*(fluid_N+1)*2);
    vertColor=(GLubyte*)malloc(sizeof(GLubyte)*4*3*(fluid_N+1)*(fluid_N+1)*2);
    indexV=indexC=0;
    
    for ( i=0 ; i<=fluid_N ; i++ ) {
        x = (i-0.5f)*hi;
        for ( j=0 ; j<=fluid_M ; j++ ) {
            y = (j-0.5f)*hj;
            
            d00 = dens[IX(i,j)];
            d01 = dens[IX(i,j+1)];
            d10 = dens[IX(i+1,j)];
            d11 = dens[IX(i+1,j+1)];
            INIT_COL(d00)
            CHECK_COL
            
            //1ST TRIANGLE
            //glColor3f ( d00, d00, d00 ); glVertex2f ( x, y );
            vertColor[indexC++]=cr;
            vertColor[indexC++]=cg;
            vertColor[indexC++]=cb;
            vertColor[indexC++]=255; //alpha            
            vertices[indexV++]=x;vertices[indexV++]=y;
            
            INIT_COL(d10)
            CHECK_COL            
            //glColor3f ( d10, d10, d10 ); glVertex2f ( x+h, y );
            vertColor[indexC++]=cr;
            vertColor[indexC++]=cg;
            vertColor[indexC++]=cb;
            vertColor[indexC++]=255; //alpha            
            vertices[indexV++]=x+hi;vertices[indexV++]=y;
            
            INIT_COL(d11)
            CHECK_COL            
            //glColor3f ( d11, d11, d11 ); glVertex2f ( x+h, y+h );
            vertColor[indexC++]=cr;
            vertColor[indexC++]=cg;
            vertColor[indexC++]=cb;
            vertColor[indexC++]=255; //alpha            
            vertices[indexV++]=x+hi;vertices[indexV++]=y+hj;
            
            
            //2ND TRIANGLE
            //glColor3f ( d11, d11, d11 ); glVertex2f ( x+h, y+h );
            vertColor[indexC++]=cr;
            vertColor[indexC++]=cg;
            vertColor[indexC++]=cb;
            vertColor[indexC++]=255; //alpha            
            vertices[indexV++]=x+hi;vertices[indexV++]=y+hj;
            
            
            INIT_COL(d01)
            CHECK_COL            
            //glColor3f ( d01, d01, d01 ); glVertex2f ( x, y+h );
            vertColor[indexC++]=cr;
            vertColor[indexC++]=cg;
            vertColor[indexC++]=cb;
            vertColor[indexC++]=255; //alpha            
            vertices[indexV++]=x;vertices[indexV++]=y+hj;
            
            INIT_COL(d00)
            CHECK_COL            
            //glColor3f ( d00, d00, d00 ); glVertex2f ( x, y );
            vertColor[indexC++]=cr;
            vertColor[indexC++]=cg;
            vertColor[indexC++]=cb;
            vertColor[indexC++]=255; //alpha            
            vertices[indexV++]=x;vertices[indexV++]=y;
            
            
        }
    }
    
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, vertices);
    glColorPointer(4, GL_UNSIGNED_BYTE, 0, vertColor);
    glDrawArrays(GL_TRIANGLES,0,indexV/2);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    
    free(vertices);
    free(vertColor);
#endif
}
//
void renderFluid(int width,int height,unsigned char *beatL,unsigned char *beatR,short int *spectL,short int *spectR,int count,int mode,unsigned char alpha) {
    int ci,k=0;
    
    int size = (fluid_N+2)*(fluid_M+2);
        
    memset(u_prev,0,size*sizeof(freal));
    memset(v_prev,0,size*sizeof(freal));
    memset(dens_prev,0,size*sizeof(freal));
    
    freal denscol=source;
    int wsrc=(fluid_N+2)/(count+2)/2;
    for (int i=0;i<count;i++) {
        ci=(i+1)*(fluid_N+2)/(count+2);
        
        k++;
        if (k==3) {
            denscol=-denscol;
            k=1;
        }
        
        freal uforce1=((rand()&15)-7)*force/64;
        freal uforce2=((rand()&15)-7)*force/64;
        for (int j=0;j<fluid_M/16;j++) {
            
            for (int l=-wsrc;l<=wsrc;l++) {
                if (beatL[i]) {
                    dens_prev[IX(ci+l,j)] = denscol;
                    v_prev[IX(ci+l,j)] = force;
                    u_prev[IX(ci+l,j)] = uforce1;
                }
                
                if (beatR[i]) {
                    dens_prev[IX(ci+l,fluid_M+1-j)] = denscol;
                    v_prev[IX(ci+l,fluid_M+1-j)] = -force;                    
                    u_prev[IX(ci+l,fluid_M+1-j)] = uforce2;
                }
            }
        }
    }        
    
    //update
    vel_step ( fluid_N, fluid_M, u, v, u_prev, v_prev, visc, dt );
    dens_step ( fluid_N, fluid_M, dens, dens_prev, u, v, diff, dt );
    //dispersion
	for (int i=0 ; i<(fluid_N+2)*(fluid_M+2) ; i++ ) {
        dens[i]=(dens[i]*253)>>8;
//        u[i]=(u[i]*254)>>8;
//        v[i]=(v[i]*254)>>8;
    }
    
    //render
    glMatrixMode ( GL_PROJECTION );
	glLoadIdentity ();
    glOrthof(0, 1, 0, 1, 0, 1);
    glTranslatef(0,0,-1);
    if ( dvel ) draw_velocity();
    else		draw_density(alpha);
    
}

#undef IX
#define IX(i,j) ((i)+(N+2)*(j))

//FIXED POINT version
void add_source ( int N, int M,freal * x, freal * s, freal dt );
void set_bnd ( int N, int M,int b, freal * x );
void lin_solve ( int N, int M,int b, freal * x, freal * x0, freal a, freal c );
void diffuse ( int N, int M, int b, freal * x, freal * x0, freal diff, freal dt );
void advect ( int N, int M, int b, freal * d, freal * d0, freal * u, freal * v, freal dt );
void project ( int N, int M, freal * u, freal * v, freal * p, freal * div );

//2 solver for vect & density
void dens_step ( int N, int M, freal * x, freal * x0, freal * u, freal * v, freal diff, freal dt ) {
	add_source ( N, M, x, x0, dt );
	SWAP ( x0, x ); diffuse ( N, M, 0, x, x0, diff, dt );
	SWAP ( x0, x ); advect ( N, M, 0, x, x0, u, v, dt );
}

void vel_step ( int N, int M, freal * u, freal * v, freal * u0, freal * v0, freal visc, freal dt ) {
	add_source ( N, M, u, u0, dt ); add_source ( N, M, v, v0, dt );
	SWAP ( u0, u ); diffuse ( N, M, 1, u, u0, visc, dt );
	SWAP ( v0, v ); diffuse ( N, M, 2, v, v0, visc, dt );
	project ( N, M, u, v, u0, v0 );
	SWAP ( u0, u ); SWAP ( v0, v );
	advect ( N, M, 1, u, u0, u0, v0, dt ); advect ( N, M, 2, v, v0, u0, v0, dt );
	project ( N, M, u, v, u0, v0 );
}


//Internal stuff
void add_source ( int N, int M,freal * x, freal * s, freal dt ) {
	int i, size=(N+2)*(M+2);
	for ( i=0 ; i<size ; i++ ) x[i] += XM(dt,s[i]);
}

void set_bnd ( int N, int M,int b, freal * x ) {
	int i,j;
    
	for ( i=1 ; i<=N ; i++ ) {
		x[IX(i,0  )] = b==2 ? -x[IX(i,1)] : x[IX(i,1)];
		x[IX(i,M+1)] = b==2 ? -x[IX(i,M)] : x[IX(i,M)];
	}
    for ( j=1 ; j<=M ; j++ ) {
		x[IX(0  ,j)] = b==1 ? -x[IX(1,j)] : x[IX(1,j)];
		x[IX(N+1,j)] = b==1 ? -x[IX(N,j)] : x[IX(N,j)];
	}

	x[IX(0  ,0  )] = (x[IX(1,0  )]+x[IX(0  ,1)])>>1;
	x[IX(0  ,M+1)] = (x[IX(1,M+1)]+x[IX(0  ,M)])>>1;
	x[IX(N+1,0  )] = (x[IX(N,0  )]+x[IX(N+1,1)])>>1;
	x[IX(N+1,M+1)] = (x[IX(N,M+1)]+x[IX(N+1,M)])>>1;
}

void lin_solve ( int N, int M,int b, freal * x, freal * x0, freal a, freal c ) {
	int i, j, k;
    
    if ((a==0)&&(c==I2X(1))) {
        FOR_EACH_CELL
        x[IX(i,j)] = x0[IX(i,j)];
		END_FOR
    } else {
        for ( k=0 ; k<K_LOOP_VAL ; k++ ) {
            FOR_EACH_CELL
			x[IX(i,j)] = XM((x0[IX(i,j)] + XM(a,(x[IX(i-1,j)]+x[IX(i+1,j)]+x[IX(i,j-1)]+x[IX(i,j+1)]))),c);
            END_FOR
            set_bnd ( N, M, b, x );
        }
    }
}

void diffuse ( int N, int M, int b, freal * x, freal * x0, freal diff, freal dt ) {
	freal a=XM(dt,diff)*N*M;
	lin_solve ( N, M, b, x, x0, a, XD(I2X(1),(I2X(1)+4*a)) );
}

void advect ( int N, int M, int b, freal * d, freal * d0, freal * u, freal * v, freal dt ) {
	int i, j, i0, j0, i1, j1;
	freal x, y, s0, t0, s1, t1, dt0;
    
	dt0 = dt*N;
	FOR_EACH_CELL
    x = I2X(i)-XM(dt0,u[IX(i,j)]); y = I2X(j)-XM(dt0,v[IX(i,j)]);
    if (x<F2X(0.5f)) x=F2X(0.5f); if (x>I2X(N)+F2X(0.5f)) x=I2X(N)+F2X(0.5f); i0=X2I(x); i1=i0+1;
    if (y<F2X(0.5f)) y=F2X(0.5f); if (y>I2X(M)+F2X(0.5f)) y=I2X(M)+F2X(0.5f); j0=X2I(y); j1=j0+1;
    s1 = x-I2X(i0); s0 = I2X(1)-s1; t1 = y-I2X(j0); t0 = I2X(1)-t1;
    d[IX(i,j)] = XM(s0,(XM(t0,d0[IX(i0,j0)])+XM(t1,d0[IX(i0,j1)])))+
    XM(s1,(XM(t0,d0[IX(i1,j0)])+XM(t1,d0[IX(i1,j1)])));
	END_FOR
	set_bnd ( N, M, b, d );}

void project ( int N, int M, freal * u, freal * v, freal * p, freal * div ){
	int i, j;
    
	FOR_EACH_CELL
    div[IX(i,j)] = -((u[IX(i+1,j)]-u[IX(i-1,j)])+(v[IX(i,j+1)]-v[IX(i,j-1)]))/(N*2);
    p[IX(i,j)] = 0;
	END_FOR	
	set_bnd ( N, M, 0, div ); set_bnd ( N, M, 0, p );
    
	lin_solve ( N, M, 0, p, div, I2X(1), F2X(1.0f/4.0f) );
    
	FOR_EACH_CELL
    u[IX(i,j)] -= (N*(p[IX(i+1,j)]-p[IX(i-1,j)]))/2;
    v[IX(i,j)] -= (M*(p[IX(i,j+1)]-p[IX(i,j-1)]))/2;
	END_FOR
	set_bnd ( N, M, 1, u ); set_bnd ( N, M, 2, v );
}

/*
 #define IX(i,j) ((i)+(N+2)*(j))
 #define SWAP(x0,x) {float * tmp=x0;x0=x;x=tmp;}
 #define FOR_EACH_CELL for ( i=1 ; i<=N ; i++ ) { for ( j=1 ; j<=N ; j++ ) {
 #define END_FOR }}

 
 void add_source ( int N, float * x, float * s, float dt )
{
	int i, size=(N+2)*(N+2);
	for ( i=0 ; i<size ; i++ ) x[i] += dt*s[i];
}

void set_bnd ( int N, int b, float * x )
{
	int i;

	for ( i=1 ; i<=N ; i++ ) {
		x[IX(0  ,i)] = b==1 ? -x[IX(1,i)] : x[IX(1,i)];
		x[IX(N+1,i)] = b==1 ? -x[IX(N,i)] : x[IX(N,i)];
		x[IX(i,0  )] = b==2 ? -x[IX(i,1)] : x[IX(i,1)];
		x[IX(i,N+1)] = b==2 ? -x[IX(i,N)] : x[IX(i,N)];
	}
	x[IX(0  ,0  )] = 0.5f*(x[IX(1,0  )]+x[IX(0  ,1)]);
	x[IX(0  ,N+1)] = 0.5f*(x[IX(1,N+1)]+x[IX(0  ,N)]);
	x[IX(N+1,0  )] = 0.5f*(x[IX(N,0  )]+x[IX(N+1,1)]);
	x[IX(N+1,N+1)] = 0.5f*(x[IX(N,N+1)]+x[IX(N+1,N)]);
}

void lin_solve ( int N, int b, float * x, float * x0, float a, float c )
{
	int i, j, k;
    
    if ((a==0)&&(c==1)) {
        FOR_EACH_CELL
            x[IX(i,j)] = x0[IX(i,j)];
		END_FOR
    } else {
	for ( k=0 ; k<K_LOOP_VAL ; k++ ) {
		FOR_EACH_CELL
			x[IX(i,j)] = (x0[IX(i,j)] + a*(x[IX(i-1,j)]+x[IX(i+1,j)]+x[IX(i,j-1)]+x[IX(i,j+1)]))/c;
		END_FOR
		set_bnd ( N, b, x );
	}
    }
}

void diffuse ( int N, int b, float * x, float * x0, float diff, float dt )
{
	float a=dt*diff*N*N;
	lin_solve ( N, b, x, x0, a, 1+4*a );
}

void advect ( int N, int b, float * d, float * d0, float * u, float * v, float dt )
{
	int i, j, i0, j0, i1, j1;
	float x, y, s0, t0, s1, t1, dt0;

	dt0 = dt*N;
	FOR_EACH_CELL
		x = i-dt0*u[IX(i,j)]; y = j-dt0*v[IX(i,j)];
		if (x<0.5f) x=0.5f; if (x>N+0.5f) x=N+0.5f; i0=(int)x; i1=i0+1;
		if (y<0.5f) y=0.5f; if (y>N+0.5f) y=N+0.5f; j0=(int)y; j1=j0+1;
		s1 = x-i0; s0 = 1-s1; t1 = y-j0; t0 = 1-t1;
		d[IX(i,j)] = s0*(t0*d0[IX(i0,j0)]+t1*d0[IX(i0,j1)])+
					 s1*(t0*d0[IX(i1,j0)]+t1*d0[IX(i1,j1)]);
	END_FOR
	set_bnd ( N, b, d );
}

void project ( int N, float * u, float * v, float * p, float * div )
{
	int i, j;

	FOR_EACH_CELL
		div[IX(i,j)] = -0.5f*(u[IX(i+1,j)]-u[IX(i-1,j)]+v[IX(i,j+1)]-v[IX(i,j-1)])/N;
		p[IX(i,j)] = 0;
	END_FOR	
	set_bnd ( N, 0, div ); set_bnd ( N, 0, p );

	lin_solve ( N, 0, p, div, 1, 4 );

	FOR_EACH_CELL
		u[IX(i,j)] -= 0.5f*N*(p[IX(i+1,j)]-p[IX(i-1,j)]);
		v[IX(i,j)] -= 0.5f*N*(p[IX(i,j+1)]-p[IX(i,j-1)]);
	END_FOR
	set_bnd ( N, 1, u ); set_bnd ( N, 2, v );
}

void dens_step ( int N, float * x, float * x0, float * u, float * v, float diff, float dt )
{
	add_source ( N, x, x0, dt );
	SWAP ( x0, x ); diffuse ( N, 0, x, x0, diff, dt );
	SWAP ( x0, x ); advect ( N, 0, x, x0, u, v, dt );
}

void vel_step ( int N, float * u, float * v, float * u0, float * v0, float visc, float dt )
{
	add_source ( N, u, u0, dt ); add_source ( N, v, v0, dt );
	SWAP ( u0, u ); diffuse ( N, 1, u, u0, visc, dt );
	SWAP ( v0, v ); diffuse ( N, 2, v, v0, visc, dt );
	project ( N, u, v, u0, v0 );
	SWAP ( u0, u ); SWAP ( v0, v );
	advect ( N, 1, u, u0, u0, v0, dt ); advect ( N, 2, v, v0, u0, v0, dt );
	project ( N, u, v, u0, v0 );
}
*/
