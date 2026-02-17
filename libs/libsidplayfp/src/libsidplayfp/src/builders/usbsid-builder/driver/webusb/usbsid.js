/* WebUSB constants */
const CDC_CLASS     = 0x0A; /* Interface 1 */
const DEVICE_CLASS  = 0xFF; /* Interface 4 */
const CTRL_TRANSFER = 0x22;
const CTRL_ENABLE   = 0x01;
const CTRL_DISABLE  = 0x00;

/* USBSID constants */
const USBSID_VID = 0xcafe;
const USBSID_PID = 0x4011;

/* Buffer constants */
const BUFFER_SIZE       = 64; /* 64KB for one USB packet */
const MAX_PACKET_SIZE   = 64; /* Absolute maximum amount of bytes per packet */
const MAX_WRITE_BYTES   =  3; /* 1 Start byte, 1 register, 1 value */
const MAX_CYCLED_BYTES  =  5; /* 1 Start byte, 1 register, 1 value, 1 cycles_hi, 1 cycles_lo */
const MAX_WRITE_BUFFER  = 63; /* 1 Start byte, 62 / 2 writes */
const MAX_CYCLED_BUFFER = 61; /* 1 Start byte, 60 / 4 cycled writes */

/* USBSID Command byte constants */
/* BYTE 0 - top 2 bits */
const WRITE        =  0; /*        0b0 ~ 0x00 */
const READ         =  1; /*        0b1 ~ 0x40 */
const CYCLED_WRITE =  2; /*       0b10 ~ 0x80 */
const COMMAND      =  3; /*       0b11 ~ 0xC0 */
/* BYTE 0 - lower 5 bits for Commands */
const PAUSE        = 10; /*     0b1010 ~ 0x0A */
const UNPAUSE      = 11; /*     0b1011 ~ 0x0B */
const MUTE         = 12; /*     0b1100 ~ 0x0C */
const UNMUTE       = 13; /*     0b1101 ~ 0x0D */
const RESET_SID    = 14; /*     0b1110 ~ 0x0E */
const DISABLE_SID  = 15; /*     0b1111 ~ 0x0F */
const ENABLE_SID   = 16; /*    0b10000 ~ 0x10 */
const CLEAR_BUS    = 17; /*    0b10001 ~ 0x11 */
const CONFIG       = 18; /*    0b10010 ~ 0x12 */
const RESET_MCU    = 19; /*    0b10011 ~ 0x13 */
const BOOTLOADER   = 20; /*    0b10100 ~ 0x14 */
/* BYTE 0 - lower 6 bits for byte count */

/* USBSID WEBUSB Command constants */
const WEBUSB_COMMAND  = 0xFF;
const WEBUSB_RESET    = 0x15;
const WEBUSB_CONTINUE = 0x16;

/* USBSID Config constants */
const RESET_USBSID     = 0x20;
const READ_CONFIG      = 0x30;
const APPLY_CONFIG     = 0x31;
const STORE_CONFIG     = 0x32;
const SAVE_CONFIG      = 0x33;
const SAVE_NORESET     = 0x34;
const RESET_CONFIG     = 0x35;
const SINGLE_SID       = 0x40;
const DUAL_SID         = 0x41;
const QUAD_SID         = 0x42;
const TRIPLE_SID       = 0x43;
const SET_CLOCK        = 0x50;
const DETECT_SIDS      = 0x51;
const TEST_ALLSIDS     = 0x52;
const TEST_SID1        = 0x53;
const TEST_SID2        = 0x54;
const TEST_SID3        = 0x55;
const TEST_SID4        = 0x56;
const LOAD_MIDI_STATE  = 0x60;
const SAVE_MIDI_STATE  = 0x61;
const RESET_MIDI_STATE = 0x63;
const USBSID_VERSION   = 0x80;
const TOGGLE_AUDIO     = 0x88; /* Toggle mono <-> stereo (v1.3+ boards only) */
const SET_AUDIO        = 0x89; /* Set mono <-> stereo (v1.3+ boards only) */

/* USBSID SID related constants */
const clock_rates = {
  DEFAULT: 0, /* 1000000 */
  PAL:     1, /*  985248 */
  NTSC:    2, /* 1022727 */
  DREAN:   3, /* 1023440 */
};

/* Global delay function */
const us_delay = (ms) => new Promise((resolve) => setTimeout(resolve, ms));

/* USBSID main class */
class USBSID {
  constructor(classtype = DEVICE_CLASS, async_await = true, buffered = true) {
    this.interfaces;
    this.device = {};
    this.classtype = classtype;
    this.async_await = async_await;
    this.buffered = buffered;
  }

