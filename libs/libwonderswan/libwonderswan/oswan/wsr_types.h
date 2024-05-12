//
//  wsr_types.h
//  libwonderswan
//
//  Created by Yohann Magnien David on 12/05/2024.
//

#ifndef wsr_types_h
#define wsr_types_h

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned int DWORD;

typedef unsigned int UINT32;
typedef unsigned short UINT16;
typedef unsigned char UINT8;

typedef int INT32;
typedef short INT16;
typedef char INT8;

typedef int BOOLEAN;
typedef int BOOL;

typedef int HWND;
typedef int HINSTANCE;
typedef long LRESULT;

typedef unsigned int UINT;

typedef char *LPSTR;
typedef char *LPCSTR;

typedef int WPARAM;
typedef int LPARAM;

#define CALLBACK

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define DIK_UP 1
#define DIK_DOWN 2
#define DIK_RIGHT 3
#define DIK_LEFT 4
#define DIK_W 5
#define DIK_S 6
#define DIK_D 7
#define DIK_A 8
#define DIK_RETURN 9
#define DIK_C 10
#define DIK_X 11
#define DIK_SPACE 23


#endif /* wsr_types_h */
