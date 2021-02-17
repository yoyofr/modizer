/******************************************************************************
 ** Filename:    intproto.c
 ** Purpose:     Definition of data structures for integer protos.
 ** Author:      Dan Johnson
 ** History:     Thu Feb  7 14:38:16 1991, DSJ, Created.
 **
 ** (c) Copyright Hewlett-Packard Company, 1988.
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 ** http://www.apache.org/licenses/LICENSE-2.0
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 ******************************************************************************/
/*-----------------------------------------------------------------------------
          Include Files and Type Defines
-----------------------------------------------------------------------------*/

#include <math.h>
#include <stdio.h>
#include <assert.h>
#ifdef __UNIX__
#include <unistd.h>
#endif

#include "classify.h"
#include "const.h"
#include "emalloc.h"
#include "fontinfo.h"
#include "genericvector.h"
#include "globals.h"
#include "helpers.h"
#include "intproto.h"
#include "mfoutline.h"
#include "ndminx.h"
#include "picofeat.h"
#include "points.h"
#include "shapetable.h"
#include "svmnode.h"

// Include automatically generated configuration file if running autoconf.
#ifdef HAVE_CONFIG_H
#include "config_auto.h"
#endif

using tesseract::FontSet;

/* match debug display constants*/
#define PROTO_PRUNER_SCALE  (4.0)

#define INT_DESCENDER (0.0  * INT_CHAR_NORM_RANGE)
#define INT_BASELINE  (0.25 * INT_CHAR_NORM_RANGE)
#define INT_XHEIGHT (0.75 * INT_CHAR_NORM_RANGE)
#define INT_CAPHEIGHT (1.0  * INT_CHAR_NORM_RANGE)

#define INT_XCENTER (0.5  * INT_CHAR_NORM_RANGE)
#define INT_YCENTER (0.5  * INT_CHAR_NORM_RANGE)
#define INT_XRADIUS (0.2  * INT_CHAR_NORM_RANGE)
#define INT_YRADIUS (0.2  * INT_CHAR_NORM_RANGE)
#define INT_MIN_X 0
#define INT_MIN_Y 0
#define INT_MAX_X INT_CHAR_NORM_RANGE
#define INT_MAX_Y INT_CHAR_NORM_RANGE

/** define pad used to snap near horiz/vertical protos to horiz/vertical */
#define HV_TOLERANCE  (0.0025)   /* approx 0.9 degrees */

typedef enum
{ StartSwitch, EndSwitch, LastSwitch }
SWITCH_TYPE;
#define MAX_NUM_SWITCHES  3

typedef struct
{
  SWITCH_TYPE Type;
  inT8 X, Y;
  inT16 YInit;
  inT16 Delta;
}


FILL_SWITCH;

typedef struct
{
  uinT8 NextSwitch;
  uinT8 AngleStart, AngleEnd;
  inT8 X;
  inT16 YStart, YEnd;
  inT16 StartDelta, EndDelta;
  FILL_SWITCH Switch[MAX_NUM_SWITCHES];
}


TABLE_FILLER;

typedef struct
{
  inT8 X;
  inT8 YStart, YEnd;
  uinT8 AngleStart, AngleEnd;
}


FILL_SPEC;


/* constants for conversion from old inttemp format */
#define OLD_MAX_NUM_CONFIGS      32
#define OLD_WERDS_PER_CONFIG_VEC ((OLD_MAX_NUM_CONFIGS + BITS_PER_WERD - 1) /\
                                  BITS_PER_WERD)

/*-----------------------------------------------------------------------------
            Macros
-----------------------------------------------------------------------------*/
/** macro for performing circular increments of bucket indices */
#define CircularIncrement(i,r)  (((i) < (r) - 1)?((i)++):((i) = 0))

/** macro for mapping floats to ints without bounds checking */
#define MapParam(P,O,N)   (floor (((P) + (O)) * (N)))

/*---------------------------------------------------------------------------
            Private Function Prototypes
----------------------------------------------------------------------------*/
FLOAT32 BucketStart(int Bucket, FLOAT32 Offset, int NumBuckets);

FLOAT32 BucketEnd(int Bucket, FLOAT32 Offset, int NumBuckets);

void DoFill(FILL_SPEC *FillSpec,
            CLASS_PRUNER_STRUCT* Pruner,
            register uinT32 ClassMask,
            register uinT32 ClassCount,
            register uinT32 WordIndex);

BOOL8 FillerDone(TABLE_FILLER *Filler);

void FillPPCircularBits(uinT32
                        ParamTable[NUM_PP_BUCKETS][WERDS_PER_PP_VECTOR],
                        int Bit, FLOAT32 Center, FLOAT32 Spread, bool debug);

void FillPPLinearBits(uinT32 ParamTable[NUM_PP_BUCKETS][WERDS_PER_PP_VECTOR],
                      int Bit, FLOAT32 Center, FLOAT32 Spread, bool debug);

void GetCPPadsForLevel(int Level,
                       FLOAT32 *EndPad,
                       FLOAT32 *SidePad,
                       FLOAT32 *AnglePad);

ScrollView::Color GetMatchColorFor(FLOAT32 Evidence);

void GetNextFill(TABLE_FILLER *Filler, FILL_SPEC *Fill);

void InitTableFiller(FLOAT32 EndPad,
                     FLOAT32 SidePad,
                     FLOAT32 AnglePad,
                     PROTO Proto,
                     TABLE_FILLER *Filler);

#ifndef GRAPHICS_DISABLED
void RenderIntFeature(ScrollView *window, const INT_FEATURE_STRUCT* Feature,
                      ScrollView::Color color);

void RenderIntProto(ScrollView *window,
                    INT_CLASS Class,
                    PROTO_ID ProtoId,
                    ScrollView::Color color);
#endif  // GRAPHICS_DISABLED

int TruncateParam(FLOAT32 Param, int Min, int Max, char *Id);

/*-----------------------------------------------------------------------------
        Global Data Definitions and Declarations
-----------------------------------------------------------------------------*/

/* global display lists used to display proto and feature match information*/
ScrollView *IntMatchWindow = NULL;
ScrollView *FeatureDisplayWindow = NULL;
ScrollView *ProtoDisplayWindow = NULL;

/*-----------------------------------------------------------------------------
        Variables
-----------------------------------------------------------------------------*/

/* control knobs */
INT_VAR(classify_num_cp_levels, 3, "Number of Class Pruner Levels");
double_VAR(classify_cp_angle_pad_loose, 45.0,
           "Class Pruner Angle Pad Loose");
double_VAR(classify_cp_angle_pad_medium, 20.0,
           "Class Pruner Angle Pad Medium");
double_VAR(classify_cp_angle_pad_tight, 10.0,
           "CLass Pruner Angle Pad Tight");
double_VAR(classify_cp_end_pad_loose, 0.5, "Class Pruner End Pad Loose");
double_VAR(classify_cp_end_pad_medium, 0.5, "Class Pruner End Pad Medium");
double_VAR(classify_cp_end_pad_tight, 0.5, "Class Pruner End Pad Tight");
double_VAR(classify_cp_side_pad_loose, 2.5, "Class Pruner Side Pad Loose");
double_VAR(classify_cp_side_pad_medium, 1.2, "Class Pruner Side Pad Medium");
double_VAR(classify_cp_side_pad_tight, 0.6, "Class Pruner Side Pad Tight");
double_VAR(classify_pp_angle_pad, 45.0, "Proto Pruner Angle Pad");
double_VAR(classify_pp_end_pad, 0.5, "Proto Prune End Pad");
double_VAR(classify_pp_side_pad, 2.5, "Proto Pruner Side Pad");

/*-----------------------------------------------------------------------------
              Public Code
-----------------------------------------------------------------------------*/
/// Builds a feature from an FCOORD for position with all the necessary
/// clipping and rounding.
INT_FEATURE_STRUCT::INT_FEATURE_STRUCT(const FCOORD& pos, uinT8 theta)
  : X(ClipToRange<inT16>(static_cast<inT16>(pos.x() + 0.5), 0, 255)),
    Y(ClipToRange<inT16>(static_cast<inT16>(pos.y() + 0.5), 0, 255)),
    Theta(theta),
    CP_misses(0) {
}
/** Builds a feature from ints with all the necessary clipping and casting. */
INT_FEATURE_STRUCT::INT_FEATURE_STRUCT(int x, int y, int theta)
  : X(static_cast<uinT8>(ClipToRange(x, 0, MAX_UINT8))),
    Y(static_cast<uinT8>(ClipToRange(y, 0, MAX_UINT8))),
    Theta(static_cast<uinT8>(ClipToRange(theta, 0, MAX_UINT8))),
    CP_misses(0) {
}

/**
 * This routine adds a new class structure to a set of
 * templates. Classes have to be added to Templates in
 * the order of increasing ClassIds.
 *
 * @param Templates templates to add new class to
 * @param ClassId   class id to associate new class with
 * @param Class   class data structure to add to templates
 *
 * Globals: none
 *
 * @note Exceptions: none
 * @note History: Mon Feb 11 11:52:08 1991, DSJ, Created.
 */
void AddIntClass(INT_TEMPLATES Templates, CLASS_ID ClassId, INT_CLASS Class) {
  int Pruner;

  assert (LegalClassId (ClassId));
  if (ClassId != Templates->NumClasses) {
    fprintf(stderr, "Please make sure that classes are added to templates");
    fprintf(stderr, " in increasing order of ClassIds\n");
    exit(1);
  }
  ClassForClassId (Templates, ClassId) = Class;
  Templates->NumClasses++;

  if (Templates->NumClasses > MaxNumClassesIn (Templates)) {
    Pruner = Templates->NumClassPruners++;
    Templates->ClassPruners[Pruner] = new CLASS_PRUNER_STRUCT;
    memset(Templates->ClassPruners[Pruner], 0, sizeof(CLASS_PRUNER_STRUCT));
  }
}                                /* AddIntClass */