  async webusb_findinterface(connectType) {
    this.interfaces = this.device.configuration.interfaces;
    this.interfaces.forEach((element) => {
      element.alternates.forEach((elementalt) => {
        if (elementalt.interfaceClass == connectType) {
          this.device.interfaceNumber = element.interfaceNumber;
          elementalt.endpoints.forEach((elementendpoint) => {
            if (elementendpoint.direction == "out") {
              this.device.endpointOut = elementendpoint.endpointNumber;
            }
            if (elementendpoint.direction == "in") {
              this.device.endpointIn = elementendpoint.endpointNumber;
            }
          });
        }
      });
    });
  }

  async init(dev) {
    try {
      const self = this;

      if (typeof dev !== "undefined") {
        self.device = dev;
      } else {
        self.device = await navigator.usb.requestDevice({
          filters: [
            {
              vendorId: USBSID_VID,
              productId: USBSID_PID,
            },
          ],
        });

        if (self.device == null) {
          throw new Error("Could not find USBSID-Pico");
        }
        await self.device.open();
        console.log("USBSID-Pico opened:", self.device.opened);
      }
      console.log("?" + self.device);

      self.isClosing = false;
      if (typeof self.device !== "undefined") {
        if (self.device.configuration === null) {
          console.log("selectConfiguration");
          await self.device.selectConfiguration(1);
        }
        console.log("Start device claim");
        await self.webusb_findinterface(self.classtype);
        await self.device.claimInterface(self.device.interfaceNumber);
        await self.device.selectConfiguration(1);
        await self.device.selectAlternateInterface(self.device.interfaceNumber, 0);
        /* enable the interface */
        if (self.classtype === DEVICE_CLASS) {
          await self.device.controlTransferOut({
            requestType: "class",
            recipient: "interface",
            request: CTRL_TRANSFER,
            value: CTRL_ENABLE,
            index: self.device.interfaceNumber,
          });
        }
      }
    } catch (err) {
      console.error(err);
    }
  }

  async write(buffer) {
    const result = this.device.transferOut(this.device.endpointOut, buffer);
    if (this.async_await) {
      const r = await Promise.resolve(result);
      return r;
    }
  }

  async close() {
    this.isClosing = true;
    try {
      await this.device.releaseInterface(this.device.interfaceNumber);
      await this.device.close();
      console.log("Closed device");
    } catch (err) {
      console.log("Error:", err);
    }
  }

  isOpen() {
    return this.device.opened;
  }
}

/** ----- USBSID-Pico ----- */

/** Set player type here */
const sidplayers = ["hermit", "jsidplay2"];
const PLAYER = sidplayers[1];
/** Cycle exact writing
 * `true`  ~ each write is 4 bytes long
 * `false` ~ each write is 2 bytes long
 */
const CYCLE_EXACT = true;
/** Buffer writes before sending
 * `true`  ~ writes will build up until full or flushed
 * `false` ~ writes will be sent immediately
 */
const BUFFERED_WRITES = true;

/* Variables */
var max_buffer_size = CYCLE_EXACT && BUFFERED_WRITES ? MAX_CYCLED_BUFFER : MAX_WRITE_BUFFER;
var write_cmd = (CYCLE_EXACT ? CYCLED_WRITE : WRITE) << 6;
var usbsid, max_packet_size;
var backbuf = new Array(BUFFER_SIZE);
var backbufIdx = 1;
var bufferQueue = new USBSID_queue();
var rasterrate = 19656; /* 20000 */

var c64clock = performance.now();
var c64clock_now = performance.now();
var flush_buff = 0;
var cycledbuffer = new Array(MAX_CYCLED_BYTES);
var cycledbuffer2 = new Array(MAX_CYCLED_BYTES);
var cbIdx = 1,
  cbIdx2 = 1;
var cbWrDone = 1,
  cbWrDone2 = 1;

/** ----- main functions ----- */

/** Helper `thread` that flushes data if timer exceeds rasterrate */
async function USBSID_timer() {
  c64clock_now = performance.now();
  if (c64clock_now - c64clock >= rasterrate / 1000) {
    c64clock = c64clock_now;
    if (cbWrDone == 1 && cbIdx > 1) {
      cycledbuffer[0] = (CYCLED_WRITE << 6) | (cbIdx - 1);
      const result = usbsid.write(new Uint8Array(cycledbuffer.slice(0, cbIdx)));
      if (usbsid.async_await) {
        const r = await Promise.resolve(result);
        return r;
      }
      cbIdx = 1;
    }
  }
  timer2 = setTimeout(() => USBSID_timer());
}

