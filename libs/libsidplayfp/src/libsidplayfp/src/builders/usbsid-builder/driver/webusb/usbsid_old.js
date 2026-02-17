/*
 * USBSID-Pico is a RPi Pico (RP2040) based board for interfacing one or two
 * MOS SID chips and/or hardware SID emulators over (WEB)USB with your computer,
 * phone or ASID supporting player
 *
 * usbsid.js
 * This file is part of USBSID-Pico (https://github.com/LouDnl/USBSID-Pico)
 * File author: LouD
 *
 * Copyright (c) 2024-2025 LouD
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */

/* Open Issues & Bugs:
 * every write returns a confirmation, without await this confirmation comes way later, causing a delay
 * disabling return wait makes perfect play but up to 30~60 second delay after stop, new tune or reset
 * enabling return wait causes slow play and stuttering but immediate reset etc.
 *
 * Breaking connection prematurely during play causes massive spam and or computer lock up due to this spam:
 * 64jukebox.vue:1 Uncaught (in promise) NetworkError: Failed to execute 'transferOut' on 'USBDevice': A transfer error has occurred.Understand this errorAI
 * c64jukebox.vue:1 Uncaught (in promise) AbortError: Failed to execute 'transferOut' on 'USBDevice': The transfer was cancelled.
 */

/* Set player type here */
const sidplayers = ['hermit', 'jsidplay2'];
const PLAYER = sidplayers[1];

/* WebUSB Constants */
const USBSID_VID    = 0xcafe;
const USBSID_PID    = 0x4011;
const DEVICE_CLASS  = 0xff;
const CTRL_TRANSFER = 0x22;
const CTRL_ENABLE   = 0x01;
const CTRL_DISABLE  = 0x00;
/* Setting these at lets say 256 works, but for each 64KB a URB_SUCCESS is returned */
const BUFFER_SIZE      = 64;  /* 64KB for one USB packet */
const PACKET_SIZE      = 64;  /* Used in usbsid_threadoutput */
const MAX_WRITE_BYTES  =  3;  /* 1 Start byte, 1 register, 1 value */
const MAX_CYCLED_BYTES = 61;  /* 1 Start byte, 60 / 4 cycled writes */
/* WebUSB variables */
var interfaces;
var usbsid = {};
var initialized = false,
    allow_play = false,
    error = false,
    cycled = true,
    async = false;
/* Buffer */
var backbuf, bufferQueue;
var backbufIdx = 1,
    flush = 0;

/* USBSID Command byte constants */
/* BYTE 0 - top 2 bits */
const WRITE        =   0;   /*        0b0 ~ 0x00 */
const READ         =   1;   /*        0b1 ~ 0x40 */
const CYCLED_WRITE =   2;   /*       0b10 ~ 0x80 */
const COMMAND      =   3;   /*       0b11 ~ 0xC0 */
/* BYTE 0 - lower 5 bits for Commands */
const PAUSE        =  10;   /*     0b1010 ~ 0x0A */
const UNPAUSE      =  11;   /*     0b1011 ~ 0x0B */
const MUTE         =  12;   /*     0b1100 ~ 0x0C */
const UNMUTE       =  13;   /*     0b1101 ~ 0x0D */
const RESET_SID    =  14;   /*     0b1110 ~ 0x0E */
const DISABLE_SID  =  15;   /*     0b1111 ~ 0x0F */
const ENABLE_SID   =  16;   /*    0b10000 ~ 0x10 */
const CLEAR_BUS    =  17;   /*    0b10001 ~ 0x11 */
const CONFIG       =  18;   /*    0b10010 ~ 0x12 */
const RESET_MCU    =  19;   /*    0b10011 ~ 0x13 */
const BOOTLOADER   =  20;   /*    0b10100 ~ 0x14 */
/* BYTE 0 - lower 6 bits for byte count */

/* USBSID WEBUSB Command constants */
const WEBUSB_COMMAND  = 0xFF;
const WEBUSB_RESET    = 0x15;
const WEBUSB_CONTINUE = 0x16;

/* USBSID Config constants */
const clock_rates = {
  DEFAULT: 0, /* 1000000 */
  PAL:     1, /*  985248 */
  NTSC:    2, /* 1022727 */
  DREAN:   3, /* 1023440 */
};
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


/* WebUSB internal functions */

async function webusb_requestdev(device) {
  if (typeof device !== "undefined") {
    usbsid = device;
  } else {
    usbsid = await navigator.usb.requestDevice({
      filters: [
        {
          vendorId: USBSID_VID,
          productId: USBSID_PID,
        },
      ],
    });
  }
}

