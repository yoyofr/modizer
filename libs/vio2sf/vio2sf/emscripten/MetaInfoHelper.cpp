#include "MetaInfoHelper.h"


#define __STDC_WANT_LIB_EXT1__ 1 /* mbstowcs */
#include <stdlib.h>     /* malloc, free, rand */
#include <locale.h>

#include <stdio.h>
#include <string.h>

#define TEXT_MAX	255


// no point to expose these internals in the header..
#define MAX_ENTRIES 7
static char* _infoTexts[MAX_ENTRIES];

static char _t0[TEXT_MAX];
static char _t1[TEXT_MAX];
static char _t2[TEXT_MAX];
static char _t3[TEXT_MAX];
static char _t4[TEXT_MAX];
static char _t5[TEXT_MAX];
static char _t6[TEXT_MAX];

// using the singleton this old C style initialization hack could now be ditched
struct StaticBlock {
    StaticBlock(){
		_infoTexts[0]= _t0;
		_infoTexts[1]= _t1;
		_infoTexts[2]= _t2;
		_infoTexts[3]= _t3;
		_infoTexts[4]= _t4;
		_infoTexts[5]= _t5;
		_infoTexts[6]= _t6;
    }
};

static StaticBlock _emscripen_info;

emsutil::MetaInfoHelper::MetaInfoHelper() {
}

emsutil::MetaInfoHelper* emsutil::MetaInfoHelper::_instance = NULL;

emsutil::MetaInfoHelper* emsutil::MetaInfoHelper::getInstance() {
	if (_instance == NULL) {
		_instance = new MetaInfoHelper();
		return _instance;
	} else {
		return _instance;
	}
}

const char** emsutil::MetaInfoHelper::getMeta() {
	return (const char**)_infoTexts;
}

void emsutil::MetaInfoHelper::clear() {
	strcpy(_t0, "");
	strcpy(_t1, "");
	strcpy(_t2, "");
	strcpy(_t3, "");
	strcpy(_t4, "");
	strcpy(_t5, "");
	strcpy(_t6, "");
}

void emsutil::MetaInfoHelper::copyText(char *dest, const wchar_t *src, int max_len) {
	if (src) {
		setlocale(LC_ALL, "");
		snprintf(dest, max_len, "%ls", src);
	}
}

void emsutil::MetaInfoHelper::setWText(unsigned char i, const wchar_t *t,const char* dflt) {
	if (i >= MAX_ENTRIES) {
		fprintf(stderr, "error: MetaInfoHelper out of bounds access\n");
		exit(-1);
	}
	if (t) {
		copyText(_infoTexts[i], t, TEXT_MAX);
	} else {
		strncpy(_infoTexts[i], dflt, TEXT_MAX);
	}
}
void emsutil::MetaInfoHelper::setText(unsigned char i, const char *t,const char* dflt) {
	if (i >= MAX_ENTRIES) {
		fprintf(stderr, "error: MetaInfoHelper out of bounds access\n");
		exit(-1);
	}
	strncpy(_infoTexts[i], t?t:dflt, TEXT_MAX);
}

