package usbsid;

// import java.io.*;
import java.nio.ByteBuffer;
import java.nio.IntBuffer;
import java.text.MessageFormat;
import java.util.Arrays;
import java.util.List;
import java.util.logging.Logger;

import org.usb4java.BufferUtils;
// import org.usb4java;
import org.usb4java.Context;
import org.usb4java.DeviceList;
import org.usb4java.LibUsb;
import org.usb4java.Device;
import org.usb4java.DeviceDescriptor;
import org.usb4java.DeviceHandle;
// import org.usb4java.Interface;
// import org.usb4java.EndpointDescriptor;
import org.usb4java.LibUsbException;
import org.usb4java.Transfer;
import org.usb4java.TransferCallback;

import javax.usb.UsbPipe;
import javax.usb.UsbDevice;
import javax.usb.UsbDeviceDescriptor;
import javax.usb.UsbConfiguration;
import javax.usb.UsbInterface;
import javax.usb.UsbInterfacePolicy;
import javax.usb.UsbEndpoint;
import javax.usb.UsbException;
import javax.usb.UsbHostManager;
import javax.usb.UsbHub;
// import javax.usb.util.*;

// import org.apache.commons.lang3.ObjectUtils.Null;


public class USBSIDDevice {

  /* Logging */
  private static Logger logger = null;
  static {
    System.setProperty("java.util.logging.SimpleFormatter.format",
                "[%1$tF %1$tT] [USBSID] [%4$s] %5$s %n");
    logger = Logger.getLogger(USBSIDDevice.class.getName());
  }

  /* Device constants */
  private static byte US_CFG = 1;
  private static byte US_ITF = 1;
  private static byte US_EPOUT = (byte)0x02;
  private static byte US_EPIN = (byte)0x82;
  private static short VENDOR_ID = (short)0xCAFE;
  private static short PRODUCT_ID = (short)0x4011;

  /* Mutable */
  public static Object device = null;

  /* Device variables */
  public static boolean us_overrideDriver = false;
  private static boolean us_isUSBX = false;
  private static boolean us_isLIBUSB = false;
  private static boolean us_isAvailable = false;
  private static boolean us_isOpen = false;

  private class USBX {

    /* javax.usb */
    private static UsbPipe pipe_out = null;
    private static UsbPipe pipe_in = null;
    private static UsbDevice xdevice = null;
    private static UsbConfiguration config = null;
    private static UsbInterface iface = null;
    private static UsbEndpoint ep_out = null;
    private static UsbEndpoint ep_in = null;

    private static void openUSBX()
      throws UsbException
    {
      xdevice = findUSBSIDX(
          UsbHostManager.getUsbServices().getRootUsbHub());
      if (xdevice == null)
      {
          logger.warning("USBSID-Pico not found");
          return;
      }
      device = xdevice;
      config = xdevice.getUsbConfiguration(US_CFG);
      iface = config.getUsbInterface(US_ITF);
      iface.claim(new UsbInterfacePolicy()
      {
        @Override
        public boolean forceClaim(UsbInterface usbInterface)
        {
          return true;
        }
      });

      ep_out = iface.getUsbEndpoint(US_EPOUT);
      pipe_out = ep_out.getUsbPipe();
      pipe_out.open();
      ep_in = iface.getUsbEndpoint(US_EPIN);
      pipe_in = ep_in.getUsbPipe();
      pipe_in.open();
      us_isOpen = true;
    }

    private static void closeUSBX()
      throws UsbException
    {
      if (pipe_in != null) pipe_in.close();
      if (pipe_out != null) pipe_out.close();
      try {
        if (iface.isClaimed()) iface.release();
      } catch (javax.usb.UsbPlatformException uPE) {
        logger.info("Device not found for re-attaching kernel, skipping.");
      }
    }

    private static UsbDevice findUSBSIDX(UsbHub hub)
      throws UsbException
    {
      UsbDevice usbsidpico = null;
      for (UsbDevice xdevice: (List<UsbDevice>) hub.getAttachedUsbDevices())
        {
          if (xdevice.isUsbHub())
          {
            usbsidpico = findUSBSIDX((UsbHub) xdevice);
            if (usbsidpico != null) return usbsidpico;
          }
          else
          {
            UsbDeviceDescriptor desc = xdevice.getUsbDeviceDescriptor();
            if (desc.idVendor() == VENDOR_ID &&
                desc.idProduct() == PRODUCT_ID) return xdevice;
          }
        }
        return null;
    }

