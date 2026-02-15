
#ifndef CODEPAGES_H
#define CODEPAGES_H

// https://docs.microsoft.com/en-us/windows/win32/intl/code-page-identifiers

const char* codepageName(UINT codepage)
{
    switch (codepage)
    {
    case 437:
        return "CP437";
    case 708:
        return "ASMO-708";
    case 737:
        return "CP737";
    case 775:
        return "CP775";
    case 850:
        return "CP850";
    case 852:
        return "CP852";
    case 855:
        return "CP855";
    case 857:
        return "CP857";
    case 858:
        return "CP858";
    case 860:
        return "CP860";
    case 861:
        return "CP861";
    case 862:
        return "CP862";
    case 863:
        return "CP863";
    case 864:
        return "CP864";
    case 865:
        return "CP865";
    case 866:
        return "CP866";
    case 869:
        return "CP869";
    case 874:
        return "CP874";
    case 932:
        return "CP932"; // SHIFT_JIS
    case 936:
        return "CP936"; // GB2312
    case 949:
        return "CP949"; // KS_C_5601-1987
    case 950:
        return "CP950"; // BIG5
    case 1200:
        return "UTF-16";
    case 1250:
        return "CP1250";
    case 1251:
        return "CP1251";
    case 1252:
        return "CP1252";
    case 1253:
        return "CP1253";
    case 1254:
        return "CP1254";
    case 1255:
        return "CP1255";
    case 1256:
        return "CP1256";
    case 1257:
        return "CP1257";
    case 1258:
        return "CP1258";
    case 1361:
        return "CP1361"; // JOHAB
    case 10000:
        return "MACINTOSH";
    case 10004:
        return "MACARABIC";
    case 10005:
        return "MACHEBREW";
    case 10006:
        return "MACGREEK";
    case 10007:
        return "MACCYRILLIC";
    case 10010:
        return "MACROMANIA";
    case 10017:
        return "MACUKRAINE";
    case 10021:
        return "MACTHAI";
    case 10029:
        return "MACCENTRALEUROPE";
    case 10079:
        return "MACICELAND";
    case 10081:
        return "MACTURKISH";
    case 10082:
        return "MACCROATIAN";
    case 12000:
        return "UTF-32";
    case 12001:
        return "UTF-32BE";
    case 20127:
        return "US-ASCII";
    case 20866:
        return "KOI8-R";
    case 20932:
        return "EUC-JP";
    case 21866:
        return "KOI8-U";
    case 28591:
        return "ISO-8859-1";
    case 28592:
        return "ISO-8859-2";
    case 28593:
        return "ISO-8859-3";
    case 28594:
        return "ISO-8859-4";
    case 28595:
        return "ISO-8859-5";
    case 28596:
        return "ISO-8859-6";
    case 28597:
        return "ISO-8859-7";
    case 28598:
        return "ISO-8859-8";
    case 28599:
        return "ISO-8859-9";
    case 28603:
        return "ISO-8859-13";
    case 28605:
        return "ISO-8859-15";
    case 50220:
        return "ISO-2022-JP";
    case 50221:
        return "CSISO2022JP";
    case 50222:
        return "ISO-2022-JP";
    case 50225:
        return "ISO-2022-KR";
    case 51932:
        return "EUC-JP";
    case 51936:
        return "EUC-CN";
    case 51949:
        return "EUC-KR";
    case 52936:
        return "HZ-GB-2312";
    case 54936:
        return "GB18030";
    case 65000:
        return "UTF-7";
    case 65001:
        return "UTF-8";
    default:
        return "";
    }
}

#endif