/**
 * This routine returns the index of the next free config
 * in Class.
 *
 * @param Class class to add new configuration to
 *
 * Globals: none
 *
 * @return Index of next free config.
 * @note Exceptions: none
 * @note History: Mon Feb 11 14:44:40 1991, DSJ, Created.
 */
int AddIntConfig(INT_CLASS Class) {
  int Index;

  assert(Class->NumConfigs < MAX_NUM_CONFIGS);

  Index = Class->NumConfigs++;
  Class->ConfigLengths[Index] = 0;
  return Index;
}                                /* AddIntConfig */


/**
 * This routine allocates the next free proto in Class and
 * returns its index.
 *
 * @param Class class to add new proto to
 *
 * Globals: none
 *
 * @return Proto index of new proto.
 * @note Exceptions: none
 * @note History: Mon Feb 11 13:26:41 1991, DSJ, Created.
 */
int AddIntProto(INT_CLASS Class) {
  int Index;
  int ProtoSetId;
  PROTO_SET ProtoSet;
  INT_PROTO Proto;
  uinT32 *Word;

  if (Class->NumProtos >= MAX_NUM_PROTOS)
    return (NO_PROTO);

  Index = Class->NumProtos++;

  if (Class->NumProtos > MaxNumIntProtosIn(Class)) {
    ProtoSetId = Class->NumProtoSets++;

    ProtoSet = (PROTO_SET) Emalloc(sizeof(PROTO_SET_STRUCT));
    Class->ProtoSets[ProtoSetId] = ProtoSet;
    memset(ProtoSet, 0, sizeof(*ProtoSet));

    /* reallocate space for the proto lengths and install in class */
    Class->ProtoLengths =
      (uinT8 *)Erealloc(Class->ProtoLengths,
                        MaxNumIntProtosIn(Class) * sizeof(uinT8));
    memset(&Class->ProtoLengths[Index], 0,
           sizeof(*Class->ProtoLengths) * (MaxNumIntProtosIn(Class) - Index));
  }

  /* initialize proto so its length is zero and it isn't in any configs */
  Class->ProtoLengths[Index] = 0;
  Proto = ProtoForProtoId (Class, Index);
  for (Word = Proto->Configs;
       Word < Proto->Configs + WERDS_PER_CONFIG_VEC; *Word++ = 0);

  return (Index);
}

/**
 * This routine adds Proto to the class pruning tables
 * for the specified class in Templates.
 *
 * Globals:
 *  - classify_num_cp_levels number of levels used in the class pruner
 * @param Proto   floating-pt proto to add to class pruner
 * @param ClassId   class id corresponding to Proto
 * @param Templates set of templates containing class pruner
 * @return none
 * @note Exceptions: none
 * @note History: Wed Feb 13 08:49:54 1991, DSJ, Created.
 */
void AddProtoToClassPruner (PROTO Proto, CLASS_ID ClassId,
                            INT_TEMPLATES Templates)
#define MAX_LEVEL     2
{
  CLASS_PRUNER_STRUCT* Pruner;
  uinT32 ClassMask;
  uinT32 ClassCount;
  uinT32 WordIndex;
  int Level;
  FLOAT32 EndPad, SidePad, AnglePad;
  TABLE_FILLER TableFiller;
  FILL_SPEC FillSpec;

  Pruner = CPrunerFor (Templates, ClassId);
  WordIndex = CPrunerWordIndexFor (ClassId);
  ClassMask = CPrunerMaskFor (MAX_LEVEL, ClassId);

  for (Level = classify_num_cp_levels - 1; Level >= 0; Level--) {
    GetCPPadsForLevel(Level, &EndPad, &SidePad, &AnglePad);
    ClassCount = CPrunerMaskFor (Level, ClassId);
    InitTableFiller(EndPad, SidePad, AnglePad, Proto, &TableFiller);

    while (!FillerDone (&TableFiller)) {
      GetNextFill(&TableFiller, &FillSpec);
      DoFill(&FillSpec, Pruner, ClassMask, ClassCount, WordIndex);
    }
  }
}                                /* AddProtoToClassPruner */

/**
 * This routine updates the proto pruner lookup tables
 * for Class to include a new proto identified by ProtoId
 * and described by Proto.
 * @param Proto floating-pt proto to be added to proto pruner
 * @param ProtoId id of proto
 * @param Class integer class that contains desired proto pruner
 * @param debug debug flag
 * @note Globals: none
 * @return none
 * @note Exceptions: none
 * @note History: Fri Feb  8 13:07:19 1991, DSJ, Created.
 */
void AddProtoToProtoPruner(PROTO Proto, int ProtoId,
                           INT_CLASS Class, bool debug) {
  FLOAT32 Angle, X, Y, Length;
  FLOAT32 Pad;
  int Index;
  PROTO_SET ProtoSet;

  if (ProtoId >= Class->NumProtos)
    cprintf("AddProtoToProtoPruner:assert failed: %d < %d",
            ProtoId, Class->NumProtos);
  assert(ProtoId < Class->NumProtos);

  Index = IndexForProto (ProtoId);
  ProtoSet = Class->ProtoSets[SetForProto (ProtoId)];

  Angle = Proto->Angle;
#ifndef _WIN32
  assert(!isnan(Angle));
#endif

  FillPPCircularBits (ProtoSet->ProtoPruner[PRUNER_ANGLE], Index,
                      Angle + ANGLE_SHIFT, classify_pp_angle_pad / 360.0,
                      debug);

  Angle *= 2.0 * PI;
  Length = Proto->Length;

  X = Proto->X + X_SHIFT;
  Pad = MAX (fabs (cos (Angle)) * (Length / 2.0 +
                                   classify_pp_end_pad *
                                   GetPicoFeatureLength ()),
             fabs (sin (Angle)) * (classify_pp_side_pad *
                                   GetPicoFeatureLength ()));

  FillPPLinearBits(ProtoSet->ProtoPruner[PRUNER_X], Index, X, Pad, debug);

  Y = Proto->Y + Y_SHIFT;
  Pad = MAX (fabs (sin (Angle)) * (Length / 2.0 +
                                   classify_pp_end_pad *
                                   GetPicoFeatureLength ()),
             fabs (cos (Angle)) * (classify_pp_side_pad *
                                   GetPicoFeatureLength ()));

  FillPPLinearBits(ProtoSet->ProtoPruner[PRUNER_Y], Index, Y, Pad, debug);
}                                /* AddProtoToProtoPruner */

/**
 * Returns a quantized bucket for the given param shifted by offset,
 * notionally (param + offset) * num_buckets, but clipped and casted to the
 * appropriate type.
 */
uinT8 Bucket8For(FLOAT32 param, FLOAT32 offset, int num_buckets) {
  int bucket = IntCastRounded(MapParam(param, offset, num_buckets));
  return static_cast<uinT8>(ClipToRange(bucket, 0, num_buckets - 1));
}
uinT16 Bucket16For(FLOAT32 param, FLOAT32 offset, int num_buckets) {
  int bucket = IntCastRounded(MapParam(param, offset, num_buckets));
  return static_cast<uinT16>(ClipToRange(bucket, 0, num_buckets - 1));
}

/**
 * Returns a quantized bucket for the given circular param shifted by offset,
 * notionally (param + offset) * num_buckets, but modded and casted to the
 * appropriate type.
 */
uinT8 CircBucketFor(FLOAT32 param, FLOAT32 offset, int num_buckets) {
  int bucket = IntCastRounded(MapParam(param, offset, num_buckets));
  return static_cast<uinT8>(Modulo(bucket, num_buckets));
}                                /* CircBucketFor */


#ifndef GRAPHICS_DISABLED
/**
 * This routine clears the global feature and proto
 * display lists.
 *
 * Globals:
 * - FeatureShapes display list for features
 * - ProtoShapes display list for protos
 * @return none
 * @note Exceptions: none
 * @note History: Thu Mar 21 15:40:19 1991, DSJ, Created.
 */
void UpdateMatchDisplay() {
  if (IntMatchWindow != NULL)
    IntMatchWindow->Update();
}                                /* ClearMatchDisplay */
#endif

/**
 * This operation updates the config vectors of all protos
 * in Class to indicate that the protos with 1's in Config
 * belong to a new configuration identified by ConfigId.
 * It is assumed that the length of the Config bit vector is
 * equal to the number of protos in Class.
 * @param Config    config to be added to class
 * @param ConfigId  id to be used for new config
 * @param Class   class to add new config to
 * @return none
 * @note Globals: none
 * @note Exceptions: none
 * @note History: Mon Feb 11 14:57:31 1991, DSJ, Created.
 */
void ConvertConfig(BIT_VECTOR Config, int ConfigId, INT_CLASS Class) {
  int ProtoId;
  INT_PROTO Proto;
  int TotalLength;

  for (ProtoId = 0, TotalLength = 0;
    ProtoId < Class->NumProtos; ProtoId++) {
    if (test_bit(Config, ProtoId)) {
      Proto = ProtoForProtoId(Class, ProtoId);
      SET_BIT(Proto->Configs, ConfigId);
      TotalLength += Class->ProtoLengths[ProtoId];
    }
  }
  Class->ConfigLengths[ConfigId] = TotalLength;
}                                /* ConvertConfig */