async function webusb_findinterface() {
  interfaces = usbsid.configuration.interfaces;
  interfaces.forEach((element) => {
    element.alternates.forEach((elementalt) => {
      if (elementalt.interfaceClass == DEVICE_CLASS) {
        usbsid.interfaceNumber = element.interfaceNumber;
        elementalt.endpoints.forEach((elementendpoint) => {
          if (elementendpoint.direction == "out") {
            usbsid.endpointOut = elementendpoint.endpointNumber;
          }
          if (elementendpoint.direction == "in") {
            usbsid.endpointIn = elementendpoint.endpointNumber;
          }
        });
      }
    });
  });
}

async function webusb_open() {
  if (usbsid !== undefined) {
    console.log(`Product name: ${usbsid.productName}, Product Id: ${usbsid.productId.toString(16)}`);
    try {
      /* open the device */
      await usbsid.open();

      /* select config 1 */
      await usbsid.selectConfiguration(1);

      /* find the vendor class webusb interface to connect to */
      await webusb_findinterface();

      /* claim the interface */
      await usbsid.claimInterface(usbsid.interfaceNumber);
      /* select the interface */
      await usbsid.selectAlternateInterface(usbsid.interfaceNumber, 0);
      /* enable the interface */
      await usbsid.controlTransferOut({
        requestType: "class",
        recipient: "interface",
        request: CTRL_TRANSFER,
        value: CTRL_ENABLE,
        index: usbsid.interfaceNumber,
      });
      // usbsid.transferIn(usbsid.interfaceNumber, 64); // flush buffer
      if (usbsid.opened) {
        initialized = true;
        error = false;
      }
    } catch (error) {
      console.error(error);
      error = true;
    }
  }
}


/* USBSID-Pico functions */

async function usbsid_connect(device) {
  if (error) {
    error = false;
  }
  if (initialized) {
    await usbsid_disconnect();
  }
  await webusb_requestdev(device);
  await webusb_open();
}

async function usbsid_disconnect() {
  try {
    if (initialized) {
      await usbsid.controlTransferOut({
        requestType: "class",
        recipient: "interface",
        request: CTRL_TRANSFER,
        value: CTRL_DISABLE,
        index: usbsid.interfaceNumber,
      });
      /* await usbsid.reset(); */
      await usbsid.releaseInterface(usbsid.interfaceNumber);
      await usbsid.close();
      initialized = false;
      error = false;
    }
  } catch (error) {
    console.error(error);
    error = true;
  }
}

async function usbsid_init(device, start_async = false, start_cycled = true) {
  async = start_async;
  cycled = start_cycled;
  try {
    await usbsid_connect(device);
    if (initialized) {
      allow_play = true;
      backbuf = new Array(BUFFER_SIZE);
      bufferQueue = new usbsid_queue();
      backbufIdx = 1;
      flush = 0;
      bufferQueue.clear();

      timer = setTimeout(() => usbsid_threadoutput());
      return 0;
    }
  } catch (error) {
    console.error(error);
    error = true;
    return -1;
  }
}

async function usbsid_close() {
  try {
    backbuf = undefined;
    bufferQueue = undefined;
    return await usbsid_disconnect();
  } catch (error) {
    console.error(error);
    error = true;
  }
}


/* Write functions */

async function usbsid_write_direct(data) {
  if (!initialized || error) {
    return error;
  }
  if (allow_play)
  try {
    /* Temporary delay workaround ~ creates possibility to choose for the delay or not */
    if (async) {
      return await usbsid.transferOut(usbsid.endpointOut, data);
    } else if (!async) {
      usbsid.transferOut(usbsid.endpointOut, data);
    }
  } catch (error) {}
}

async function usbsid_write_array(array) {
  if (allow_play)
  try {
    return await usbsid_write_direct(new Uint8Array(array));
  } catch (error) {
    console.error(error);
    error = true;
  }
}

async function usbsid_write_buffer(buffer, size) {
  if (!initialized || error) {
    return error;
  }
  if (allow_play)
  try {
    /* Temporary delay workaround ~ creates possibility to choose for the delay or not */
    if (async) {
      return await usbsid_write_direct(new Uint8Array(buffer.slice(0, size)));
    } else if (!async) {
      usbsid_write_direct(new Uint8Array(buffer.slice(0, size)));
    }
  } catch (error) {
    console.error(error);
    error = true;
  }
}

