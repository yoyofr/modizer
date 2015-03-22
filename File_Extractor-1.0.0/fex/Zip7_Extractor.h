// 7-zip archive extractor

// File_Extractor 1.0.0
#ifndef ZIP7_EXTRACTOR_H
#define ZIP7_EXTRACTOR_H

#include "File_Extractor.h"

struct Zip7_Extractor_Impl;

class Zip7_Extractor : public File_Extractor {
public:
	Zip7_Extractor();
	virtual ~Zip7_Extractor();

protected:
	virtual blargg_err_t open_v();
	virtual void         close_v();
	
	virtual blargg_err_t next_v();
	virtual blargg_err_t rewind_v();
	virtual fex_pos_t    tell_arc_v() const;
	virtual blargg_err_t seek_arc_v( fex_pos_t );

	virtual blargg_err_t data_v( void const** out );
	
private:
	Zip7_Extractor_Impl* impl;
	fex_pos_t index;
	blargg_vector<char> name8;
    blargg_vector<blargg_wchar_t> name16;
	
	blargg_err_t zip7_err( int err );
};

#endif