    private static void asyncWriteX(byte[] buffer)
      throws UsbException
    {
      try {
        pipe_out.asyncSubmit(buffer);
      } catch (javax.usb.UsbDisconnectedException UDE) {
        logger.info("[USBSID] was already disconnected");
      }
    }

    private static void syncWriteX(byte[] buffer)
      throws UsbException
    {
      try {
        pipe_out.syncSubmit(buffer);
      } catch (javax.usb.UsbDisconnectedException UDE) {
        logger.info("[USBSID] was already disconnected");
      }
    }

    private static byte[] syncReadX(byte[] buffer, int len)
      throws UsbException
    {
      /* Beats me why this is, but hey it works ;) */
      len *= 2; /* double the size of the read length for 2nd URB package */
      byte[] data = new byte[len];
      try {
        pipe_out.syncSubmit(buffer);
        pipe_in.syncSubmit(data);
      } catch (javax.usb.UsbDisconnectedException UDE) {
        logger.info("[USBSID] was already disconnected");
      }
      byte[] b_in = Arrays.copyOfRange(data, 0, (len/2));
      return b_in;
    }

  }

  private class USBL {

    /* usb4java.libusb */
    private static Context ctx = null;
    private static DeviceList devicelist = null;
    private static Device ldevice = null;
    private static DeviceHandle devh = null;
    private static Transfer transfer_out = null;
    private static ByteBuffer out_buffer = null;

    private static int len_out_buffer = 64;
    private static int timeout = 0;
    private static int LIBUSB_OPTION_LOG_LEVEL = 0;
    private static int LIBUSB_OPTION_USE_USBDK = 1;
    private static short ACM_CTRL_DTR = 0x01;
    private static short ACM_CTRL_RTS = 0x02;

    private static boolean kernel_isDetached = false;
    private static int result = -1;

    private static void openLIBUSB()
      throws LibUsbException
    {
      ctx = new Context();
      result = LibUsb.init(ctx);
      if (result != LibUsb.SUCCESS) throw new LibUsbException("[USBSID] Unable to initialize libusb.", result);

      LibUsb.setOption(ctx, LIBUSB_OPTION_LOG_LEVEL, 0); /* 4 for max verbose logging */
      LibUsb.setOption(ctx, LIBUSB_OPTION_USE_USBDK, 1); /* 1 to enable */

      result = findLUSBSID(VENDOR_ID, PRODUCT_ID);
      if (ldevice == null) {
        logger.warning("USBSID-Pico not found");
        return;
      }
      if ((ldevice != null) && us_isAvailable) {
        try {
          devh = LibUsb.openDeviceWithVidPid(ctx, VENDOR_ID, PRODUCT_ID);
        } catch (LibUsbException LUE) {
          String error = "[USBSID] Opening device unsuccessful";
          logger.severe(error);
          throw(new LibUsbException(error, result));
        }
      } else {
        device = null;
        us_isAvailable = false;
        return;
      }

      detachKernelDriver();
      configureDevice();
      initOutBuffer();
      us_isOpen = true;
      device = ldevice;
    }

    private static void closeLIBUSB()
      throws LibUsbException
    {
      us_isOpen = false;
      deinitOutBuffer();
      int result = LibUsb.releaseInterface(devh, US_ITF);
      if (result != LibUsb.SUCCESS) throw new LibUsbException("[USBSID] Unable to release interface", result);
      if (kernel_isDetached) {
        releaseInterface();
      }
      LibUsb.exit(ctx);
    }

    private static int findLUSBSID(short vendorId, short productId)
      throws LibUsbException
    {
      /* Read the USB device list */
      devicelist = new DeviceList();
      int result = LibUsb.getDeviceList(ctx, devicelist);
      if (result < 0) throw new LibUsbException("[USBSID] Unable to get device list", result);

      try
      {
        // Iterate over all devices and scan for the right one
        for (Device dev: devicelist)
        {
          DeviceDescriptor descriptor = new DeviceDescriptor();
          result = LibUsb.getDeviceDescriptor(dev, descriptor);
          if (result != LibUsb.SUCCESS) throw new LibUsbException("[USBSID] Unable to read device descriptor", result);
          if (descriptor.idVendor() == vendorId && descriptor.idProduct() == productId) {
            ldevice = dev;
            us_isAvailable = true;
            return 0;
          }
        }
      }
      finally
      {
        /* Ensure the allocated device list is freed */
        LibUsb.freeDeviceList(devicelist, true);
      }
      return -1;
    }

