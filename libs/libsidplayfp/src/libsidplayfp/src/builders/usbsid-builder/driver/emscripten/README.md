# Tests, don't use in production :)
This directory contains an edited copy of the driver for test use with emscripten and libusb

# Brainfarts

# webusb incoming control transfers on connect

```sh
[tud_vendor_control_xfer_cb] stage:1, rhport:0, bRequest:0xb, wValue:0, wIndex:4, wLength:0, bmRequestType:1, type:0, recipient:1, direction:0
[tud_vendor_control_xfer_cb] stage:1, rhport:0, bRequest:0x22, wValue:1, wIndex:4, wLength:0, bmRequestType:21, type:1, recipient:1, direction:0
[tud_vendor_control_xfer_cb] stage:3, rhport:0, bRequest:0x22, wValue:1, wIndex:4, wLength:0, bmRequestType:21, type:1, recipient:1, direction:0
```

# cdc incoming control transfers on connect
```sh
[tud_cdc_line_state_cb] itf:0, dtr:1, rts:1
[tud_cdc_line_coding_cb] itf:0, bit_rate:9000000, stop_bits:0, parity:0, data_bits:8
```

# libusb control_transfer
```c
libusb_control_transfer(
  libusb_device_handle *dev_handle,
	uint8_t request_type, 
  uint8_t bRequest, 
  uint16_t wValue, 
  uint16_t wIndex,
	unsigned char *data, 
  uint16_t wLength, 
  unsigned int timeout
);
```

# testlibusb.c
```js
navigator.usb.requestDevice({
  filters: [
    {
      vendorId: 0xcafe,
      productId: 0x4011,
    },
  ],
})
backend_SID.Module._runmeonmyass(['-v'])
```

# usbsid
```js
var device = await navigator.usb.requestDevice({
  filters: [
    {
      vendorId: 0xcafe,
      productId: 0x4011,
    },
  ],
});
var us = new Module.USBSID_Class();
us.USBSID_Init(true,true);

```

# websid
```js
var device = await navigator.usb.requestDevice({
  filters: [
    {
      vendorId: 0xcafe,
      productId: 0x4011,
    },
  ],
});
var us = new backend_SID.Module.USBSID_Class();
us.USBSID_Init(true,true);
```