namespace tesseract {
/**
 * This routine converts Proto to integer format and
 * installs it as ProtoId in Class.
 * @param Proto floating-pt proto to be converted to integer format
 * @param ProtoId id of proto
 * @param Class integer class to add converted proto to
 * @return none
 * @note Globals: none
 * @note Exceptions: none
 * @note History: Fri Feb  8 11:22:43 1991, DSJ, Created.
 */
void Classify::ConvertProto(PROTO Proto, int ProtoId, INT_CLASS Class) {
  INT_PROTO P;
  FLOAT32 Param;

  assert(ProtoId < Class->NumProtos);

  P = ProtoForProtoId(Class, ProtoId);

  Param = Proto->A * 128;
  P->A = TruncateParam(Param, -128, 127, NULL);

  Param = -Proto->B * 256;
  P->B = TruncateParam(Param, 0, 255, NULL);

  Param = Proto->C * 128;
  P->C = TruncateParam(Param, -128, 127, NULL);

  Param = Proto->Angle * 256;
  if (Param < 0 || Param >= 256)
    P->Angle = 0;
  else
    P->Angle = (uinT8) Param;

  /* round proto length to nearest integer number of pico-features */
  Param = (Proto->Length / GetPicoFeatureLength()) + 0.5;
  Class->ProtoLengths[ProtoId] = TruncateParam(Param, 1, 255, NULL);
  if (classify_learning_debug_level >= 2)
    cprintf("Converted ffeat to (A=%d,B=%d,C=%d,L=%d)",
            P->A, P->B, P->C, Class->ProtoLengths[ProtoId]);
}                                /* ConvertProto */

/**
 * This routine converts from the old floating point format
 * to the new integer format.
 * @param FloatProtos prototypes in old floating pt format
 * @param target_unicharset the UNICHARSET to use
 * @return New set of training templates in integer format.
 * @note Globals: none
 * @note Exceptions: none
 * @note History: Thu Feb  7 14:40:42 1991, DSJ, Created.
 */
INT_TEMPLATES Classify::CreateIntTemplates(CLASSES FloatProtos,
                                           const UNICHARSET&
                                           target_unicharset) {
  INT_TEMPLATES IntTemplates;
  CLASS_TYPE FClass;
  INT_CLASS IClass;
  int ClassId;
  int ProtoId;
  int ConfigId;

  IntTemplates = NewIntTemplates();

  for (ClassId = 0; ClassId < target_unicharset.size(); ClassId++) {
    FClass = &(FloatProtos[ClassId]);
    if (FClass->NumProtos == 0 && FClass->NumConfigs == 0 &&
        strcmp(target_unicharset.id_to_unichar(ClassId), " ") != 0) {
      cprintf("Warning: no protos/configs for %s in CreateIntTemplates()\n",
              target_unicharset.id_to_unichar(ClassId));
    }
    assert(UnusedClassIdIn(IntTemplates, ClassId));
    IClass = NewIntClass(FClass->NumProtos, FClass->NumConfigs);
    FontSet fs;
    fs.size = FClass->font_set.size();
    fs.configs = new int[fs.size];
    for (int i = 0; i < fs.size; ++i) {
      fs.configs[i] = FClass->font_set.get(i);
    }
    if (this->fontset_table_.contains(fs)) {
      IClass->font_set_id = this->fontset_table_.get_id(fs);
      delete[] fs.configs;
    } else {
      IClass->font_set_id = this->fontset_table_.push_back(fs);
    }
    AddIntClass(IntTemplates, ClassId, IClass);

    for (ProtoId = 0; ProtoId < FClass->NumProtos; ProtoId++) {
      AddIntProto(IClass);
      ConvertProto(ProtoIn(FClass, ProtoId), ProtoId, IClass);
      AddProtoToProtoPruner(ProtoIn(FClass, ProtoId), ProtoId, IClass,
                            classify_learning_debug_level >= 2);
      AddProtoToClassPruner(ProtoIn(FClass, ProtoId), ClassId, IntTemplates);
    }

    for (ConfigId = 0; ConfigId < FClass->NumConfigs; ConfigId++) {
      AddIntConfig(IClass);
      ConvertConfig(FClass->Configurations[ConfigId], ConfigId, IClass);
    }
  }
  return (IntTemplates);
}                                /* CreateIntTemplates */
}  // namespace tesseract


#ifndef GRAPHICS_DISABLED
/**
 * This routine renders the specified feature into a
 * global display list.
 *
 * Globals:
 * - FeatureShapes global display list for features
 * @param Feature   pico-feature to be displayed
 * @param Evidence  best evidence for this feature (0-1)
 * @return none
 * @note Exceptions: none
 * @note History: Thu Mar 21 14:45:04 1991, DSJ, Created.
 */
void DisplayIntFeature(const INT_FEATURE_STRUCT *Feature, FLOAT32 Evidence) {
  ScrollView::Color color = GetMatchColorFor(Evidence);
  RenderIntFeature(IntMatchWindow, Feature, color);
  if (FeatureDisplayWindow) {
    RenderIntFeature(FeatureDisplayWindow, Feature, color);
  }
}                                /* DisplayIntFeature */

/**
 * This routine renders the specified proto into a
 * global display list.
 *
 * Globals:
 * - ProtoShapes global display list for protos
 * @param Class   class to take proto from
 * @param ProtoId   id of proto in Class to be displayed
 * @param Evidence  total evidence for proto (0-1)
 * @return none
 * @note Exceptions: none
 * @note History: Thu Mar 21 14:45:04 1991, DSJ, Created.
 */
void DisplayIntProto(INT_CLASS Class, PROTO_ID ProtoId, FLOAT32 Evidence) {
  ScrollView::Color color = GetMatchColorFor(Evidence);
  RenderIntProto(IntMatchWindow, Class, ProtoId, color);
  if (ProtoDisplayWindow) {
    RenderIntProto(ProtoDisplayWindow, Class, ProtoId, color);
  }
}                                /* DisplayIntProto */
#endif

/**
 * This routine creates a new integer class data structure
 * and returns it.  Sufficient space is allocated
 * to handle the specified number of protos and configs.
 * @param MaxNumProtos  number of protos to allocate space for
 * @param MaxNumConfigs number of configs to allocate space for
 * @return New class created.
 * @note Globals: none
 * @note Exceptions: none
 * @note History: Fri Feb  8 10:51:23 1991, DSJ, Created.
 */
INT_CLASS NewIntClass(int MaxNumProtos, int MaxNumConfigs) {
  INT_CLASS Class;
  PROTO_SET ProtoSet;
  int i;

  assert(MaxNumConfigs <= MAX_NUM_CONFIGS);

  Class = (INT_CLASS) Emalloc(sizeof(INT_CLASS_STRUCT));
  Class->NumProtoSets = ((MaxNumProtos + PROTOS_PER_PROTO_SET - 1) /
                            PROTOS_PER_PROTO_SET);

  assert(Class->NumProtoSets <= MAX_NUM_PROTO_SETS);

  Class->NumProtos = 0;
  Class->NumConfigs = 0;

  for (i = 0; i < Class->NumProtoSets; i++) {
    /* allocate space for a proto set, install in class, and initialize */
    ProtoSet = (PROTO_SET) Emalloc(sizeof(PROTO_SET_STRUCT));
    memset(ProtoSet, 0, sizeof(*ProtoSet));
    Class->ProtoSets[i] = ProtoSet;

    /* allocate space for the proto lengths and install in class */
  }
  if (MaxNumIntProtosIn (Class) > 0) {
    Class->ProtoLengths =
      (uinT8 *)Emalloc(MaxNumIntProtosIn (Class) * sizeof (uinT8));
    memset(Class->ProtoLengths, 0,
           MaxNumIntProtosIn(Class) * sizeof(*Class->ProtoLengths));
  } else {
    Class->ProtoLengths = NULL;
  }
  memset(Class->ConfigLengths, 0, sizeof(Class->ConfigLengths));

  return (Class);

}                                /* NewIntClass */


void free_int_class(INT_CLASS int_class) {
  int i;

  for (i = 0; i < int_class->NumProtoSets; i++) {
    Efree (int_class->ProtoSets[i]);
  }
  if (int_class->ProtoLengths != NULL) {
    Efree (int_class->ProtoLengths);
  }
  Efree(int_class);
}

/**
 * This routine allocates a new set of integer templates
 * initialized to hold 0 classes.
 * @return The integer templates created.
 * @note Globals: none
 * @note Exceptions: none
 * @note History: Fri Feb  8 08:38:51 1991, DSJ, Created.
 */
INT_TEMPLATES NewIntTemplates() {
  INT_TEMPLATES T;
  int i;

  T = (INT_TEMPLATES) Emalloc (sizeof (INT_TEMPLATES_STRUCT));
  T->NumClasses = 0;
  T->NumClassPruners = 0;

  for (i = 0; i < MAX_NUM_CLASSES; i++)
    ClassForClassId (T, i) = NULL;

  return (T);
}                                /* NewIntTemplates */


/*---------------------------------------------------------------------------*/
void free_int_templates(INT_TEMPLATES templates) {
  int i;

  for (i = 0; i < templates->NumClasses; i++)
    free_int_class(templates->Class[i]);
  for (i = 0; i < templates->NumClassPruners; i++)
    delete templates->ClassPruners[i];
  Efree(templates);
}


