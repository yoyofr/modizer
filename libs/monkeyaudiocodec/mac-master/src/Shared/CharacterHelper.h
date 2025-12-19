/*******************************************************************************************
Character set conversion helpers
*******************************************************************************************/

#ifndef APE_CHARACTER_HELPER_H
#define APE_CHARACTER_HELPER_H

class CAPECharacterHelper
{
public:

    static str_ansi * GetANSIFromUTF8(const str_utf8 * pUTF8);
    static str_ansi * GetANSIFromUTF16(const str_utf16 * pUTF16);
    static str_utf16 * GetUTF16FromANSI(const str_ansi * pANSI);
    static str_utf16 * GetUTF16FromUTF8(const str_utf8 * pUTF8);
    static str_utf8 * GetUTF8FromANSI(const str_ansi * pANSI);
    static str_utf8 * GetUTF8FromUTF16(const str_utf16 * pUTF16);
};

#endif // #ifndef APE_CHARACTER_HELPER_H