    private static void configureDevice()
    {
      try {
        ByteBuffer buffer = ByteBuffer.allocateDirect(0);
        result = LibUsb.controlTransfer(devh, (byte)0x21, (byte)0x22, (short)(ACM_CTRL_DTR | ACM_CTRL_RTS), (short)0, buffer, (long)100);
      } catch (LibUsbException LUE) {
        logger.info("CONFIG ERROR: " + result);
      }
    }

    private static void initOutBuffer()
    {
      try {
        out_buffer = LibUsb.devMemAlloc(devh, len_out_buffer);
        if (out_buffer == null) throw new Exception("[USBSID] LibUsb.devMemAlloc failed", null);
      } catch (Exception e) {
        logger.info("[USBSID] Unable to use devMemAlloc, allocating default");
        out_buffer = BufferUtils.allocateByteBuffer(len_out_buffer);
      }
      transfer_out = LibUsb.allocTransfer();
      LibUsb.fillBulkTransfer(transfer_out, devh, US_EPOUT, out_buffer, usb_out, null, timeout);
    }

    private static void deinitOutBuffer()
    {
      LibUsb.cancelTransfer(transfer_out);
      LibUsb.freeTransfer(transfer_out);
      try {
        LibUsb.devMemFree(devh, out_buffer, len_out_buffer);
      } catch (Exception e) {
        logger.info("[USBSID] Unable to use devMemFree, freeing default");
        out_buffer = null;
      }
    }

    private static void detachKernelDriver()
    {
      for (int itf = 0; itf < 2; itf++) {
        if (LibUsb.kernelDriverActive(devh, itf) == 1) {
          try {
            result = LibUsb.detachKernelDriver(devh, itf);
            if (result != LibUsb.SUCCESS || result != LibUsb.ERROR_NOT_SUPPORTED) throw new LibUsbException("[USBSID] Unable to claim interface", result);
          } catch (LibUsbException LUE) {
            logger.info(MessageFormat.format("[USBSID] Detaching kerneldriver for interface {0} result: {0}", itf, LUE.getMessage()));
          }
        }
        try {
          result = LibUsb.claimInterface(devh, itf);
          if (result != LibUsb.SUCCESS) throw new LibUsbException("[USBSID] Unable to claim interface", result);
        } catch (LibUsbException LUE) {
          logger.info(MessageFormat.format("[USBSID] Claiming interface {0} result: {0}", itf, LUE.getMessage()));
        }
      }
      kernel_isDetached = true;
    }

    private static void releaseInterface()
    {
      for (int itf = 0; itf < 2; itf++) {
        if (LibUsb.kernelDriverActive(devh, itf) == 1) {
          try {
            result = LibUsb.detachKernelDriver(devh, itf);
            if (result != LibUsb.SUCCESS || result != LibUsb.ERROR_NOT_SUPPORTED) throw new LibUsbException("[USBSID] Unable to claim interface", result);
          } catch (LibUsbException LUE) {
            logger.info(MessageFormat.format("[USBSID] Detaching kerneldriver for interface {0} result: {0}", itf, LUE.getMessage()));
          }
        }
        try {
          result = LibUsb.releaseInterface(devh, itf);
          if (result != LibUsb.SUCCESS) throw new LibUsbException("[USBSID] Unable to release interface", result);
        } catch (LibUsbException LUE) {
          logger.info(MessageFormat.format("[USBSID] Release interface {0} result: {0}", itf, LUE.getMessage()));
        }
      }
      kernel_isDetached = false;
    }

    private static void asyncWriteL(byte[] buffer)
    {
      try {
        out_buffer.put(buffer);
        int result = LibUsb.submitTransfer(transfer_out);
        LibUsb.handleEventsCompleted(ctx, null);
        if (result != LibUsb.SUCCESS) throw(new LibUsbException("[USBSID] Transfer failed", result));
      } catch (LibUsbException LUE) {
        throw new LibUsbException(LUE.getMessage(), LUE.getErrorCode());
      } finally {
        out_buffer.position(0);
      }
    }