namespace tesseract {
/**
 * This routine reads a set of integer templates from
 * File.  File must already be open and must be in the
 * correct binary format.
 * @param  File    open file to read templates from
 * @return Pointer to integer templates read from File.
 * @note Globals: none
 * @note Exceptions: none
 * @note History: Wed Feb 27 11:48:46 1991, DSJ, Created.
 */
INT_TEMPLATES Classify::ReadIntTemplates(FILE *File) {
  int i, j, w, x, y, z;
  BOOL8 swap;
  int nread;
  int unicharset_size;
  int version_id = 0;
  INT_TEMPLATES Templates;
  CLASS_PRUNER_STRUCT* Pruner;
  INT_CLASS Class;
  uinT8 *Lengths;
  PROTO_SET ProtoSet;

  /* variables for conversion from older inttemp formats */
  int b, bit_number, last_cp_bit_number, new_b, new_i, new_w;
  CLASS_ID class_id, max_class_id;
  inT16 *IndexFor = new inT16[MAX_NUM_CLASSES];
  CLASS_ID *ClassIdFor = new CLASS_ID[MAX_NUM_CLASSES];
  CLASS_PRUNER_STRUCT **TempClassPruner =
      new CLASS_PRUNER_STRUCT*[MAX_NUM_CLASS_PRUNERS];
  uinT32 SetBitsForMask =           // word with NUM_BITS_PER_CLASS
    (1 << NUM_BITS_PER_CLASS) - 1;  // set starting at bit 0
  uinT32 Mask, NewMask, ClassBits;
  int MaxNumConfigs = MAX_NUM_CONFIGS;
  int WerdsPerConfigVec = WERDS_PER_CONFIG_VEC;

  /* first read the high level template struct */
  Templates = NewIntTemplates();
  // Read Templates in parts for 64 bit compatibility.
  if (fread(&unicharset_size, sizeof(int), 1, File) != 1)
    cprintf("Bad read of inttemp!\n");
  if (fread(&Templates->NumClasses,
            sizeof(Templates->NumClasses), 1, File) != 1 ||
      fread(&Templates->NumClassPruners,
            sizeof(Templates->NumClassPruners), 1, File) != 1)
    cprintf("Bad read of inttemp!\n");
  // Swap status is determined automatically.
  swap = Templates->NumClassPruners < 0 ||
    Templates->NumClassPruners > MAX_NUM_CLASS_PRUNERS;
  if (swap) {
    Reverse32(&Templates->NumClassPruners);
    Reverse32(&Templates->NumClasses);
    Reverse32(&unicharset_size);
  }
  if (Templates->NumClasses < 0) {
    // This file has a version id!
    version_id = -Templates->NumClasses;
    if (fread(&Templates->NumClasses, sizeof(Templates->NumClasses),
              1, File) != 1)
      cprintf("Bad read of inttemp!\n");
    if (swap)
      Reverse32(&Templates->NumClasses);
  }

  if (version_id < 3) {
    MaxNumConfigs = OLD_MAX_NUM_CONFIGS;
    WerdsPerConfigVec = OLD_WERDS_PER_CONFIG_VEC;
  }

  if (version_id < 2) {
    for (i = 0; i < unicharset_size; ++i) {
      if (fread(&IndexFor[i], sizeof(inT16), 1, File) != 1)
        cprintf("Bad read of inttemp!\n");
    }
    for (i = 0; i < Templates->NumClasses; ++i) {
      if (fread(&ClassIdFor[i], sizeof(CLASS_ID), 1, File) != 1)
        cprintf("Bad read of inttemp!\n");
    }
    if (swap) {
      for (i = 0; i < Templates->NumClasses; i++)
        Reverse16(&IndexFor[i]);
      for (i = 0; i < Templates->NumClasses; i++)
        Reverse32(&ClassIdFor[i]);
    }
  }

  /* then read in the class pruners */
  for (i = 0; i < Templates->NumClassPruners; i++) {
    Pruner = new CLASS_PRUNER_STRUCT;
    if ((nread =
         fread(Pruner, 1, sizeof(CLASS_PRUNER_STRUCT),
                File)) != sizeof(CLASS_PRUNER_STRUCT))
      cprintf("Bad read of inttemp!\n");
    if (swap) {
      for (x = 0; x < NUM_CP_BUCKETS; x++) {
        for (y = 0; y < NUM_CP_BUCKETS; y++) {
          for (z = 0; z < NUM_CP_BUCKETS; z++) {
            for (w = 0; w < WERDS_PER_CP_VECTOR; w++) {
              Reverse32(&Pruner->p[x][y][z][w]);
            }
          }
        }
      }
    }
    if (version_id < 2) {
      TempClassPruner[i] = Pruner;
    } else {
      Templates->ClassPruners[i] = Pruner;
    }
  }

  /* fix class pruners if they came from an old version of inttemp */
  if (version_id < 2) {
    // Allocate enough class pruners to cover all the class ids.
    max_class_id = 0;
    for (i = 0; i < Templates->NumClasses; i++)
      if (ClassIdFor[i] > max_class_id)
        max_class_id = ClassIdFor[i];
    for (i = 0; i <= CPrunerIdFor(max_class_id); i++) {
      Templates->ClassPruners[i] = new CLASS_PRUNER_STRUCT;
      memset(Templates->ClassPruners[i], 0, sizeof(CLASS_PRUNER_STRUCT));
    }
    // Convert class pruners from the old format (indexed by class index)
    // to the new format (indexed by class id).
    last_cp_bit_number = NUM_BITS_PER_CLASS * Templates->NumClasses - 1;
    for (i = 0; i < Templates->NumClassPruners; i++) {
      for (x = 0; x < NUM_CP_BUCKETS; x++)
        for (y = 0; y < NUM_CP_BUCKETS; y++)
          for (z = 0; z < NUM_CP_BUCKETS; z++)
            for (w = 0; w < WERDS_PER_CP_VECTOR; w++) {
              if (TempClassPruner[i]->p[x][y][z][w] == 0)
                continue;
              for (b = 0; b < BITS_PER_WERD; b += NUM_BITS_PER_CLASS) {
                bit_number = i * BITS_PER_CP_VECTOR + w * BITS_PER_WERD + b;
                if (bit_number > last_cp_bit_number)
                  break; // the rest of the bits in this word are not used
                class_id = ClassIdFor[bit_number / NUM_BITS_PER_CLASS];
                // Single out NUM_BITS_PER_CLASS bits relating to class_id.
                Mask = SetBitsForMask << b;
                ClassBits = TempClassPruner[i]->p[x][y][z][w] & Mask;
                // Move these bits to the new position in which they should
                // appear (indexed corresponding to the class_id).
                new_i = CPrunerIdFor(class_id);
                new_w = CPrunerWordIndexFor(class_id);
                new_b = CPrunerBitIndexFor(class_id) * NUM_BITS_PER_CLASS;
                if (new_b > b) {
                  ClassBits <<= (new_b - b);
                } else {
                  ClassBits >>= (b - new_b);
                }
                // Copy bits relating to class_id to the correct position
                // in Templates->ClassPruner.
                NewMask = SetBitsForMask << new_b;
                Templates->ClassPruners[new_i]->p[x][y][z][new_w] &= ~NewMask;
                Templates->ClassPruners[new_i]->p[x][y][z][new_w] |= ClassBits;
              }
            }
    }
    for (i = 0; i < Templates->NumClassPruners; i++) {
      delete TempClassPruner[i];
    }
  }

  /* then read in each class */
  for (i = 0; i < Templates->NumClasses; i++) {
    /* first read in the high level struct for the class */
    Class = (INT_CLASS) Emalloc (sizeof (INT_CLASS_STRUCT));
    if (fread(&Class->NumProtos, sizeof(Class->NumProtos), 1, File) != 1 ||
        fread(&Class->NumProtoSets, sizeof(Class->NumProtoSets), 1, File) != 1 ||
        fread(&Class->NumConfigs, sizeof(Class->NumConfigs), 1, File) != 1)
      cprintf ("Bad read of inttemp!\n");
    if (version_id == 0) {
      // Only version 0 writes 5 pointless pointers to the file.
      for (j = 0; j < 5; ++j) {
        int junk;
        if (fread(&junk, sizeof(junk), 1, File) != 1)
          cprintf ("Bad read of inttemp!\n");
      }
    }
    if (version_id < 4) {
      for (j = 0; j < MaxNumConfigs; ++j) {
        if (fread(&Class->ConfigLengths[j], sizeof(uinT16), 1, File) != 1)
          cprintf ("Bad read of inttemp!\n");
      }
      if (swap) {
        Reverse16(&Class->NumProtos);
        for (j = 0; j < MaxNumConfigs; j++)
          Reverse16(&Class->ConfigLengths[j]);
      }
    } else {
      ASSERT_HOST(Class->NumConfigs < MaxNumConfigs);
      for (j = 0; j < Class->NumConfigs; ++j) {
        if (fread(&Class->ConfigLengths[j], sizeof(uinT16), 1, File) != 1)
          cprintf ("Bad read of inttemp!\n");
      }
      if (swap) {
        Reverse16(&Class->NumProtos);
        for (j = 0; j < MaxNumConfigs; j++)
          Reverse16(&Class->ConfigLengths[j]);
      }
    }
    if (version_id < 2) {
      ClassForClassId (Templates, ClassIdFor[i]) = Class;
    } else {
      ClassForClassId (Templates, i) = Class;
    }

    /* then read in the proto lengths */
    Lengths = NULL;
    if (MaxNumIntProtosIn (Class) > 0) {
      Lengths = (uinT8 *)Emalloc(sizeof(uinT8) * MaxNumIntProtosIn(Class));
      if ((nread =
           fread((char *)Lengths, sizeof(uinT8),
                 MaxNumIntProtosIn(Class), File)) != MaxNumIntProtosIn (Class))
        cprintf ("Bad read of inttemp!\n");
    }
    Class->ProtoLengths = Lengths;

    /* then read in the proto sets */
    for (j = 0; j < Class->NumProtoSets; j++) {
      ProtoSet = (PROTO_SET)Emalloc(sizeof(PROTO_SET_STRUCT));
      if (version_id < 3) {
        if ((nread =
             fread((char *) &ProtoSet->ProtoPruner, 1,
                    sizeof(PROTO_PRUNER), File)) != sizeof(PROTO_PRUNER))
          cprintf("Bad read of inttemp!\n");
        for (x = 0; x < PROTOS_PER_PROTO_SET; x++) {
          if ((nread = fread((char *) &ProtoSet->Protos[x].A, 1,
                             sizeof(inT8), File)) != sizeof(inT8) ||
              (nread = fread((char *) &ProtoSet->Protos[x].B, 1,
                             sizeof(uinT8), File)) != sizeof(uinT8) ||
              (nread = fread((char *) &ProtoSet->Protos[x].C, 1,
                             sizeof(inT8), File)) != sizeof(inT8) ||
              (nread = fread((char *) &ProtoSet->Protos[x].Angle, 1,
                             sizeof(uinT8), File)) != sizeof(uinT8))
            cprintf("Bad read of inttemp!\n");
          for (y = 0; y < WerdsPerConfigVec; y++)
            if ((nread = fread((char *) &ProtoSet->Protos[x].Configs[y], 1,
                               sizeof(uinT32), File)) != sizeof(uinT32))
              cprintf("Bad read of inttemp!\n");
        }
      } else {
        if ((nread =
             fread((char *) ProtoSet, 1, sizeof(PROTO_SET_STRUCT),
                   File)) != sizeof(PROTO_SET_STRUCT))
          cprintf("Bad read of inttemp!\n");
      }
      if (swap) {
        for (x = 0; x < NUM_PP_PARAMS; x++)
          for (y = 0; y < NUM_PP_BUCKETS; y++)
            for (z = 0; z < WERDS_PER_PP_VECTOR; z++)
              Reverse32(&ProtoSet->ProtoPruner[x][y][z]);
        for (x = 0; x < PROTOS_PER_PROTO_SET; x++)
          for (y = 0; y < WerdsPerConfigVec; y++)
            Reverse32(&ProtoSet->Protos[x].Configs[y]);
      }
      Class->ProtoSets[j] = ProtoSet;
    }
    if (version_id < 4)
      Class->font_set_id = -1;
    else {
      fread(&Class->font_set_id, sizeof(int), 1, File);
      if (swap)
        Reverse32(&Class->font_set_id);
    }
  }

  if (version_id < 2) {
    /* add an empty NULL class with class id 0 */
    assert(UnusedClassIdIn (Templates, 0));
    ClassForClassId (Templates, 0) = NewIntClass (1, 1);
    ClassForClassId (Templates, 0)->font_set_id = -1;
    Templates->NumClasses++;
    /* make sure the classes are contiguous */
    for (i = 0; i < MAX_NUM_CLASSES; i++) {
      if (i < Templates->NumClasses) {
        if (ClassForClassId (Templates, i) == NULL) {
          fprintf(stderr, "Non-contiguous class ids in inttemp\n");
          exit(1);
        }
      } else {
        if (ClassForClassId (Templates, i) != NULL) {
          fprintf(stderr, "Class id %d exceeds NumClassesIn (Templates) %d\n",
                  i, Templates->NumClasses);
          exit(1);
        }
      }
    }
  }
  if (version_id >= 4) {
    this->fontinfo_table_.read(File, NewPermanentTessCallback(read_info), swap);
    if (version_id >= 5) {
      this->fontinfo_table_.read(File,
                                 NewPermanentTessCallback(read_spacing_info),
                                 swap);
    }
    this->fontset_table_.read(File, NewPermanentTessCallback(read_set), swap);
  }

  // Clean up.
  delete[] IndexFor;
  delete[] ClassIdFor;
  delete[] TempClassPruner;

  return (Templates);
}                                /* ReadIntTemplates */


#ifndef GRAPHICS_DISABLED
/**
 * This routine sends the shapes in the global display
 * lists to the match debugger window.
 *
 * Globals:
 * - FeatureShapes display list containing feature matches
 * - ProtoShapes display list containing proto matches
 * @return none
 * @note Exceptions: none
 * @note History: Thu Mar 21 15:47:33 1991, DSJ, Created.
 */
void Classify::ShowMatchDisplay() {
  InitIntMatchWindowIfReqd();
  if (ProtoDisplayWindow) {
    ProtoDisplayWindow->Clear();
  }
  if (FeatureDisplayWindow) {
    FeatureDisplayWindow->Clear();
  }
  ClearFeatureSpaceWindow(
      static_cast<NORM_METHOD>(static_cast<int>(classify_norm_method)),
      IntMatchWindow);
  IntMatchWindow->ZoomToRectangle(INT_MIN_X, INT_MIN_Y,
                                  INT_MAX_X, INT_MAX_Y);
  if (ProtoDisplayWindow) {
    ProtoDisplayWindow->ZoomToRectangle(INT_MIN_X, INT_MIN_Y,
                                        INT_MAX_X, INT_MAX_Y);
  }
  if (FeatureDisplayWindow) {
    FeatureDisplayWindow->ZoomToRectangle(INT_MIN_X, INT_MIN_Y,
                                          INT_MAX_X, INT_MAX_Y);
  }
}                                /* ShowMatchDisplay */

/// Clears the given window and draws the featurespace guides for the
/// appropriate normalization method.
void ClearFeatureSpaceWindow(NORM_METHOD norm_method, ScrollView* window) {
  window->Clear();

  window->Pen(ScrollView::GREY);
  // Draw the feature space limit rectangle.
  window->Rectangle(0, 0, INT_MAX_X, INT_MAX_Y);
  if (norm_method == baseline) {
    window->SetCursor(0, INT_DESCENDER);
    window->DrawTo(INT_MAX_X, INT_DESCENDER);
    window->SetCursor(0, INT_BASELINE);
    window->DrawTo(INT_MAX_X, INT_BASELINE);
    window->SetCursor(0, INT_XHEIGHT);
    window->DrawTo(INT_MAX_X, INT_XHEIGHT);
    window->SetCursor(0, INT_CAPHEIGHT);
    window->DrawTo(INT_MAX_X, INT_CAPHEIGHT);
  } else {
    window->Rectangle(INT_XCENTER - INT_XRADIUS, INT_YCENTER - INT_YRADIUS,
                      INT_XCENTER + INT_XRADIUS, INT_YCENTER + INT_YRADIUS);
  }
}
#endif

/**
 * This routine writes Templates to File.  The format
 * is an efficient binary format.  File must already be open
 * for writing.
 * @param File open file to write templates to
 * @param Templates templates to save into File
 * @param target_unicharset the UNICHARSET to use
 * @return none
 * @note Globals: none
 * @note Exceptions: none
 * @note History: Wed Feb 27 11:48:46 1991, DSJ, Created.
 */
void Classify::WriteIntTemplates(FILE *File, INT_TEMPLATES Templates,
                                 const UNICHARSET& target_unicharset) {
  int i, j;
  INT_CLASS Class;
  int unicharset_size = target_unicharset.size();
  int version_id = -5;  // When negated by the reader -1 becomes +1 etc.

  if (Templates->NumClasses != unicharset_size) {
    cprintf("Warning: executing WriteIntTemplates() with %d classes in"
            " Templates, while target_unicharset size is %d\n",
            Templates->NumClasses, unicharset_size);
  }

  /* first write the high level template struct */
  fwrite(&unicharset_size, sizeof(unicharset_size), 1, File);
  fwrite(&version_id, sizeof(version_id), 1, File);
  fwrite(&Templates->NumClassPruners, sizeof(Templates->NumClassPruners),
         1, File);
  fwrite(&Templates->NumClasses, sizeof(Templates->NumClasses), 1, File);

  /* then write out the class pruners */
  for (i = 0; i < Templates->NumClassPruners; i++)
    fwrite(Templates->ClassPruners[i],
           sizeof(CLASS_PRUNER_STRUCT), 1, File);

  /* then write out each class */
  for (i = 0; i < Templates->NumClasses; i++) {
    Class = Templates->Class[i];

    /* first write out the high level struct for the class */
    fwrite(&Class->NumProtos, sizeof(Class->NumProtos), 1, File);
    fwrite(&Class->NumProtoSets, sizeof(Class->NumProtoSets), 1, File);
    ASSERT_HOST(Class->NumConfigs == this->fontset_table_.get(Class->font_set_id).size);
    fwrite(&Class->NumConfigs, sizeof(Class->NumConfigs), 1, File);
    for (j = 0; j < Class->NumConfigs; ++j) {
      fwrite(&Class->ConfigLengths[j], sizeof(uinT16), 1, File);
    }

    /* then write out the proto lengths */
    if (MaxNumIntProtosIn (Class) > 0) {
      fwrite ((char *) (Class->ProtoLengths), sizeof (uinT8),
              MaxNumIntProtosIn (Class), File);
    }

    /* then write out the proto sets */
    for (j = 0; j < Class->NumProtoSets; j++)
      fwrite ((char *) Class->ProtoSets[j],
              sizeof (PROTO_SET_STRUCT), 1, File);

    /* then write the fonts info */
    fwrite(&Class->font_set_id, sizeof(int), 1, File);
  }

  /* Write the fonts info tables */
  this->fontinfo_table_.write(File, NewPermanentTessCallback(write_info));
  this->fontinfo_table_.write(File,
                              NewPermanentTessCallback(write_spacing_info));
  this->fontset_table_.write(File, NewPermanentTessCallback(write_set));
}                                /* WriteIntTemplates */
} // namespace tesseract


