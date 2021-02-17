/**********************************************************************
 * File:			word.c
 * Description: Code for the WERD class.
 * Author:		Ray Smith
 * Created:		Tue Oct 08 14:32:12 BST 1991
 *
 * (C) Copyright 1991, Hewlett-Packard Ltd.
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 ** http://www.apache.org/licenses/LICENSE-2.0
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 *
 **********************************************************************/

#ifndef           WERD_H
#define           WERD_H

#include          "params.h"
#include          "bits16.h"
#include          "elst2.h"
#include          "strngs.h"
#include          "blckerr.h"
#include          "stepblob.h"

enum WERD_FLAGS
{
  W_SEGMENTED,                   //< correctly segmented
  W_ITALIC,                      //< italic text
  W_BOLD,                        //< bold text
  W_BOL,                         //< start of line
  W_EOL,                         //< end of line
  W_NORMALIZED,                  //< flags
  W_SCRIPT_HAS_XHEIGHT,          //< x-height concept makes sense.
  W_SCRIPT_IS_LATIN,             //< Special case latin for y. splitting.
  W_DONT_CHOP,                   //< fixed pitch chopped
  W_REP_CHAR,                    //< repeated character
  W_FUZZY_SP,                    //< fuzzy space
  W_FUZZY_NON,                   //< fuzzy nonspace
  W_INVERSE                      //< white on black
};

enum DISPLAY_FLAGS
{
  /* Display flags bit number allocations */
  DF_BOX,                        //< Bounding box
  DF_TEXT,                       //< Correct ascii
  DF_POLYGONAL,                  //< Polyg approx
  DF_EDGE_STEP,                  //< Edge steps
  DF_BN_POLYGONAL,               //< BL normalisd polyapx
  DF_BLAMER                      //< Blamer information
};

class ROW;                       //forward decl

class WERD : public ELIST2_LINK {
  public:
    WERD() {}
    // WERD constructed with:
    //   blob_list - blobs of the word (we take this list's contents)
    //   blanks - number of blanks before the word
    //   text - correct text (outlives WERD)
    WERD(C_BLOB_LIST *blob_list, uinT8 blanks, const char *text);

    // WERD constructed from:
    //   blob_list - blobs in the word
    //   clone - werd to clone flags, etc from.
    WERD(C_BLOB_LIST *blob_list, WERD *clone);

    // Construct a WERD from a single_blob and clone the flags from this.
    // W_BOL and W_EOL flags are set according to the given values.
    WERD* ConstructFromSingleBlob(bool bol, bool eol, C_BLOB* blob);

    ~WERD() {
    }

    // assignment
    WERD & operator= (const WERD &source);

    // This method returns a new werd constructed using the blobs in the input
    // all_blobs list, which correspond to the blobs in this werd object. The
    // blobs used to construct the new word are consumed and removed from the
    // input all_blobs list.
    // Returns NULL if the word couldn't be constructed.
    // Returns original blobs for which no matches were found in the output list
    // orphan_blobs (appends).
    WERD *ConstructWerdWithNewBlobs(C_BLOB_LIST *all_blobs,
                                    C_BLOB_LIST *orphan_blobs);

    // Accessors for reject / DUFF blobs in various formats
    C_BLOB_LIST *rej_cblob_list() {  // compact format
      return &rej_cblobs;
    }

    // Accessors for good blobs in various formats.
    C_BLOB_LIST *cblob_list() {  // get compact blobs
      return &cblobs;
    }

    uinT8 space() {  // access function
      return blanks;
    }
    void set_blanks(uinT8 new_blanks) {
      blanks = new_blanks;
    }
    int script_id() const {
      return script_id_;
    }
    void set_script_id(int id) {
      script_id_ = id;
    }

    // Returns the (default) bounding box including all the dots.
    TBOX bounding_box() const;  // compute bounding box
    // Returns the bounding box including the desired combination of upper and
    // lower noise/diacritic elements.
    TBOX restricted_bounding_box(bool upper_dots, bool lower_dots) const;
    // Returns the bounding box of only the good blobs.
    TBOX true_bounding_box() const;

    const char *text() const { return correct.string(); }
    void set_text(const char *new_text) { correct = new_text; }

    BOOL8 flag(WERD_FLAGS mask) const { return flags.bit(mask); }
    void set_flag(WERD_FLAGS mask, BOOL8 value) { flags.set_bit(mask, value); }

    BOOL8 display_flag(uinT8 flag) const { return disp_flags.bit(flag); }
    void set_display_flag(uinT8 flag, BOOL8 value) {
      disp_flags.set_bit(flag, value);
    }

    WERD *shallow_copy();  // shallow copy word

    // reposition word by vector
    void move(const ICOORD vec);

    // join other's blobs onto this werd, emptying out other.
    void join_on(WERD* other);

    // copy other's blobs onto this word, leaving other intact.
    void copy_on(WERD* other);

    // tprintf word metadata (but not blob innards)
    void print();

    #ifndef GRAPHICS_DISABLED
    // plot word on window in a uniform colour
    void plot(ScrollView *window, ScrollView::Color colour);

    // Get the next color in the (looping) rainbow.
    static ScrollView::Color NextColor(ScrollView::Color colour);

    // plot word on window in a rainbow of colours
    void plot(ScrollView *window);

    // plot rejected blobs in a rainbow of colours
    void plot_rej_blobs(ScrollView *window);
    #endif  // GRAPHICS_DISABLED

    // Removes noise from the word by moving small outlines to the rej_cblobs
    // list, based on the size_threshold.
    void CleanNoise(float size_threshold);

    // Extracts all the noise outlines and stuffs the pointers into the given
    // vector of outlines. Afterwards, the outlines vector owns the pointers.
    void GetNoiseOutlines(GenericVector<C_OUTLINE *> *outlines);
    // Adds the selected outlines to the indcated real blobs, and puts the rest
    // back in rej_cblobs where they came from. Where the target_blobs entry is
    // NULL, a run of wanted outlines is put into a single new blob.
    // Ownership of the outlines is transferred back to the word. (Hence
    // GenericVector and not PointerVector.)
    // Returns true if any new blob was added to the start of the word, which
    // suggests that it might need joining to the word before it, and likewise
    // sets make_next_word_fuzzy true if any new blob was added to the end.
    bool AddSelectedOutlines(const GenericVector<bool> &wanted,
                             const GenericVector<C_BLOB *> &target_blobs,
                             const GenericVector<C_OUTLINE *> &outlines,
                             bool *make_next_word_fuzzy);

 private:
    uinT8 blanks;                // no of blanks
    uinT8 dummy;                 // padding
    BITS16 flags;                // flags about word
    BITS16 disp_flags;           // display flags
    inT16 script_id_;            // From unicharset.
    STRING correct;              // correct text
    C_BLOB_LIST cblobs;          // compacted blobs
    C_BLOB_LIST rej_cblobs;      // DUFF blobs
};

ELIST2IZEH (WERD)
#include          "ocrrow.h"     // placed here due to
// compare words by increasing order of left edge, suitable for qsort(3)
int word_comparator(const void *word1p, const void *word2p);
#endif
