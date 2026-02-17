#include <stdio.h>
#include "USBSIDInterface.h"

USBSIDitf us;

int main(int argc, char *argv[])
{
  us = create_USBSID();
  init_USBSID(us, true, true);

  bool isinit = initialised_USBSID(us);
  bool isavail = available_USBSID(us);
  bool isopen = portisopen_USBSID(us);
  int f = found_USBSID(us);

  uint8_t buff[4] = {0};
  int s = 4;
  writesingle_USBSID(us, buff, s);
  uint8_t rs = readsingle_USBSID(us, 0x0C);

  writebuffer_USBSID(us, buff, s);
  write_USBSID(us, 0x01, 0x01);
  writecycled_USBSID(us, 0x01, 0x01, 0xFFFF);
  uint8_t r = read_USBSID(us, 0x0C);

  writering_USBSID(us, 0x01, 0x01);
  writeringcycled_USBSID(us, 0x01, 0x01, 0xFFFF);

  enablethread_USBSID(us);
  disablethread_USBSID(us);
  setflush_USBSID(us);
  flush_USBSID(us);
  restartringbuffer_USBSID(us);
  setbuffsize_USBSID(us, 8192);
  setdiffsize_USBSID(us, 64);
  restartthread_USBSID(us, true);

  int_fast64_t d = waitforcycle_USBSID(us, 0xFFFF);

  setclockrate_USBSID(us, 100000, true);
  long cr = getclockrate_USBSID(us);
  long rr = getrefreshrate_USBSID(us);
  long rar = getrasterrate_USBSID(us);
  int nsids = getnumsids_USBSID(us);
  int nfmsid = getfmoplsid_USBSID(us);
  int pcbv = getpcbversion_USBSID(us);
  setstereo_USBSID(us, 1);
  togglestereo_USBSID(us);
  pause_USBSID(us);
  mute_USBSID(us);
  unmute_USBSID(us);
  clearbus_USBSID(us);
  resetallregisters_USBSID(us);
  reset_USBSID(us);
  close_USBSID(us);

  /* Helper to ignore unused variables error */
  printf("%d %d %d %d %x %x %ld %ld %ld %ld %d %d %d\n",
    isinit,isavail,isopen,f,
    rs, r, d,
    cr,rr,rar,nsids,nfmsid,pcbv
  );

  return 1;
}