/*-----------------------------------------------------------------------------
              Private Code
-----------------------------------------------------------------------------*/
/**
 * This routine returns the parameter value which
 * corresponds to the beginning of the specified bucket.
 * The bucket number should have been generated using the
 * BucketFor() function with parameters Offset and NumBuckets.
 * @param Bucket    bucket whose start is to be computed
 * @param Offset    offset used to map params to buckets
 * @param NumBuckets  total number of buckets
 * @return Param value corresponding to start position of Bucket.
 * @note Globals: none
 * @note Exceptions: none
 * @note History: Thu Feb 14 13:24:33 1991, DSJ, Created.
 */
FLOAT32 BucketStart(int Bucket, FLOAT32 Offset, int NumBuckets) {
  return (((FLOAT32) Bucket / NumBuckets) - Offset);

}                                /* BucketStart */

/**
 * This routine returns the parameter value which
 * corresponds to the end of the specified bucket.
 * The bucket number should have been generated using the
 * BucketFor() function with parameters Offset and NumBuckets.
 * @param Bucket    bucket whose end is to be computed
 * @param Offset    offset used to map params to buckets
 * @param NumBuckets  total number of buckets
 * @return Param value corresponding to end position of Bucket.
 * @note Globals: none
 * @note Exceptions: none
 * @note History: Thu Feb 14 13:24:33 1991, DSJ, Created.
 */
