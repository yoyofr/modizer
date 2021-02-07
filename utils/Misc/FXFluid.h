#ifndef st_FXFluid_h_
#define st_FXFluid_h_


typedef signed int freal;

int initFluid(int width,int height);
void closeFluid();
void renderFluid(int width,int height,unsigned char *beatL,unsigned char *beatR,short int *spectL,short int *spectR,int count,int mode,unsigned char alpha);


void dens_step ( int N, int M, freal * x, freal * x0, freal * u, freal * v, freal diff, freal dt );
void vel_step ( int N, int M, freal * u, freal * v, freal * u0, freal * v0, freal visc, freal dt );

#endif