/** Internal write thread ~ loops */
async function USBSID_threadOutput() {
  var bufferFrame;
  while (bufferQueue.isNotEmpty()) {
    bufferFrame = bufferQueue.dequeue();
    /* exit condition */
    if (bufferFrame.bufferIdx < 0) {
      timer = null;
      // timer2 = null;
      bufferQueue.clear();
      return;
    }
    const result = uSwrite(bufferFrame.buffer, max_packet_size);
    if (usbsid.async_await) {
      const r = await Promise.resolve(result);
      return r;
    }
  }
  /* restart the timer */
  timer = setTimeout(() => USBSID_threadOutput());
}

/** Internal write to class function */
async function uSwrite(buff, size) {
  try {
    const result = usbsid.write(new Uint8Array(buff.slice(0, size)));
    if (usbsid.async_await) {
      const r = await Promise.resolve(result);
      return r;
    }
  } catch (error) {}
}

/** Init USBSID-Pico device
 * required: device
 * optional:
 * classtype, defaults to DEVICE_CLASS
 * async, defaults to true
 * buffered, defaults to true
 */
async function USBSID_init(
  device,
  async_await = true,
  buffered = true,
  selfbuffered = false,
  classtype = DEVICE_CLASS
) {
  if (usbsid && usbsid.device && usbsid.isOpen()) {
    console.log("Device is already open!");
    return -1;
  }
  try {
    usbsid = new USBSID(classtype, async_await, buffered);
    await usbsid.init(device);
    max_packet_size = usbsid.buffered ? max_buffer_size : CYCLE_EXACT ? MAX_CYCLED_BYTES : MAX_WRITE_BYTES;
    backbufIdx = 1;
    bufferQueue.clear();
    if (buffered) timer = setTimeout(() => USBSID_threadOutput());
    if (selfbuffered) timer2 = setTimeout(() => USBSID_timer());

    return 0;
  } catch (err) {
    console.log(err);
    return -1;
  }
}

/** De-init USBSID-Pico device */
async function USBSID_deinit() {
  if (usbsid) {
    USBSID_reset(0);

    uSoutb(0, -1); /* signal end of thread */

    timer = null;
    timer2 = null;
    await USBSID_close();
    device = undefined;
  }
  clkdrift = 0;
  ftdi = undefined;
}

/** Buffer out queue function */
function uSoutb(b, flush) {
  backbuf[backbufIdx++] = b;

  if (backbufIdx < max_buffer_size && flush == 0) return;
  backbuf[0] = write_cmd | (backbufIdx - 1);

  if (flush < 0)
    /* indicate exit request */
    bufferQueue.enqueue({
      buffer: [...backbuf],
      bufferIdx: -1,
    });
  else {
    bufferQueue.enqueue({
      buffer: [...backbuf],
      bufferIdx: backbufIdx,
    });
    backbufIdx = 1;
  }
}

/** Non cycled buffered write function */
function USBSID_write(chip, addr, data, flush) {
  uSoutb(calculate_chip_address(chip, addr), 0);
  uSoutb(data, flush);
}

/** Cycled buffered write function */
function USBSID_clkdwrite(cycles, chip, addr, data) {
  uSoutb(calculate_chip_address(chip, addr), 0);
  uSoutb(data, 0);
  let cycles_hi = (cycles >> 8) & 0xff;
  let cycles_lo = cycles & 0xff;
  uSoutb(cycles_hi, 0);
  uSoutb(cycles_lo, backbufIdx == max_buffer_size ? 1 : 0);
}

/** Cycled self buffered write function (fastest) */
async function USBSID_clkdbufwrite(cycles, chip, addr, data) {
  cbWrDone = 0;
  cycledbuffer[cbIdx++] = calculate_chip_address(chip, addr);
  cycledbuffer[cbIdx++] = data;
  let cycles_hi = (cycles >> 8) & 0xff;
  let cycles_lo = cycles & 0xff;
  cycledbuffer[cbIdx++] = cycles_hi;
  cycledbuffer[cbIdx++] = cycles_lo;
  cbWrDone = 1;

  if (cbIdx < max_buffer_size) return;
  cbWrDone = 0;
  cycledbuffer[0] = (CYCLED_WRITE << 6) | (cbIdx - 1);
  const result = usbsid.write(new Uint8Array(cycledbuffer.slice(0, cbIdx)));
  cbIdx = 1;
  cbWrDone = 1;
  if (usbsid.async_await) {
    const r = await Promise.resolve(result);
    return r;
  }
}

