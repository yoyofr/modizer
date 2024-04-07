/**
 * Helper used for the passing of song meta information to the JavaScript side via EMSCRIPTEN.
 /*/
#ifndef METAINFOHELPER_H
#define METAINFOHELPER_H

#include <wchar.h>
#include <string>

namespace emsutil {
	class MetaInfoHelper {
	private:
		MetaInfoHelper();

		void copyText(char *dest, const wchar_t *src, int max_len);

	public:
		static MetaInfoHelper* getInstance();

		/**
		* Array used to pass info to JavsScript side.
		*/
		const char** getMeta();

		/**
		* Clear all the meta informatiop.
		*/
		void clear();

		/**
		* Setters for available meta info attributes.
		*/
		void setWText(unsigned char i, const wchar_t *t, const char* dflt);
		void setText(unsigned char i, const char *t, const char* dflt);
	private:
		static MetaInfoHelper* _instance;

	};
}
#endif