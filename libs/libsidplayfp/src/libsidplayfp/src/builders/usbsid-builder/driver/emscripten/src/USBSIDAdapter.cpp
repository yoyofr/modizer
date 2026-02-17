#ifdef EMSCRIPTEN
#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>

#include "USBSID.h"
using USBSID_NS::USBSID_Class;
using namespace USBSID_NS;

EMSCRIPTEN_BINDINGS(us_class)
{
  emscripten::class_<USBSID_Class>("USBSID_Class")
    .constructor<>()
    .function("USBSID_Init", &USBSID_Class::USBSID_Init)
    .function("USBSID_Close", &USBSID_Class::USBSID_Close)
    .function("USBSID_SetClockRate", &USBSID_Class::USBSID_SetClockRate)
    .function("USBSID_GetClockRate", &USBSID_Class::USBSID_GetClockRate)
    .function("USBSID_GetRefreshRate", &USBSID_Class::USBSID_GetRefreshRate)
    .function("USBSID_GetRasterRate", &USBSID_Class::USBSID_GetRasterRate)
    .function("USBSID_Flush", &USBSID_Class::USBSID_Flush)
    .function("USBSID_SetFlush", &USBSID_Class::USBSID_SetFlush)
    ;
}
#endif