/** Cycled direct write function ~ slow but functional */
async function USBSID_cycledwrite(cycles, chip, addr, data) {
  var writebuffer = new Array(MAX_CYCLED_BYTES);
  var wbIdx = 1;
  writebuffer[wbIdx++] = calculate_chip_address(chip, addr);
  writebuffer[wbIdx++] = data;
  let cycles_hi = (cycles >> 8) & 0xff;
  let cycles_lo = cycles & 0xff;
  writebuffer[wbIdx++] = cycles_hi;
  writebuffer[wbIdx++] = cycles_lo;
  writebuffer[0] = (CYCLED_WRITE << 6) | (backbufIdx - 1);
  wbIdx = 1;
  const result = usbsid.write(new Uint8Array(writebuffer.slice(0, MAX_CYCLED_BYTES)));
  if (usbsid.async_await) {
    const r = await Promise.resolve(result);
    return r;
  }
}

/** ----- Util functions ----- */

/** Command write function */
function USBSID_cmdwrite(command) {
  try {
    uSwrite([(COMMAND << 6) | command], 3);
  } catch (err) {
    console.error(err);
  }
}

/** Config write function */
function USBSID_configwrite(command, a = 0, b = 0, c = 0, d = 0, e = 0) {
  try {
    uSwrite([(COMMAND << 6) | CONFIG, command, a, b, c, d, e], 6);
  } catch (err) {
    console.error(err);
  }
}

/** Internal close function */
async function USBSID_close() {
  await usbsid.close();
}

/** Returns true or false if playing */
function USBSID_is_playing() {
  return bufferQueue.isNotEmpty();
}

/** Reset function */
function USBSID_reset(volume) {
  try {
    bufferQueue.clear();
    USBSID_cmdwrite(RESET_SID);
    /* sleep for 50us */
    us_delay(50); /* wait for reset to complete */

    USBSID_write(0, 0x18, volume, 1);
    USBSID_write(1, 0x18, volume, 1);
    USBSID_write(3, 0x18, volume, 1);
    USBSID_write(4, 0x18, volume, 1);

    clkdrift = 0;
    us_delay(250); /* wait for send/receive to complete */

    backbufIdx = 1;
  } catch (err) {
    console.error(err);
  }
}

/** Set USBSID clockrate */
function USBSID_setclock(clockrate) {
  try {
    USBSID_configwrite(SET_CLOCK, clockrate);
  } catch (err) {
    console.error(err);
  }
}

/** Pause USBSID */
function USBSID_pause() {
  try {
    USBSID_cmdwrite(PAUSE);
  } catch (err) {
    console.error(err);
  }
}

/** Set Mono/Stereo */
function USBSID_setaudio(stereo) {
  try {
    USBSID_configwrite(SET_AUDIO, stereo);
  } catch (err) {
    console.error(err);
  }
}

/** Toggle Mono/Stereo */
function USBSID_toggleaudio() {
  try {
    USBSID_configwrite(TOGGLE_AUDIO);
  } catch (err) {
    console.error(err);
  }
}

/** ----- Helper functions ----- **/

function jsidplay2_chip_address(chip, addr) {
  /* chip can be one of:
       0x20  32 00100000
     - 0x2D  45 00101101
     I 0x49  73 01001001 => 1?
     a 0x61  97 01100001 => 0?
     s 0x73 115 01110011 => 2?
  */
  /* Nasty workaround for getting the correct chipno! */
  addr =
    chip == "a"
      ? (0 * 0x20) | addr
      : chip == "I"
      ? (1 * 0x20) | addr
      : chip == " "
      ? (1 * 0x20) | addr
      : chip == "s"
      ? (2 * 0x20) | addr
      : chip == "-"
      ? (2 * 0x20) | addr
      : (0 * 0x20) | addr;
  return addr;
}

function default_chip_address(chip, addr) {
  return (chip * 0x20) | addr;
}

function calculate_chip_address(chip, addr) {
  switch (PLAYER) {
    case "jsidplay2":
      return jsidplay2_chip_address(chip, addr);
    default:
      return default_chip_address(chip, addr);
  }
}

/** The actual queue here peoples */
function USBSID_queue() {
  var head, tail;
  return Object.freeze({
    enqueue(value) {
      const link = { value, next: undefined };
      tail = head ? (tail.next = link) : (head = link);
    },
    dequeue() {
      if (head) {
        const value = head.value;
        head = head.next;
        return value;
      }
    },
    dequeueAll() {
      var dequeued = {
        head: head,
        tail: tail,
      };
      tail = head = undefined;
      return dequeued;
    },
    peek() {
      return head?.value;
    },
    clear() {
      tail = head = undefined;
    },
    isNotEmpty() {
      return head;
    },
  });
}
