/* WebUSB constants */
const CDC_CLASS     = 0x0A; /* Interface 1 */
const DEVICE_CLASS  = 0xFF; /* Interface 4 */
const CTRL_TRANSFER = 0x22;
const CTRL_ENABLE   = 0x01;
const CTRL_DISABLE  = 0x00;

/* USBSID constants */
const USBSID_VID = 0xcafe;
const USBSID_PID = 0x4011;

var device = {};
var us;

async function requestDevice () {
  device = await navigator.usb.requestDevice({
    filters: [
      {
        vendorId: USBSID_VID,
        productId: USBSID_PID,
      },
    ],
  });
}

function connect() {
  us = new Module.USBSID_Class();
  us.USBSID_Init(true,true);
}

function disconnect() {
  us.delete();
}

function setResult(data) {
  var result = document.getElementById("result");
  result.textContent = data;
}

function SetClockRate() {
  var clockrate_cycles = 985248;
  var suspend_sids = true;
  us.USBSID_SetClockRate(clockrate_cycles, suspend_sids);
}

function GetClockRate() {
  Promise.resolve(us.USBSID_GetClockRate()).then(function(result) {
    setResult(result);
  });
}

function GetRefreshRate() {
  var rr = us.USBSID_GetRefreshRate();
  setResult(rr);
}

function GetRasterRate() {
  var rr = us.USBSID_GetRasterRate();
  setResult(rr);
}