    private static void syncWriteL(byte[] buffer)
      throws LibUsbException
    {
      try {
        ByteBuffer b = ByteBuffer.allocateDirect(buffer.length);
        b.put(buffer);
        IntBuffer transfered = IntBuffer.allocate(1);
        result = LibUsb.bulkTransfer(devh, US_EPOUT, b, transfered, timeout);
        if (result != LibUsb.SUCCESS) throw new LibUsbException("[USBSID] Transfer failed", result);
        result = LibUsb.handleEventsTimeout(null, timeout);
        if (result != LibUsb.SUCCESS) throw new LibUsbException("[USBSID] Unable to handle events", result);
      } catch (LibUsbException LUE) {
        throw new LibUsbException(LUE.getMessage(), LUE.getErrorCode());
      }
    }

    private static byte[] syncReadL(byte[] buffer, int len)
      throws LibUsbException
    {
      /* Beats me why this is, but hey it works ;) */
      len *= 2; /* double the size of the read length for 2nd URB package */
      try {
        ByteBuffer b_out = ByteBuffer.allocateDirect(buffer.length);
        b_out.put(buffer);
        IntBuffer t_out = IntBuffer.allocate(1);
        result = LibUsb.bulkTransfer(devh, US_EPOUT, b_out, t_out, timeout);
        ByteBuffer data = ByteBuffer.allocateDirect(len);
        IntBuffer t_in = IntBuffer.allocate(1);
        int result = LibUsb.bulkTransfer(devh, US_EPIN, data, t_in, timeout);
        if (result != LibUsb.SUCCESS) throw new LibUsbException("[USBSID] Transfer failed", result);
        result = LibUsb.handleEventsTimeout(null, timeout);
        if (result != LibUsb.SUCCESS) throw new LibUsbException("[USBSID] Unable to handle events", result);
        byte[] b_temp = new byte[data.remaining()];
        data.get(b_temp);
        byte[] b_in = Arrays.copyOfRange(b_temp, 0, (len/2));
        return b_in;
      } catch (LibUsbException LUE) {
        throw new LibUsbException(LUE.getMessage(), LUE.getErrorCode());
      }
    }

    private static TransferCallback usb_out = new TransferCallback()
    {
      @Override
      public void processTransfer(Transfer transfer)
      {
        if(transfer.status() != LibUsb.TRANSFER_COMPLETED) {
          int result = transfer.status();
          if (result != LibUsb.TRANSFER_CANCELLED) {
            logger.severe(MessageFormat.format("[USBSID] Warning: transfer out interrupted with status {0}, {0}: {0}\r", result, LibUsb.errorName(result), LibUsb.strError(result)));
          }
          LibUsb.freeTransfer(transfer);
          return;
        }
        if (transfer.actualLength() != len_out_buffer) {
          logger.warning(MessageFormat.format("[USBSID] Sent data length {0} is different from the defined buffer length: {0} or actual length {0}\r", transfer.length(), 64, transfer.actualLength()));
        }
      }
    };

  }

  public static boolean isOpen()
  {
    return us_isOpen;
  }

  public static boolean isWinblows()
  {
    final String OS = System.getProperty("os.name").toLowerCase();
    return OS.contains("win");
  }

  private static void setDriverType()
  {
    if (!isWinblows()) {
      us_isUSBX = true;  /* Perfect for Linux! but no worky on poor Winblows */
    } else {
      us_isLIBUSB = true;  /* Winblows fallback */
    }
  }


  /* Public woohah */

  public static void setdriver_USBSID(String driver)
  {
    switch (driver) {
      case "usbx":
        us_isUSBX = true;
        break;
      case "libusb":
        us_isLIBUSB = true;
        break;
      default:
        us_isUSBX = true;
        break;
    }
    us_overrideDriver = true;
  }

  public static void open_USBSID()
  {
    if (!us_overrideDriver) setDriverType();
    if (us_isUSBX) {
      try {
        USBX.openUSBX();
      } catch (UsbException UE) {
        logger.warning("[USBSID] Exception occured: " + UE);
        UE.printStackTrace();
      }
    } else if (us_isLIBUSB) {
      try {
        USBL.openLIBUSB();
      } catch (LibUsbException LUE) {
        logger.warning("[USBSID] Exception occured: " + LUE);
        LUE.printStackTrace();
      }
    }
  }

