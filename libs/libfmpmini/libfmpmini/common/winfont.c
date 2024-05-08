#include "winfont.h"
#include <windows.h>

struct font_win32 {
  HFONT font;
  HDC dc;
  HBITMAP bitmap;
  HBRUSH white;
  HBRUSH black;
  uint8_t *data;
  uint8_t buf[16];
};

/*
#include "jisunih.h"

uint16_t jis2unih(uint8_t jis) {
  if (jis & 0x80) {
    const uint16_t *tab = jis2unicode_h;
    while (tab[0]) {
      if (tab[0] == jis) return tab[1];
      tab += 2;
    }
    return 0;
  } else {
    return jis;
  }
}
*/
uint16_t jis2unih(uint8_t jis) {
  uint16_t wbuf[2];
  int outchar = MultiByteToWideChar(932, MB_ERR_INVALID_CHARS,
                                    (char *)&jis, 1, (wchar_t *)wbuf, 2);
  if (!outchar) return 0;
  return wbuf[0];
}
/*
#include "jisuni.h"

uint16_t jis2uni(uint16_t jis) {
  if ((jis>>8) == 0x29) {
    return jis2unih(jis & 0xff);
  }
  const uint16_t *tab = jis2unicode;
  while (tab[0]) {
    if (tab[0] == jis) return tab[1];
    tab += 2;
  }
  return 0;
}
*/
uint16_t jis2uni(uint16_t jis) {
  uint16_t sjis = jis2sjis(jis);
  uint8_t abuf[2] = {sjis>>8, sjis};
  uint16_t wbuf[2];
  int outchar = MultiByteToWideChar(932, MB_ERR_INVALID_CHARS,
                                    (char *)abuf, 2, (wchar_t *)wbuf, 2);
  if (!outchar) return 0;
  return wbuf[0];
}

static const void *winfont_get(const struct fmdsp_font *font,
                               uint16_t c, enum fmdsp_font_type type) {
  struct font_win32 *fw = (struct font_win32 *)font->data;
  RECT r = {
    0, 0, 8, 16
  };
  FillRect(fw->dc, &r, fw->white);
  wchar_t text[2] = {0};
  switch (type) {
  case FMDSP_FONT_ANK:
    text[0] = jis2unih(c);
    TextOutW(fw->dc, 0, 0, text, 1);
    break;
  case FMDSP_FONT_JIS_LEFT:
    {
      uint8_t row = c >> 8;
      if (row == 0x29 || row == 0x2a) {
        // doublebyte halfwidth ANK
        // JIS doublewidth  JIS halfwidth
        // 0x2921-0x297e -> 0x21-0x7e (ASCII)
        // 0x2a21-0x2a7e -> 0xa1-0xfe (halfwidth katakana)
        uint16_t jis = (c & 0xff) | ((row-0x29) * 0x80);
        text[0] = jis2unih(jis);
      } else {
        text[0] = jis2uni(c);
      }
    }
    TextOutW(fw->dc, 0, 0, text, 1);
    break;
  case FMDSP_FONT_JIS_RIGHT:
    text[0] = jis2uni(c);
    TextOutW(fw->dc, -8, 0, text, 1);
    break;
  }
  GdiFlush();
//  return fw->data;
  for (int y = 0; y < 16; y++) {
    fw->buf[y] = fw->data[y*4];
  }
  return fw->buf;
}

struct BITMAPINFO_mono {
  BITMAPINFOHEADER bmihead;
  RGBQUAD bmicolors[2];
};

bool fmdsp_font_win(struct fmdsp_font *font) {
  struct font_win32 *fw = HeapAlloc(
    GetProcessHeap(), 0, sizeof(struct font_win32)
  );
  if (!fw) goto err;
  fw->font = CreateFontW(16, 0, 0, 0, 
                         FW_NORMAL, FALSE, FALSE, FALSE,
                         SHIFTJIS_CHARSET,
                         OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                         NONANTIALIASED_QUALITY, FIXED_PITCH,
                         L"MS Gothic");
  if (!fw->font) goto err_fw;
  fw->dc = CreateCompatibleDC(0);
  if (!fw->dc) goto err_font;
  struct BITMAPINFO_mono bmi = {0};
  bmi.bmihead.biSize = sizeof(bmi.bmihead);
  bmi.bmihead.biWidth = 8;
  bmi.bmihead.biHeight = -16;
  bmi.bmihead.biPlanes = 1;
  bmi.bmihead.biBitCount = 1;
  bmi.bmihead.biCompression = BI_RGB;
  bmi.bmihead.biSizeImage = 16;
  bmi.bmihead.biClrUsed = 2;
  bmi.bmicolors[0].rgbRed = 0xff;
  bmi.bmicolors[0].rgbGreen = 0xff;
  bmi.bmicolors[0].rgbBlue = 0xff;
  fw->bitmap = CreateDIBSection(
    fw->dc, (BITMAPINFO *)&bmi,
    DIB_RGB_COLORS, (void **)&fw->data,
    0, 0);
  if (!fw->bitmap) {
    goto err_dc;
  }
  SelectObject(fw->dc, fw->bitmap);
  SelectObject(fw->dc, fw->font);
  fw->white = CreateSolidBrush(RGB(0xff, 0xff, 0xff));
  fw->black = CreateSolidBrush(RGB(0x00, 0x00, 0x00));
  font->get = winfont_get;
  font->data = fw;
  font->width_half = 8;
  font->height = 16;
  return true;
err_dc:
  DeleteDC(fw->dc);
err_font:
  DeleteObject(fw->font);
err_fw:
  HeapFree(GetProcessHeap(), 0, fw);
err:
  return false;
}