FLOAT32 BucketEnd(int Bucket, FLOAT32 Offset, int NumBuckets) {
  return (((FLOAT32) (Bucket + 1) / NumBuckets) - Offset);
}                                /* BucketEnd */

/**
 * This routine fills in the section of a class pruner
 * corresponding to a single x value for a single proto of
 * a class.
 * @param FillSpec  specifies which bits to fill in pruner
 * @param Pruner    class pruner to be filled
 * @param ClassMask indicates which bits to change in each word
 * @param ClassCount  indicates what to change bits to
 * @param WordIndex indicates which word to change
 * @return none
 * @note Globals: none
 * @note Exceptions: none
 * @note History: Tue Feb 19 11:11:29 1991, DSJ, Created.
 */
void DoFill(FILL_SPEC *FillSpec,
            CLASS_PRUNER_STRUCT* Pruner,
            register uinT32 ClassMask,
            register uinT32 ClassCount,
            register uinT32 WordIndex) {
  int X, Y, Angle;
  uinT32 OldWord;

  X = FillSpec->X;
  if (X < 0)
    X = 0;
  if (X >= NUM_CP_BUCKETS)
    X = NUM_CP_BUCKETS - 1;

  if (FillSpec->YStart < 0)
    FillSpec->YStart = 0;
  if (FillSpec->YEnd >= NUM_CP_BUCKETS)
    FillSpec->YEnd = NUM_CP_BUCKETS - 1;

  for (Y = FillSpec->YStart; Y <= FillSpec->YEnd; Y++)
    for (Angle = FillSpec->AngleStart;
         TRUE; CircularIncrement (Angle, NUM_CP_BUCKETS)) {
      OldWord = Pruner->p[X][Y][Angle][WordIndex];
      if (ClassCount > (OldWord & ClassMask)) {
        OldWord &= ~ClassMask;
        OldWord |= ClassCount;
        Pruner->p[X][Y][Angle][WordIndex] = OldWord;
      }
      if (Angle == FillSpec->AngleEnd)
        break;
    }
}                                /* DoFill */

/**
 * Return TRUE if the specified table filler is done, i.e.
 * if it has no more lines to fill.
 * @param Filler    table filler to check if done
 * @return TRUE if no more lines to fill, FALSE otherwise.
 * @note Globals: none
 * @note Exceptions: none
 * @note History: Tue Feb 19 10:08:05 1991, DSJ, Created.
 */
BOOL8 FillerDone(TABLE_FILLER *Filler) {
  FILL_SWITCH *Next;

  Next = &(Filler->Switch[Filler->NextSwitch]);

  if (Filler->X > Next->X && Next->Type == LastSwitch)
    return (TRUE);
  else
    return (FALSE);

}                                /* FillerDone */

/**
 * This routine sets Bit in each bit vector whose
 * bucket lies within the range Center +- Spread.  The fill
 * is done for a circular dimension, i.e. bucket 0 is adjacent
 * to the last bucket.  It is assumed that Center and Spread
 * are expressed in a circular coordinate system whose range
 * is 0 to 1.
 * @param ParamTable  table of bit vectors, one per param bucket
 * @param Bit bit position in vectors to be filled
 * @param Center center of filled area
 * @param Spread spread of filled area
 * @param debug debug flag
 * @return none
 * @note Globals: none
 * @note Exceptions: none
 * @note History: Tue Oct 16 09:26:54 1990, DSJ, Created.
 */
void FillPPCircularBits(uinT32 ParamTable[NUM_PP_BUCKETS][WERDS_PER_PP_VECTOR],
                        int Bit, FLOAT32 Center, FLOAT32 Spread, bool debug) {
  int i, FirstBucket, LastBucket;

  if (Spread > 0.5)
    Spread = 0.5;

  FirstBucket = (int) floor ((Center - Spread) * NUM_PP_BUCKETS);
  if (FirstBucket < 0)
    FirstBucket += NUM_PP_BUCKETS;

  LastBucket = (int) floor ((Center + Spread) * NUM_PP_BUCKETS);
  if (LastBucket >= NUM_PP_BUCKETS)
    LastBucket -= NUM_PP_BUCKETS;
  if (debug) tprintf("Circular fill from %d to %d", FirstBucket, LastBucket);
  for (i = FirstBucket; TRUE; CircularIncrement (i, NUM_PP_BUCKETS)) {
    SET_BIT (ParamTable[i], Bit);

    /* exit loop after we have set the bit for the last bucket */
    if (i == LastBucket)
      break;
  }

}                                /* FillPPCircularBits */

/**
 * This routine sets Bit in each bit vector whose
 * bucket lies within the range Center +- Spread.  The fill
 * is done for a linear dimension, i.e. there is no wrap-around
 * for this dimension.  It is assumed that Center and Spread
 * are expressed in a linear coordinate system whose range
 * is approximately 0 to 1.  Values outside this range will
 * be clipped.
 * @param ParamTable table of bit vectors, one per param bucket
 * @param Bit bit number being filled
 * @param Center center of filled area
 * @param Spread spread of filled area
 * @param debug debug flag
 * @return none
 * @note Globals: none
 * @note Exceptions: none
 * @note History: Tue Oct 16 09:26:54 1990, DSJ, Created.
 */
void FillPPLinearBits(uinT32 ParamTable[NUM_PP_BUCKETS][WERDS_PER_PP_VECTOR],
                      int Bit, FLOAT32 Center, FLOAT32 Spread, bool debug) {
  int i, FirstBucket, LastBucket;

  FirstBucket = (int) floor ((Center - Spread) * NUM_PP_BUCKETS);
  if (FirstBucket < 0)
    FirstBucket = 0;

  LastBucket = (int) floor ((Center + Spread) * NUM_PP_BUCKETS);
  if (LastBucket >= NUM_PP_BUCKETS)
    LastBucket = NUM_PP_BUCKETS - 1;

  if (debug) tprintf("Linear fill from %d to %d", FirstBucket, LastBucket);
  for (i = FirstBucket; i <= LastBucket; i++)
    SET_BIT (ParamTable[i], Bit);

}                                /* FillPPLinearBits */


/*---------------------------------------------------------------------------*/
#ifndef GRAPHICS_DISABLED
namespace tesseract {
/**
 * This routine prompts the user with Prompt and waits
 * for the user to enter something in the debug window.
 * @param Prompt prompt to print while waiting for input from window
 * @param adaptive_on
 * @param pretrained_on
 * @param shape_id
 * @return Character entered in the debug window.
 * @note Globals: none
 * @note Exceptions: none
 * @note History: Thu Mar 21 16:55:13 1991, DSJ, Created.
 */
CLASS_ID Classify::GetClassToDebug(const char *Prompt, bool* adaptive_on,
                                   bool* pretrained_on, int* shape_id) {
  tprintf("%s\n", Prompt);
  SVEvent* ev;
  SVEventType ev_type;
  int unichar_id = INVALID_UNICHAR_ID;
  // Wait until a click or popup event.
  do {
    ev = IntMatchWindow->AwaitEvent(SVET_ANY);
    ev_type = ev->type;
    if (ev_type == SVET_POPUP) {
      if (ev->command_id == IDA_SHAPE_INDEX) {
        if (shape_table_ != NULL) {
          *shape_id = atoi(ev->parameter);
          *adaptive_on = false;
          *pretrained_on = true;
          if (*shape_id >= 0 && *shape_id < shape_table_->NumShapes()) {
            int font_id;
            shape_table_->GetFirstUnicharAndFont(*shape_id, &unichar_id,
                                                 &font_id);
            tprintf("Shape %d, first unichar=%d, font=%d\n",
                    *shape_id, unichar_id, font_id);
            return unichar_id;
          }
          tprintf("Shape index '%s' not found in shape table\n", ev->parameter);
        } else {
          tprintf("No shape table loaded!\n");
        }
      } else {
        if (unicharset.contains_unichar(ev->parameter)) {
          unichar_id = unicharset.unichar_to_id(ev->parameter);
          if (ev->command_id == IDA_ADAPTIVE) {
            *adaptive_on = true;
            *pretrained_on = false;
            *shape_id = -1;
          } else if (ev->command_id == IDA_STATIC) {
            *adaptive_on = false;
            *pretrained_on = true;
          } else {
            *adaptive_on = true;
            *pretrained_on = true;
          }
          if (ev->command_id == IDA_ADAPTIVE || shape_table_ == NULL) {
            *shape_id = -1;
            return unichar_id;
          }
          for (int s = 0; s < shape_table_->NumShapes(); ++s) {
            if (shape_table_->GetShape(s).ContainsUnichar(unichar_id)) {
              tprintf("%s\n", shape_table_->DebugStr(s).string());
            }
          }
        } else {
          tprintf("Char class '%s' not found in unicharset",
                  ev->parameter);
        }
      }
    }
    delete ev;
  } while (ev_type != SVET_CLICK);
  return 0;
}                                /* GetClassToDebug */

}  // namespace tesseract
#endif

/**
 * This routine copies the appropriate global pad variables
 * into EndPad, SidePad, and AnglePad.  This is a kludge used
 * to get around the fact that global control variables cannot
 * be arrays.  If the specified level is illegal, the tightest
 * possible pads are returned.
 * @param Level   "tightness" level to return pads for
 * @param EndPad    place to put end pad for Level
 * @param SidePad   place to put side pad for Level
 * @param AnglePad  place to put angle pad for Level
 * @return none (results are returned in EndPad, SidePad, and AnglePad.
 * @note Globals: none
 * @note Exceptions: none
 * @note History: Thu Feb 14 08:26:49 1991, DSJ, Created.
 */