  public static void close_USBSID()
  {
    sendCommand(Cmd.RESET_SID.get(), (byte)0x0);
    if (us_isUSBX) {
      try {
        USBX.closeUSBX();
      } catch (UsbException UE) {
        logger.warning("[USBSID] Exception occured: " + UE);
        UE.printStackTrace();
      }
    } else if (us_isLIBUSB) {
      try {
        USBL.closeLIBUSB();
      } catch (LibUsbException LUE) {
        logger.warning("[USBSID] Exception occured: " + LUE);
        LUE.printStackTrace();
      }
    }
  }

  public static void asyncWrite(byte[] buffer)
  {
    if (us_isUSBX) {
      try {
        USBX.asyncWriteX(buffer);
      } catch (UsbException UE) {
        logger.warning("[USBSID] Exception occured: " + UE);
        UE.printStackTrace();
      }
    } else if (us_isLIBUSB) {
      try {
        USBL.asyncWriteL(buffer);
      } catch (LibUsbException LUE) {
        logger.warning("[USBSID] Exception occured: " + LUE);
        LUE.printStackTrace();
      }
    }
  }

  public static void syncWrite(byte[] buffer)
  {
    if (us_isUSBX) {
      try {
        USBX.syncWriteX(buffer);
      } catch (UsbException UE) {
        logger.warning("[USBSID] Exception occured: " + UE);
        UE.printStackTrace();
      }
    } else if (us_isLIBUSB) {
      try {
        USBL.syncWriteL(buffer);
      } catch (LibUsbException LUE) {
        logger.warning("[USBSID] Exception occured: " + LUE);
        LUE.printStackTrace();
      }
    }
  }

  public static byte[] syncRead(byte[] buffer, int len)
  {
    byte[] r = null; // new byte[len];
    if (us_isUSBX) {
      try {
        r = USBX.syncReadX(buffer, len);
      } catch (UsbException UE) {
        logger.warning("[USBSID] Exception occured: " + UE);
        UE.printStackTrace();
      }
    } else if (us_isLIBUSB) {
      try {
        r = USBL.syncReadL(buffer, len);
      } catch (LibUsbException LUE) {
        logger.warning("[USBSID] Exception occured: " + LUE);
        LUE.printStackTrace();
      }
    }
   return r;
  }

  public static void sendCommand(byte command, Byte...subcommands)
  { /* Ignores all extra subcommands if more then 1 is supplied */
    Byte subcommand = subcommands.length > 0 ? subcommands[0] : 0;
    byte[] message = new byte[3];
    message[0] = (byte)((Cmd.COMMAND.get() << 6) | (command)); /* config command */
    message[1] = (byte)subcommand;
    syncWrite(message);
  }

  public static void sendConfigCommand(
    int command,
    Byte...args)
  {
    Byte a = args.length > 0 ? args[0] : 0;
    Byte b = args.length > 1 ? args[1] : 0;
    Byte c = args.length > 2 ? args[2] : 0;
    Byte d = args.length > 3 ? args[3] : 0;

    byte[] message = new byte[6];
    message[0] = (byte) ((Cmd.COMMAND.get() << 6) | Cmd.CONFIG.get()); /* config command */
    message[1] = (byte) (command);
    message[2] = (byte) (a);
    message[3] = (byte) (b);
    message[4] = (byte) (c);
    message[5] = (byte) (d);
    syncWrite(message);
  }

  public static byte[] rwConfigCommand(
    int command,
    int len,
    Byte...args)
  {
    Byte a = args.length > 0 ? args[0] : 0;
    Byte b = args.length > 1 ? args[1] : 0;
    Byte c = args.length > 2 ? args[2] : 0;
    Byte d = args.length > 3 ? args[3] : 0;
    byte[] message = new byte[6];
    message[0] = (byte) ((Cmd.COMMAND.get() << 6) | Cmd.CONFIG.get()); /* config command */
    message[1] = (byte) (command);
    message[2] = (byte) (a);
    message[3] = (byte) (b);
    message[4] = (byte) (c);
    message[5] = (byte) (d);
    byte[] result = null;
    result = syncRead(message, len);
    return result;
  }

}