async function usbsid_threadoutput() {
  if (initialized && allow_play) {
    var bufferFrame;
    while (bufferQueue.isNotEmpty()) {
      bufferFrame = bufferQueue.dequeue();
      // exit condition
      if (bufferFrame.bufferIdx < 0) {
        timer = null;
        bufferQueue.clear();
        return;
      }
      /* Temporary delay workaround ~ creates possibility to choose for the delay or not */
      if (async) {
        await usbsid_write_buffer(bufferFrame.buffer, PACKET_SIZE);
      } else if (!async) {
        usbsid_write_buffer(bufferFrame.buffer, PACKET_SIZE);
      }
    }
    /* restart the timer */
    timer = setTimeout(() => usbsid_threadoutput());
  }
}

async function usbsid_write(chip, addr, data) {
  // var address = calculate_chip_address(chip, addr);
  // return await usbsid_write_array([(WRITE << 6), address, data])

  backbuf[backbufIdx++] = calculate_chip_address(chip, addr);
  backbuf[backbufIdx++] = data;
  if (backbufIdx == MAX_WRITE_BYTES) flush = 1;
  backbuf[0] = (WRITE << 6) | 0;
  bufferQueue.enqueue({
    buffer: [...backbuf],
    bufferIdx: backbufIdx,
  });
  backbufIdx = 1;
}

async function usbsid_writecycled(cycles, chip, addr, data) {
  backbuf[backbufIdx++] = calculate_chip_address(chip, addr);;
  backbuf[backbufIdx++] = data;
  let cycles_hi = (cycles >> 8) & 0xff;
  let cycles_lo = cycles & 0xff;
  backbuf[backbufIdx++] = cycles_hi;
  backbuf[backbufIdx++] = cycles_lo;
  if (backbufIdx < MAX_CYCLED_BYTES) return;
  if (backbufIdx == MAX_CYCLED_BYTES) flush = 1;
  backbuf[0] = (CYCLED_WRITE << 6) | (backbufIdx - 1);
  bufferQueue.enqueue({
    buffer: [...backbuf],
    bufferIdx: backbufIdx,
  });
  backbufIdx = 1;
}

async function usbsid_config_write(command, a = 0, b = 0, c = 0, d = 0) {
  try {
    return await usbsid_write_array([(COMMAND << 6 | CONFIG), command, a, b, c, d]);
  } catch (error) {
    console.error(error);
    error = true;
  }
}

async function usbsid_setclock(clockrate) {
  try {
    return await usbsid_config_write(SET_CLOCK, clockrate);
  } catch (error) {
    console.error(error);
    error = true;
  }
}


/* Control functions */

async function usbsid_pause() {
  allow_play = false;
  bufferQueue.clear();
  usbsid.controlTransferOut({
    requestType: "class",
    recipient: "device",
    request: WEBUSB_COMMAND,
    value: PAUSE,
    index: usbsid.interfaceNumber,
  });
}

async function usbsid_continue() {
  allow_play = true;
  bufferQueue.clear();
  usbsid.controlTransferOut({
    requestType: "class",
    recipient: "device",
    request: WEBUSB_COMMAND,
    value: WEBUSB_CONTINUE,
    index: usbsid.interfaceNumber,
  });
}

async function usbsid_reset() {
  allow_play = false;
  bufferQueue.clear();

  usbsid.controlTransferOut({
    requestType: "class",
    recipient: "device",
    request: WEBUSB_COMMAND,
    value: WEBUSB_RESET,  /* (unmutes aswell) */
    index: usbsid.interfaceNumber,
  });

  delay(250);
  backbufIdx = 1;
  allow_play = true;
}

async function usbsid_is_playing() {
  return bufferQueue.isNotEmpty();
}


/* Helper functions */

function jsidplay2_chip_address(chip, addr) {
  /* chip can be one of:
       0x20  32 00100000
     - 0x2D  45 00101101
     I 0x49  73 01001001 => 1?
     a 0x61  97 01100001 => 0?
     s 0x73 115 01110011 => 2?
  */
  /* Nasty workaround for getting the correct chipno! */
  addr = chip == 'a'
    ? ((0 * 0x20) | addr)
    : chip == 'I'
    ? ((1 * 0x20) | addr)
    : chip == ' '
    ? ((1 * 0x20) | addr)
    : chip == 's'
    ? ((2 * 0x20) | addr)
    : chip == '-'
    ? ((2 * 0x20) | addr)
    : ((0 * 0x20) | addr);
  return addr;
}

function default_chip_address(chip, addr) {
  return ((chip * 0x20) | addr);
}

function calculate_chip_address(chip, addr) {
  switch (PLAYER) {
    case 'jsidplay2':
      return jsidplay2_chip_address(chip, addr);
    default:
      return default_chip_address(chip, addr);
  }
}


/* The actual queue here ladies and gentle peoples */

function usbsid_queue() {
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