void GetCPPadsForLevel(int Level,
                       FLOAT32 *EndPad,
                       FLOAT32 *SidePad,
                       FLOAT32 *AnglePad) {
  switch (Level) {
    case 0:
      *EndPad = classify_cp_end_pad_loose * GetPicoFeatureLength ();
      *SidePad = classify_cp_side_pad_loose * GetPicoFeatureLength ();
      *AnglePad = classify_cp_angle_pad_loose / 360.0;
      break;

    case 1:
      *EndPad = classify_cp_end_pad_medium * GetPicoFeatureLength ();
      *SidePad = classify_cp_side_pad_medium * GetPicoFeatureLength ();
      *AnglePad = classify_cp_angle_pad_medium / 360.0;
      break;

    case 2:
      *EndPad = classify_cp_end_pad_tight * GetPicoFeatureLength ();
      *SidePad = classify_cp_side_pad_tight * GetPicoFeatureLength ();
      *AnglePad = classify_cp_angle_pad_tight / 360.0;
      break;

    default:
      *EndPad = classify_cp_end_pad_tight * GetPicoFeatureLength ();
      *SidePad = classify_cp_side_pad_tight * GetPicoFeatureLength ();
      *AnglePad = classify_cp_angle_pad_tight / 360.0;
      break;
  }
  if (*AnglePad > 0.5)
    *AnglePad = 0.5;

}                                /* GetCPPadsForLevel */

/**
 * @param Evidence  evidence value to return color for
 * @return Color which corresponds to specified Evidence value.
 * @note Globals: none
 * @note Exceptions: none
 * @note History: Thu Mar 21 15:24:52 1991, DSJ, Created.
 */
ScrollView::Color GetMatchColorFor(FLOAT32 Evidence) {
  assert (Evidence >= 0.0);
  assert (Evidence <= 1.0);

  if (Evidence >= 0.90)
    return ScrollView::WHITE;
  else if (Evidence >= 0.75)
    return ScrollView::GREEN;
  else if (Evidence >= 0.50)
    return ScrollView::RED;
  else
    return ScrollView::BLUE;
}                                /* GetMatchColorFor */

/**
 * This routine returns (in Fill) the specification of
 * the next line to be filled from Filler.  FillerDone() should
 * always be called before GetNextFill() to ensure that we
 * do not run past the end of the fill table.
 * @param Filler    filler to get next fill spec from
 * @param Fill    place to put spec for next fill
 * @return none (results are returned in Fill)
 * @note Globals: none
 * @note Exceptions: none
 * @note History: Tue Feb 19 10:17:42 1991, DSJ, Created.
 */
void GetNextFill(TABLE_FILLER *Filler, FILL_SPEC *Fill) {
  FILL_SWITCH *Next;

  /* compute the fill assuming no switches will be encountered */
  Fill->AngleStart = Filler->AngleStart;
  Fill->AngleEnd = Filler->AngleEnd;
  Fill->X = Filler->X;
  Fill->YStart = Filler->YStart >> 8;
  Fill->YEnd = Filler->YEnd >> 8;

  /* update the fill info and the filler for ALL switches at this X value */
  Next = &(Filler->Switch[Filler->NextSwitch]);
  while (Filler->X >= Next->X) {
    Fill->X = Filler->X = Next->X;
    if (Next->Type == StartSwitch) {
      Fill->YStart = Next->Y;
      Filler->StartDelta = Next->Delta;
      Filler->YStart = Next->YInit;
    }
    else if (Next->Type == EndSwitch) {
      Fill->YEnd = Next->Y;
      Filler->EndDelta = Next->Delta;
      Filler->YEnd = Next->YInit;
    }
    else {                       /* Type must be LastSwitch */
      break;
    }
    Filler->NextSwitch++;
    Next = &(Filler->Switch[Filler->NextSwitch]);
  }

  /* prepare the filler for the next call to this routine */
  Filler->X++;
  Filler->YStart += Filler->StartDelta;
  Filler->YEnd += Filler->EndDelta;

}                                /* GetNextFill */

/**
 * This routine computes a data structure (Filler)
 * which can be used to fill in a rectangle surrounding
 * the specified Proto.
 *
 * @param EndPad, SidePad, AnglePad padding to add to proto
 * @param Proto       proto to create a filler for
 * @param Filler        place to put table filler
 *
 * @return none (results are returned in Filler)
 * @note Globals: none
 * @note Exceptions: none
 * @note History: Thu Feb 14 09:27:05 1991, DSJ, Created.
 */
void InitTableFiller (FLOAT32 EndPad, FLOAT32 SidePad,
                      FLOAT32 AnglePad, PROTO Proto, TABLE_FILLER * Filler)
#define XS          X_SHIFT
#define YS          Y_SHIFT
#define AS          ANGLE_SHIFT
#define NB          NUM_CP_BUCKETS
{
  FLOAT32 Angle;
  FLOAT32 X, Y, HalfLength;
  FLOAT32 Cos, Sin;
  FLOAT32 XAdjust, YAdjust;
  FPOINT Start, Switch1, Switch2, End;
  int S1 = 0;
  int S2 = 1;

  Angle = Proto->Angle;
  X = Proto->X;
  Y = Proto->Y;
  HalfLength = Proto->Length / 2.0;

  Filler->AngleStart = CircBucketFor(Angle - AnglePad, AS, NB);
  Filler->AngleEnd = CircBucketFor(Angle + AnglePad, AS, NB);
  Filler->NextSwitch = 0;

  if (fabs (Angle - 0.0) < HV_TOLERANCE || fabs (Angle - 0.5) < HV_TOLERANCE) {
    /* horizontal proto - handle as special case */
    Filler->X = Bucket8For(X - HalfLength - EndPad, XS, NB);
    Filler->YStart = Bucket16For(Y - SidePad, YS, NB * 256);
    Filler->YEnd = Bucket16For(Y + SidePad, YS, NB * 256);
    Filler->StartDelta = 0;
    Filler->EndDelta = 0;
    Filler->Switch[0].Type = LastSwitch;
    Filler->Switch[0].X = Bucket8For(X + HalfLength + EndPad, XS, NB);
  } else if (fabs(Angle - 0.25) < HV_TOLERANCE ||
           fabs(Angle - 0.75) < HV_TOLERANCE) {
    /* vertical proto - handle as special case */
    Filler->X = Bucket8For(X - SidePad, XS, NB);
    Filler->YStart = Bucket16For(Y - HalfLength - EndPad, YS, NB * 256);
    Filler->YEnd = Bucket16For(Y + HalfLength + EndPad, YS, NB * 256);
    Filler->StartDelta = 0;
    Filler->EndDelta = 0;
    Filler->Switch[0].Type = LastSwitch;
    Filler->Switch[0].X = Bucket8For(X + SidePad, XS, NB);
  } else {
    /* diagonal proto */

    if ((Angle > 0.0 && Angle < 0.25) || (Angle > 0.5 && Angle < 0.75)) {
      /* rising diagonal proto */
      Angle *= 2.0 * PI;
      Cos = fabs(cos(Angle));
      Sin = fabs(sin(Angle));

      /* compute the positions of the corners of the acceptance region */
      Start.x = X - (HalfLength + EndPad) * Cos - SidePad * Sin;
      Start.y = Y - (HalfLength + EndPad) * Sin + SidePad * Cos;
      End.x = 2.0 * X - Start.x;
      End.y = 2.0 * Y - Start.y;
      Switch1.x = X - (HalfLength + EndPad) * Cos + SidePad * Sin;
      Switch1.y = Y - (HalfLength + EndPad) * Sin - SidePad * Cos;
      Switch2.x = 2.0 * X - Switch1.x;
      Switch2.y = 2.0 * Y - Switch1.y;

      if (Switch1.x > Switch2.x) {
        S1 = 1;
        S2 = 0;
      }

      /* translate into bucket positions and deltas */
      Filler->X = Bucket8For(Start.x, XS, NB);
      Filler->StartDelta = -(inT16) ((Cos / Sin) * 256);
      Filler->EndDelta = (inT16) ((Sin / Cos) * 256);

      XAdjust = BucketEnd(Filler->X, XS, NB) - Start.x;
      YAdjust = XAdjust * Cos / Sin;
      Filler->YStart = Bucket16For(Start.y - YAdjust, YS, NB * 256);
      YAdjust = XAdjust * Sin / Cos;
      Filler->YEnd = Bucket16For(Start.y + YAdjust, YS, NB * 256);

      Filler->Switch[S1].Type = StartSwitch;
      Filler->Switch[S1].X = Bucket8For(Switch1.x, XS, NB);
      Filler->Switch[S1].Y = Bucket8For(Switch1.y, YS, NB);
      XAdjust = Switch1.x - BucketStart(Filler->Switch[S1].X, XS, NB);
      YAdjust = XAdjust * Sin / Cos;
      Filler->Switch[S1].YInit = Bucket16For(Switch1.y - YAdjust, YS, NB * 256);
      Filler->Switch[S1].Delta = Filler->EndDelta;

      Filler->Switch[S2].Type = EndSwitch;
      Filler->Switch[S2].X = Bucket8For(Switch2.x, XS, NB);
      Filler->Switch[S2].Y = Bucket8For(Switch2.y, YS, NB);
      XAdjust = Switch2.x - BucketStart(Filler->Switch[S2].X, XS, NB);
      YAdjust = XAdjust * Cos / Sin;
      Filler->Switch[S2].YInit = Bucket16For(Switch2.y + YAdjust, YS, NB * 256);
      Filler->Switch[S2].Delta = Filler->StartDelta;

      Filler->Switch[2].Type = LastSwitch;
      Filler->Switch[2].X = Bucket8For(End.x, XS, NB);
    } else {
      /* falling diagonal proto */
      Angle *= 2.0 * PI;
      Cos = fabs(cos(Angle));
      Sin = fabs(sin(Angle));

      /* compute the positions of the corners of the acceptance region */
      Start.x = X - (HalfLength + EndPad) * Cos - SidePad * Sin;
      Start.y = Y + (HalfLength + EndPad) * Sin - SidePad * Cos;
      End.x = 2.0 * X - Start.x;
      End.y = 2.0 * Y - Start.y;
      Switch1.x = X - (HalfLength + EndPad) * Cos + SidePad * Sin;
      Switch1.y = Y + (HalfLength + EndPad) * Sin + SidePad * Cos;
      Switch2.x = 2.0 * X - Switch1.x;
      Switch2.y = 2.0 * Y - Switch1.y;

      if (Switch1.x > Switch2.x) {
        S1 = 1;
        S2 = 0;
      }

      /* translate into bucket positions and deltas */
      Filler->X = Bucket8For(Start.x, XS, NB);
      Filler->StartDelta = static_cast<inT16>(ClipToRange<int>(
          -IntCastRounded((Sin / Cos) * 256), MIN_INT16, MAX_INT16));
      Filler->EndDelta = static_cast<inT16>(ClipToRange<int>(
          IntCastRounded((Cos / Sin) * 256), MIN_INT16, MAX_INT16));

      XAdjust = BucketEnd(Filler->X, XS, NB) - Start.x;
      YAdjust = XAdjust * Sin / Cos;
      Filler->YStart = Bucket16For(Start.y - YAdjust, YS, NB * 256);
      YAdjust = XAdjust * Cos / Sin;
      Filler->YEnd = Bucket16For(Start.y + YAdjust, YS, NB * 256);

      Filler->Switch[S1].Type = EndSwitch;
      Filler->Switch[S1].X = Bucket8For(Switch1.x, XS, NB);
      Filler->Switch[S1].Y = Bucket8For(Switch1.y, YS, NB);
      XAdjust = Switch1.x - BucketStart(Filler->Switch[S1].X, XS, NB);
      YAdjust = XAdjust * Sin / Cos;
      Filler->Switch[S1].YInit = Bucket16For(Switch1.y + YAdjust, YS, NB * 256);
      Filler->Switch[S1].Delta = Filler->StartDelta;

      Filler->Switch[S2].Type = StartSwitch;
      Filler->Switch[S2].X = Bucket8For(Switch2.x, XS, NB);
      Filler->Switch[S2].Y = Bucket8For(Switch2.y, YS, NB);
      XAdjust = Switch2.x - BucketStart(Filler->Switch[S2].X, XS, NB);
      YAdjust = XAdjust * Cos / Sin;
      Filler->Switch[S2].YInit = Bucket16For(Switch2.y - YAdjust, YS, NB * 256);
      Filler->Switch[S2].Delta = Filler->EndDelta;

      Filler->Switch[2].Type = LastSwitch;
      Filler->Switch[2].X = Bucket8For(End.x, XS, NB);
    }
  }
}                                /* InitTableFiller */


