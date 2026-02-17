package usbsid;

public interface IUSBSID {

  int USBSID_init(Integer...buffsize);
  int USBSID_init();
  int USBSID_init(String driver, int ringsize, int diffsize);

  void USBSID_exit();

  void USBSID_clkdwrite(byte addr, byte data, short cycles);

  void USBSID_writeclkdbuffer(byte addr, byte data, short cycles);

  void USBSID_setflush();

  void USBSID_reset(byte volume);

  void USBSID_setclock(double CpuClock);

  int USBSID_setstereo(int stereo);

  byte[] USBSID_getsocketconfig();

  int[] USBSID_parsesocketconfig(byte[] socketcfg);

  int USBSID_getsocketsidtype(int socket, int sidno, byte[] socketcfg);

  int USBSID_sidtypebysidno(int sidno, byte[] socketcfg);

  int USBSID_getnumsids();

  String USBSID_getpcbversion();

  String USBSID_getfwversion();

  void USBSID_delay(short cycles);

}
