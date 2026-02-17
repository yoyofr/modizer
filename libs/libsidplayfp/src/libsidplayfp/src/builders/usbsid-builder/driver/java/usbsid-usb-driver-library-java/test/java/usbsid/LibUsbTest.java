package java.usbsid;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.IntBuffer;
import java.util.function.Supplier;
import java.util.logging.Logger;

import org.testng.annotations.Test;
import org.usb4java.Device;
import org.usb4java.DeviceDescriptor;
import org.usb4java.DeviceHandle;
import org.usb4java.DeviceList;
import org.usb4java.LibUsb;

public class LibUsbTest {

  private static Logger logger = null;
  static {
  System.setProperty("java.util.logging.SimpleFormatter.format",
              "[%1$tF %1$tT] [USBSID %4$-7s] %5$s %n");
  logger = Logger.getLogger(LibUsbTest.class.getName());
  }


  private static final byte USB_INTERFACE = (byte) 0x1;
  private boolean error = false;
  private DeviceHandle[] devhandles = new DeviceHandle[4];
  // private boolean[] deviceTypes = new boolean[4];
  private byte lastaccsids[] = new byte[4];
  private ByteBuffer writeBuffer[] = new ByteBuffer[4];
  private static int deviceCount = 0;
  private final int bufferSize = 1024;

  private boolean checkforError(Supplier<Boolean> rcCheck, String info) {
		if (rcCheck.get()) {
			logger.info(info + " OK");
		} else {
			logger.severe(info + " FAILED");
			error = true;
		}
		return error;
	  }

  @Test
  private void openAllDevices() {
		if (checkforError(() -> LibUsb.init(null) == 0, "USB Init"))
			return;

		DeviceList devices = new DeviceList();

		if (checkforError(() -> LibUsb.getDeviceList(null, devices) != 0, "USB Get device list"))
			return;

		for (Device device : devices) {

			DeviceDescriptor descriptor = new DeviceDescriptor();

			if (checkforError(() -> LibUsb.getDeviceDescriptor(device, descriptor) == 0, "USB Get device descriptor"))
				return;

      logger.info(String.format("VID: 0x%04X PID: 0x%04X", descriptor.idVendor(), descriptor.idProduct()));

			// DevType devType = getDevType(descriptor);
			// if (devType != DevType.UNKNOWN) {

			if (descriptor.idVendor() == (short)0xCAFE) {
      // final boolean devType = true;
			// if (devType) {

				DeviceHandle handle = new DeviceHandle();

				if (checkforError(() -> LibUsb.open(device, handle) == 0, "USB Open"))
					return;

				if (LibUsb.hasCapability(LibUsb.CAP_SUPPORTS_DETACH_KERNEL_DRIVER) && checkforError(
						() -> LibUsb.kernelDriverActive(handle, USB_INTERFACE) == 0, "USB Kernel driver active")) {

					if (checkforError(() -> LibUsb.detachKernelDriver(handle, USB_INTERFACE) == 0,
							"USB Detach kernel driver"))
						return;
				}

				if (checkforError(() -> LibUsb.claimInterface(handle, USB_INTERFACE) == 0, "USB Claim interface"))
					return;

				IntBuffer config = IntBuffer.allocate(1);

				if (checkforError(() -> LibUsb.getConfiguration(handle, config) == 0, "USB Get configuration"))
					return;
        logger.info(String.format("Configuration: %d", config.get()));
        config.rewind();

				if (config.get() == 0) {
					if (checkforError(() -> LibUsb.setConfiguration(handle, 1) == 0, "USB Set configuration"))
						return;
//					if (checkforError(() -> LibUsb.setInterfaceAltSetting(handle, USB_INTERFACE, 1) == 0,
//							"USB Interface alt setting"))
//						return;
				}


				devhandles[deviceCount] = handle;
				// deviceTypes[deviceCount] = devType;
				lastaccsids[deviceCount] = (byte) 0xff;
				writeBuffer[deviceCount] = ByteBuffer.allocate(bufferSize).order(ByteOrder.LITTLE_ENDIAN);
				deviceCount++;
				break;
			}
		}
		LibUsb.freeDeviceList(devices, true); // usually in a finally block
	}

}
