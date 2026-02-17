package test.usbsid;

import java.io.IOException;
import java.io.InputStream;
import java.lang.reflect.Array;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.IntBuffer;
import java.text.MessageFormat;
import java.util.Arrays;
import java.util.function.Supplier;
import java.util.logging.Level;
import java.util.logging.LogManager;
import java.util.logging.Logger;

// import static org.junit.jupiter.api.Assertions.assertTrue;

import org.testng.annotations.Test;
import org.usb4java.Device;
import org.usb4java.DeviceDescriptor;
import org.usb4java.DeviceHandle;
import org.usb4java.DeviceList;
import org.usb4java.LibUsb;

import usbsid.USBSID;

public class ConnectTest
{
  // private static final Logger logger = Logger.getLogger("[USBSID TEST]");
  // private static Logger logger = null;
  // static {
  //   InputStream stream = ConnectTest.class.getClassLoader().
  //     getResourceAsStream("logging.properties");

  //   try {
  //     LogManager.getLogManager().readConfiguration(stream);
  //     logger = Logger.getLogger(ConnectTest.class.getName());

  //   } catch (IOException e) {
  //     e.printStackTrace();
  //   }
  // }
  private static Logger logger = null;
  static {
    System.setProperty("java.util.logging.SimpleFormatter.format",
                "[%1$tF %1$tT] [TEST] [%4$s] %5$s %n");
    logger = Logger.getLogger(ConnectTest.class.getName());
  }

  private boolean error = false;
  private boolean checkforError(Supplier<Boolean> rcCheck, String info)
  {
		if (rcCheck.get()) {
			logger.info(info + " ~ OK");
      return error;
		} else {
			logger.severe(info + " ~ FAILED");
			error = true;
		}
		return error;
  }

  private void readBasics(String driver)
  {
    logger.info(MessageFormat.format("Starting test for driver: {0}", driver));

    final USBSID usbsid = new USBSID();

    if (checkforError(() -> usbsid.USBSID_init(driver, 8196, 64) == 0, "USBSID-Pico opened"))
      return;

    final int numsids = usbsid.USBSID_getnumsids();
    if (checkforError(() -> numsids >= 1, "Number of SID's available: " + numsids))
      return;

    final String pcbversion = usbsid.USBSID_getpcbversion();
    if (checkforError(() -> pcbversion.length() >= 3, MessageFormat.format("PCB version read: v{0}", pcbversion)))
      return;

    final String fwversion = usbsid.USBSID_getfwversion();
    if (checkforError(() -> fwversion.length() >= 3, MessageFormat.format("Firmware version read: {0}", fwversion)))
      return;

    final byte[] socketcfg = usbsid.USBSID_getsocketconfig();
    if (checkforError(() -> socketcfg.length == 10, MessageFormat.format("Read socket config: {0}", Arrays.toString(socketcfg))))
      return;

    final int s1s1 = usbsid.USBSID_getsocketsidtype(1, 1, socketcfg);
    final int s1s2 = (numsids > 1) ? usbsid.USBSID_getsocketsidtype(1, 2, socketcfg) : 0;
    final int s2s1 = (numsids > 2) ? usbsid.USBSID_getsocketsidtype(2, 1, socketcfg) : 0;
    final int s2s2 = (numsids > 3) ? usbsid.USBSID_getsocketsidtype(2, 2, socketcfg) : 0;
    if (checkforError(() -> s1s1 != -1, MessageFormat.format("Socket one sid one type read: {0}", s1s1)))
      return;
    if (checkforError(() -> s1s2 != -1, MessageFormat.format("Socket one sid two type read: {0}", s1s2)))
      return;
    if (checkforError(() -> s2s1 != -1, MessageFormat.format("Socket two sid one type read: {0}", s2s1)))
      return;
    if (checkforError(() -> s2s2 != -1, MessageFormat.format("Socket two sid two type read: {0}", s2s2)))
      return;

    final int sid1 = usbsid.USBSID_sidtypebysidno(0, socketcfg);
    final int sid2 = (numsids > 1) ? usbsid.USBSID_sidtypebysidno(1, socketcfg) : 0;
    final int sid3 = (numsids > 2) ? usbsid.USBSID_sidtypebysidno(2, socketcfg) : 0;
    final int sid4 = (numsids > 3) ? usbsid.USBSID_sidtypebysidno(3, socketcfg) : 0;
    if (checkforError(() -> (sid1 != -1 && sid2 != -1 && sid3 != -1 && sid4 != -1),
      MessageFormat.format("SID type by sidno: SID1 {0}, SID2 {1}, SID3 {2}, SID4 {3}", sid1, sid2, sid3, sid4)))
      return;

    final int cfg[] = usbsid.USBSID_parsesocketconfig(socketcfg);
    if (checkforError(() -> cfg.length == 16,
      MessageFormat.format("Parsed socket config {0}", Arrays.toString(cfg))))
      return;

    if (checkforError(() -> usbsid.USBSID_setstereo(1) == 1, "Set USBSID to Stereo"))
      return;
    if (checkforError(() -> usbsid.USBSID_setstereo(0) == 0, "Set USBSID to Mono"))
      return;

    usbsid.USBSID_exit();
    logger.info("USBSID-Pico closed");
  }


  @Test
  public void testUSBX()
  {
    readBasics("usbsx");
  }

  // @Test
  // public void testUSBL()
  // {
  //   readBasics("libusb");
  // }

}