/*---------------------------------------------------------------------------*/
#ifndef GRAPHICS_DISABLED
/**
 * This routine renders the specified feature into ShapeList.
 * @param window to add feature rendering to
 * @param Feature feature to be rendered
 * @param color color to use for feature rendering
 * @return New shape list with rendering of Feature added.
 * @note Globals: none
 * @note Exceptions: none
 * @note History: Thu Mar 21 14:57:41 1991, DSJ, Created.
 */
void RenderIntFeature(ScrollView *window, const INT_FEATURE_STRUCT* Feature,
                      ScrollView::Color color) {
  FLOAT32 X, Y, Dx, Dy, Length;

  window->Pen(color);
  assert(Feature != NULL);
  assert(color != 0);

  X = Feature->X;
  Y = Feature->Y;
  Length = GetPicoFeatureLength() * 0.7 * INT_CHAR_NORM_RANGE;
  // The -PI has no significant effect here, but the value of Theta is computed
  // using BinaryAnglePlusPi in intfx.cpp.
  Dx = (Length / 2.0) * cos((Feature->Theta / 256.0) * 2.0 * PI - PI);
  Dy = (Length / 2.0) * sin((Feature->Theta / 256.0) * 2.0 * PI - PI);

  window->SetCursor(X, Y);
  window->DrawTo(X + Dx, Y + Dy);
}                                /* RenderIntFeature */

/**
 * This routine extracts the parameters of the specified
 * proto from the class description and adds a rendering of
 * the proto onto the ShapeList.
 *
 * @param window ScrollView instance
 * @param Class class that proto is contained in
 * @param ProtoId id of proto to be rendered
 * @param color color to render proto in
 *
 * Globals: none
 *
 * @return New shape list with a rendering of one proto added.
 * @note Exceptions: none
 * @note History: Thu Mar 21 10:21:09 1991, DSJ, Created.
 */
void RenderIntProto(ScrollView *window,
                    INT_CLASS Class,
                    PROTO_ID ProtoId,
                    ScrollView::Color color) {
  PROTO_SET ProtoSet;
  INT_PROTO Proto;
  int ProtoSetIndex;
  int ProtoWordIndex;
  FLOAT32 Length;
  int Xmin, Xmax, Ymin, Ymax;
  FLOAT32 X, Y, Dx, Dy;
  uinT32 ProtoMask;
  int Bucket;

  assert(ProtoId >= 0);
  assert(Class != NULL);
  assert(ProtoId < Class->NumProtos);
  assert(color != 0);
  window->Pen(color);

  ProtoSet = Class->ProtoSets[SetForProto(ProtoId)];
  ProtoSetIndex = IndexForProto(ProtoId);
  Proto = &(ProtoSet->Protos[ProtoSetIndex]);
  Length = (Class->ProtoLengths[ProtoId] *
    GetPicoFeatureLength() * INT_CHAR_NORM_RANGE);
  ProtoMask = PPrunerMaskFor(ProtoId);
  ProtoWordIndex = PPrunerWordIndexFor(ProtoId);

  // find the x and y extent of the proto from the proto pruning table
  Xmin = Ymin = NUM_PP_BUCKETS;
  Xmax = Ymax = 0;
  for (Bucket = 0; Bucket < NUM_PP_BUCKETS; Bucket++) {
    if (ProtoMask & ProtoSet->ProtoPruner[PRUNER_X][Bucket][ProtoWordIndex]) {
      UpdateRange(Bucket, &Xmin, &Xmax);
    }

    if (ProtoMask & ProtoSet->ProtoPruner[PRUNER_Y][Bucket][ProtoWordIndex]) {
      UpdateRange(Bucket, &Ymin, &Ymax);
    }
  }
  X = (Xmin + Xmax + 1) / 2.0 * PROTO_PRUNER_SCALE;
  Y = (Ymin + Ymax + 1) / 2.0 * PROTO_PRUNER_SCALE;
  // The -PI has no significant effect here, but the value of Theta is computed
  // using BinaryAnglePlusPi in intfx.cpp.
  Dx = (Length / 2.0) * cos((Proto->Angle / 256.0) * 2.0 * PI - PI);
  Dy = (Length / 2.0) * sin((Proto->Angle / 256.0) * 2.0 * PI - PI);

  window->SetCursor(X - Dx, Y - Dy);
  window->DrawTo(X + Dx, Y + Dy);
}                                /* RenderIntProto */
#endif

/**
 * This routine truncates Param to lie within the range
 * of Min-Max inclusive.  If a truncation is performed, and
 * Id is not null, an warning message is printed.
 *
 * @param Param   parameter value to be truncated
 * @param Min, Max  parameter limits (inclusive)
 * @param Id    string id of parameter for error messages
 *
 * Globals: none
 *
 * @return Truncated parameter.
 * @note Exceptions: none
 * @note History: Fri Feb  8 11:54:28 1991, DSJ, Created.
 */
int TruncateParam(FLOAT32 Param, int Min, int Max, char *Id) {
  if (Param < Min) {
    if (Id)
      cprintf("Warning: Param %s truncated from %f to %d!\n",
              Id, Param, Min);
    Param = Min;
  } else if (Param > Max) {
    if (Id)
      cprintf("Warning: Param %s truncated from %f to %d!\n",
              Id, Param, Max);
    Param = Max;
  }
  return static_cast<int>(floor(Param));
}                                /* TruncateParam */


#ifndef GRAPHICS_DISABLED
/**
 * Initializes the int matcher window if it is not already
 * initialized.
 */
void InitIntMatchWindowIfReqd() {
  if (IntMatchWindow == NULL) {
    IntMatchWindow = CreateFeatureSpaceWindow("IntMatchWindow", 50, 200);
    SVMenuNode* popup_menu = new SVMenuNode();

    popup_menu->AddChild("Debug Adapted classes", IDA_ADAPTIVE,
                         "x", "Class to debug");
    popup_menu->AddChild("Debug Static classes", IDA_STATIC,
                         "x", "Class to debug");
    popup_menu->AddChild("Debug Both", IDA_BOTH,
                         "x", "Class to debug");
    popup_menu->AddChild("Debug Shape Index", IDA_SHAPE_INDEX,
                         "0", "Index to debug");
    popup_menu->BuildMenu(IntMatchWindow, false);
  }
}

/**
 * Initializes the proto display window if it is not already
 * initialized.
 */
void InitProtoDisplayWindowIfReqd() {
  if (ProtoDisplayWindow == NULL) {
    ProtoDisplayWindow = CreateFeatureSpaceWindow("ProtoDisplayWindow",
                                                  550, 200);
 }
}

/**
 * Initializes the feature display window if it is not already
 * initialized.
 */
void InitFeatureDisplayWindowIfReqd() {
  if (FeatureDisplayWindow == NULL) {
    FeatureDisplayWindow = CreateFeatureSpaceWindow("FeatureDisplayWindow",
                                                    50, 700);
  }
}

/// Creates a window of the appropriate size for displaying elements
/// in feature space.
ScrollView* CreateFeatureSpaceWindow(const char* name, int xpos, int ypos) {
  return new ScrollView(name, xpos, ypos, 520, 520, 260, 260, true);
}
#endif  // GRAPHICS_DISABLED
