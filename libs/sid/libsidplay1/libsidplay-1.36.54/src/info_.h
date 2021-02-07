//
// /home/ms/source/sidplay/libsidplay/fformat/RCS/info_.h,v
//

#ifndef SIDPLAY1_INFO__H
#define SIDPLAY1_INFO__H


#include "compconf.h"
#ifdef SID_HAVE_EXCEPTIONS
#include <new>
#endif
#include <cstring>
#include <string>
using std::string;
#ifdef SID_HAVE_SSTREAM
#include <sstream>
using std::istringstream;
#else
#include <strstream.h>
#endif
#include "mytypes.h"
#include "myendian.h"
#include "fformat.h"
#include "smart.h"
#include "sidtune.h"


// Amiga Workbench specific structures.

struct Border
{
	ubyte LeftEdge[2];            // uword; initial offsets from the origin
	ubyte TopEdge[2];             // uword
	ubyte FrontPen, BackPen;	  // pens numbers for rendering
	ubyte DrawMode;		          // mode for rendering
	ubyte Count;			      // number of XY pairs
	ubyte pXY[4];			      // sword *XY; vector coordinate pairs rel to LeftTop
	ubyte pNextBorder[4];         // Border *NextBorder; pointer to any other Border too
};

struct Image
{
	ubyte LeftEdge[2];            // uword; starting offset relative to some origin
	ubyte TopEdge[2];	          // uword; starting offsets relative to some origin
	ubyte Width[2];	              // uword; pixel size (though data is word-aligned)
	ubyte Height[2];              // uword
	ubyte Depth[2];	              // uword; >= 0, for images you create
	ubyte pImageData[4];          // uword *ImageData; pointer to the actual word-aligned bits
	ubyte PlanePick, PlaneOnOff;
	ubyte pNextImage[4];          // Image *NextImage;
};

struct Gadget
{
	ubyte pNextGadget[4];         // Gadget *NextGadget; next gadget in the list 
	ubyte LeftEdge[2];		      // uword; "hit box" of gadget 
	ubyte TopEdge[2];             // uword
	ubyte Width[2];		          // uword; "hit box" of gadget 
	ubyte Height[2];              // uword
	ubyte Flags[2];		          // uword; see below for list of defines 
	ubyte Activation[2];          // uword
	ubyte GadgetType[2];	      // uword; see below for defines 
	ubyte pGadgetRender[4];       // Image *GadgetRender;
	ubyte pSelectRender[4];       // Image *SelectRender;
	ubyte pGadgetText[4];         // void *GadgetText;
	ubyte MutualExclude[4];       // udword
	ubyte pSpecialInfo[4];        // void *SpecialInfo;
	ubyte GadgetID[2];            // uword
	ubyte UserData[4];	          // udword; ptr to general purpose User data 
};

struct DiskObject
{
	ubyte Magic[2];               // uword; a magic num at the start of the file 
	ubyte Version[2];		      // uword; a version number, so we can change it 
	struct Gadget Gadget;		  // a copy of in core gadget 
	ubyte Type;
	ubyte PAD_BYTE;               // Pad it out to the next word boundry 
	ubyte pDefaultTool[4];        // char *DefaultTool;
	ubyte ppToolTypes[4];         // char **ToolTypes;
	ubyte CurrentX[4];            // udword
	ubyte CurrentY[4];            // udword
	ubyte pDrawerData[4];         // char *DrawerData;
	ubyte pToolWindow[4];         // char *ToolWindow; only applies to tools 
	ubyte StackSize[4];           // udword; only applies to tools 
};


// A magic number, not easily impersonated.
#define WB_DISKMAGIC 0xE310
// Our current version number.
#define WB_DISKVERSION 1
// Our current revision number.
#define WB_DISKREVISION	1
// I only use the lower 8 bits of Gadget.UserData for the revision #.
#define WB_DISKREVISIONMASK	0xFF

// The Workbench object types.
#define	WB_DISK		1
#define	WB_DRAWER	2
#define	WB_TOOL		3
#define	WB_PROJECT	4
#define	WB_GARBAGE	5
#define	WB_DEVICE	6
#define	WB_KICK		7
#define	WB_APPICON	8

// --- Gadget.Flags values	---
// Combinations in these bits describe the highlight technique to be used.
#define GFLG_GADGHIGHBITS 0x0003
// Complement the select box.
#define GFLG_GADGHCOMP	  0x0000
// Draw a box around the image.
#define GFLG_GADGHBOX	  0x0001
// Blast in this alternate image.
#define GFLG_GADGHIMAGE	  0x0002
// Don't highlight.
#define GFLG_GADGHNONE	  0x0003
// Set if GadgetRender and SelectRender point to an Image structure,
// clear if they point to Border structures.
#define GFLG_GADGIMAGE	  0x0004


#endif  /* SIDPLAY1_INFO__H */
