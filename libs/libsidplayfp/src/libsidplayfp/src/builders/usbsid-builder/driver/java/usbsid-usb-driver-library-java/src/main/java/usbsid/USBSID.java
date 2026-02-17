package usbsid;

import java.text.MessageFormat;

// import javax.usb.UsbException;

// import java.io.ByteArrayOutputStream;
// import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import java.util.logging.Logger;

import usbsid.Config.CLK;
import usbsid.Config.Cfg;

// public class USBSID extends JAVAXDevice implements IUSBSID {
public class USBSID extends USBSIDDevice implements IUSBSID {

  /* Logging */
  private static Logger logger = null;
  static {
    System.setProperty("java.util.logging.SimpleFormatter.format",
                "[%1$tF %1$tT] [USBSID] [%4$s] %5$s %n");
    logger = Logger.getLogger(USBSIDDevice.class.getName());
  }

  private final byte ZERO = 0x0;

  private volatile boolean run_thread = false;
  private volatile byte[] ring_buffer;
  private volatile int ring_read, ring_write;
  private final int min_ring_diff = 16;
  private final int default_ring_diff = 64;
  private final int default_ring_diffwin = 64;
  private final int default_ring_size = 256;  /* Init default buffer size */
  private final int default_ring_sizewin = 8192;  /* Init default buffer size */
  private static int diff_size = 0;
  private static int ring_size = 0;

  private byte[] thread_buffer = new byte[64];
  private byte buffer_pos = 1;

  private volatile boolean flush_buffer = false;

  private static boolean clk_retrieved = false;
  private static int cpufrequency = CLK.DEFAULT.get();

  private long start_time = System.nanoTime();
  private long last_time;

  private void timeSync() {
    last_time = (System.nanoTime() - start_time);
  }

  private void RingBuffer(int size) {
    ring_read = ring_write = 0;
    ring_buffer = new byte[size];
  }
  private void put(byte item) {
    ring_buffer[ring_write] = item;
    ring_write = (ring_write + 1) % ring_buffer.length;
  }
  private byte get() {
    byte item = ring_buffer[ring_read];
    ring_read = (ring_read + 1) % ring_buffer.length;
    return item;
  }
  private boolean higher() {
    return (ring_read < ring_write);
  }
  private int diff() {
    int d = (higher() ? (ring_read - ring_write) : (ring_write - ring_read));
    return ((d < 0) ? (d * -1) : d);
  }

  private Thread USBSID_Thread = new Thread(new Runnable() {
    @Override
    public void run() {
      try {
        logger.info("[USBSID] Thread started");
        while (run_thread) {
          if (flush_buffer && (buffer_pos >= 5)) {
            USBSID_flushbuffer();
          }
          if ((ring_read != ring_write)) {
            if (diff() > diff_size) {
              USBSID_fillbuffer();
            }
          }
        }
        logger.info("[USBSID] Thread stopped");
      } catch (Exception E) {
        E.printStackTrace();
      }
    }
  });

  private void USBSID_flushbuffer()
    throws Exception
  { /* Only call this from the Thread loop! */
    try {
      // logger.info("Flushbuffer called! @ pos: {0}", buffer_pos);
      thread_buffer[0] = (byte)((Cmd.CYCLED_WRITE.get() << 6) | (buffer_pos - 1));
      final byte[] out_buffer = thread_buffer.clone();
      asyncWrite(out_buffer);
      Arrays.fill(thread_buffer, (byte)0);
      flush_buffer = false;
      buffer_pos = 1;
    } catch (Exception E) {
      logger.severe("[USBSID] Unhandled exception occured: " + E);
      throw E;
    }
  }

  private void USBSID_fillbuffer() throws Exception
  {
    try {
      thread_buffer[buffer_pos++] = get(); // addr
      thread_buffer[buffer_pos++] = get(); // data
      thread_buffer[buffer_pos++] = get(); // cycles_hi;
      thread_buffer[buffer_pos++] = get(); // cycles_lo;
      /* logger.info("[W %02d/%02d]$%02X:%02X {0}",
        (buffer_pos - 4), (buffer_pos - 1),
        thread_buffer[buffer_pos - 4],
        thread_buffer[buffer_pos - 3],
        (thread_buffer[buffer_pos - 2] << 8 | thread_buffer[buffer_pos - 1])); */
      if (buffer_pos == 61 || flush_buffer) {
        thread_buffer[0] = (byte)((Cmd.CYCLED_WRITE.get() << 6) | (buffer_pos - 1));
        flush_buffer = false;
        buffer_pos = 1;
        final byte[] write_buffer = thread_buffer.clone();
        asyncWrite(write_buffer);
        Arrays.fill(thread_buffer, (byte)0);
      }
    } catch (Exception E) {
      logger.severe("[USBSID] Unhandled exception occured: " + E);
      throw E;
    }
  }

  @Override
  public int USBSID_init(Integer...vars)
  {
    if (device != null && isOpen()) {
      logger.warning("[USBSID] Device is already open!");
      flush_buffer = true;
      return -1;
    }
    open_USBSID();
    if (isOpen()) {
      logger.info("[USBSID] USBSID-Pico opened");
      ring_read = ring_write = 0;
      if (vars.length > 0) {
        Integer rs = vars[0];
         /* 512 bytes minimum, 65535 bytes maximum */
        ring_size = ((rs >= default_ring_size) && (rs <= 65535) ? rs : default_ring_size);
      }
      if (vars.length > 1) {
        Integer ds = vars[1];
        diff_size = ((ds >= min_ring_diff) ? ds : default_ring_diff);
      }
      RingBuffer(ring_size);
      logger.info(MessageFormat.format("[USBSID] Ring buffer size {0} with mininum head->tail distance {1}", ring_size, diff_size));
      run_thread = true;
      USBSID_Thread.setDaemon(true);
      USBSID_Thread.start();
      timeSync();  // last_time = (System.nanoTime() - start_time);
      return 0;
    }
    return -1;
  }

  @Override
  public int USBSID_init()
  {
    if (isWinblows()) {
      ring_size = default_ring_sizewin;
      diff_size = default_ring_diffwin;
    } else {
      ring_size = default_ring_size;
      diff_size = default_ring_diff;
    }
    return USBSID_init(ring_size, diff_size);
  }

  @Override
  public int USBSID_init(String driver, int ringsize, int diffsize)
  {
    setdriver_USBSID(driver);
    return USBSID_init(ringsize, diffsize);
  }

  @Override
  public void USBSID_exit()
  {
    try {
      if (device == null || !isOpen()) {
        logger.warning("[USBSID] Device is not open!");
        return;
      }
      USBSID_setflush();
      ring_read = ring_write = 0;
      run_thread = false;
      USBSID_Thread.join();
      logger.info("[USBSID] Thread joined");
      close_USBSID();
      logger.info("[USBSID] USBSID-Pico closed");
      return;
    } catch (InterruptedException | NumberFormatException E) {
      logger.severe("[USBSID] Exception occured: " + E.getMessage() + E.getCause());
      E.printStackTrace();
      return;
    }
  }

  @Override
  public void USBSID_clkdwrite(byte addr, byte data, short cycles)
  {
    try {
      byte[] writebuffer = new byte[5];
      byte cycles_hi = (byte)((cycles >> 8) & (byte)0xff);
      byte cycles_lo = (byte)(cycles & (byte)0xff);
      writebuffer[0] = (byte)(Cmd.CYCLED_WRITE.get() << 6);
      writebuffer[1] = (byte)addr;
      writebuffer[2] = (byte)data;
      writebuffer[3] = (byte)cycles_hi;
      writebuffer[4] = (byte)cycles_lo;
      asyncWrite(writebuffer);
    } catch (Exception E) {
      logger.severe("[USBSID] Unhandled exception occured: " + E);
      E.printStackTrace();
      return;
    }
  }

  @Override
  public void USBSID_writeclkdbuffer(byte addr, byte data, short cycles)
  {
    /* cycles -= 1 Works for Coma Light 13 tune 4 (do this on the emulator side!) */
    byte cycles_hi = (byte)((cycles >> 8) & (byte)0xff);
    byte cycles_lo = (byte)(cycles & (byte)0xff);
    put(addr);
    put(data);
    put(cycles_hi);
    put(cycles_lo);
    /* logger.info("[W]$%02X:%02X {0}", (addr & 0xFF), (data & 0xFF), (cycles & 0xFFFF)); */
  }

  @Override
  public void USBSID_setflush()
  {
    try {
      flush_buffer = true;
      timeSync();
    } catch (Exception E) {
      logger.severe("[USBSID] Unhandled exception occured: " + E);
      E.printStackTrace();
      return;
    }
  }

  @Override
  public void USBSID_reset(byte volume)
  {
    try {
      if (device == null || !isOpen()) {
        logger.warning("[USBSID] Device is not open!");
        return;
      }
      /* BUG: SKPICO WILL HAVE VOLUME AT 0 DUE TO NOT RESETTING FAST ENOUGH! */
      sendCommand(Cmd.RESET_SID.get(), (byte)0x0);
      if ((byte)volume == 0) sendCommand(Cmd.MUTE.get());
      if ((byte)volume > 0) sendCommand(Cmd.UNMUTE.get());
      flush_buffer = true;
      timeSync();
    } catch (Exception E) {
      logger.severe("[USBSID] Exception occured: " + E);
      E.printStackTrace();
      return;
    }
  }

  @Override
  public void USBSID_setclock(double CpuClock)
  {
    try {
      if (!clk_retrieved || cpufrequency == CLK.DEFAULT.get()) {
        byte[] clock = rwConfigCommand(Cfg.GET_CLOCK.get(), 1);
        cpufrequency = CLK.IDclk((int)clock[0]);
        clk_retrieved = true;
      }
      /* logger.info("[USBSID] Clock change requested: %.02f", CpuClock); */
      if ((int)CpuClock != cpufrequency) {
        cpufrequency = (int)CpuClock;
        logger.info(MessageFormat.format("[USBSID] Clock change requested: {0}", cpufrequency));
        byte freq = 0;
        freq = (byte)CLK.clkID(CLK.getCLK(cpufrequency));
        sendConfigCommand(Cfg.SET_CLOCK.get(), freq);
      } else {
        logger.info(MessageFormat.format("[USBSID] Clock not changed, already at: {0}", cpufrequency));
        flush_buffer = true;
        timeSync();
      }
    } catch (Exception E) {
      logger.severe("[USBSID] Unhandled exception occured: " + E);
      E.printStackTrace();
      return;
    }
  }

  @Override
  public int USBSID_setstereo(int stereo)
  {
    try {
      sendConfigCommand(Cfg.SET_AUDIO.get(), (byte)stereo);
      return stereo;
    } catch (Exception E) {
      logger.severe("[USBSID] Unhandled exception occured: " + E);
      E.printStackTrace();
    }
    return -1;
  }

  @Override
  public byte[] USBSID_getsocketconfig()
  {
    byte[] socketcfg = null;
    try {
      socketcfg = rwConfigCommand(Cfg.READ_SOCKETCFG.get(), 10);
    } catch (Exception E) {
      logger.severe("[USBSID] Unhandled exception occured: " + E);
      E.printStackTrace();
    }
    return socketcfg;
  }

  @Override
  public int[] USBSID_parsesocketconfig(byte[] socketcfg)
  { /* TODO: Has no knowledge of pre-defined socket SID id's */
    assert socketcfg[0] == Cfg.READ_SOCKETCFG.get();
    assert (byte)socketcfg[1] == (byte)0x7F;
    assert (byte)socketcfg[9] == (byte)0xFF;

    int socketone_en = (socketcfg[2] >> 4);
    int socketone_dual = (socketcfg[2] & 0xF);
    int socketone_chiptype = (socketcfg[3] >> 4);
    int socketone_clonetype = (socketcfg[3] & 0xF);
    int socketone_sidone = (socketcfg[4] >> 4);
    int socketone_sidtwo = (socketcfg[4] & 0xF);

    int sockettwo_en = (socketcfg[5] >> 4);
    int sockettwo_dual = (socketcfg[5] & 0xF);
    int sockettwo_chiptype = (socketcfg[6] >> 4);
    int sockettwo_clonetype = (socketcfg[6] & 0xF);
    int sockettwo_sidone = (socketcfg[7] >> 4);
    int sockettwo_sidtwo = (socketcfg[7] & 0xF);

    int mirrored = (socketcfg[8] & 0xF);

    int socketone_numsids = ((socketone_en == 1) ? (socketone_dual == 1) ? 2 : 1 : 0);
    int sockettwo_numsids = ((sockettwo_en == 1) ? (sockettwo_dual == 1) ? 2 : 1 : 0);
    int numsids = (socketone_numsids + sockettwo_numsids);

    final int[] parsed = {
      socketone_en, socketone_dual,
      socketone_chiptype, socketone_clonetype,
      socketone_sidone, socketone_sidtwo,
      sockettwo_en, sockettwo_dual,
      sockettwo_chiptype, sockettwo_clonetype,
      sockettwo_sidone, sockettwo_sidtwo,
      socketone_numsids, sockettwo_numsids,
      numsids, mirrored,
    };
    return parsed;
  }

  @Override
  public int USBSID_getsocketsidtype(int socket, int sidno, byte[] socketcfg)
  {
    assert socketcfg[0] == Cfg.READ_SOCKETCFG.get();
    assert (byte)socketcfg[1] == (byte)0x7F;
    assert (byte)socketcfg[9] == (byte)0xFF;

    int socketone_sidone = (socketcfg[4] >> 4);
    int socketone_sidtwo = (socketcfg[4] & 0xF);
    int sockettwo_sidone = (socketcfg[7] >> 4);
    int sockettwo_sidtwo = (socketcfg[7] & 0xF);

    switch (socket) {
      case 1:
        return ((sidno == 1) ? socketone_sidone : socketone_sidtwo);
      case 2:
        return ((sidno == 1) ? sockettwo_sidone : sockettwo_sidtwo);
    }

    return 0;
  }

  @Override
  public int USBSID_sidtypebysidno(int sidno, byte[] socketcfg)
  { /* TODO: Assumes non re-ordered socket config */
    final int[] parsedcfg = USBSID_parsesocketconfig(socketcfg);
    int socketone_en = parsedcfg[0];
    int socketone_sidone = parsedcfg[4];
    int socketone_sidtwo = parsedcfg[5];
    int sockettwo_en = parsedcfg[6];
    int sockettwo_sidone = parsedcfg[10];
    int sockettwo_sidtwo = parsedcfg[11];
    int socketone_numsids = parsedcfg[12];
    int sockettwo_numsids = parsedcfg[13];
    int numsids = parsedcfg[14];


    int configs[][] = {
       /* 0 Single one */
      {socketone_sidone},
       /* 1 Single two */
      {sockettwo_sidone},
      /* 2 Dual (one + two) */
      {socketone_sidone, sockettwo_sidone},
      /* 3 Dual one */
      {socketone_sidone, socketone_sidtwo},
      /* 4 Dual two */
      {sockettwo_sidone, sockettwo_sidtwo},
      /* 5 Tripe one (dual one, single two) */
      {socketone_sidone, socketone_sidtwo, sockettwo_sidone},
      /* 6 Tripe two (single one, dual two) */
      {socketone_sidone, sockettwo_sidone, sockettwo_sidtwo},
      /* 7 Quad */
      {socketone_sidone, socketone_sidtwo, sockettwo_sidone, sockettwo_sidtwo},
    };

    int sidtypes[] = null;
    switch (numsids) {
      case 1:
        sidtypes =
          ((socketone_en == 1) ? configs[0] : (sockettwo_en == 1) ? configs[1] : null);
        break;
      case 2:
        sidtypes =
          ((socketone_en == 1 && socketone_numsids == 1)
              && (sockettwo_en == 1 && sockettwo_numsids == 1)
            ? configs[2]
            : (socketone_en == 1 && socketone_numsids == 2)
            ? configs[3]
            : (sockettwo_en == 1 && sockettwo_numsids == 2)
            ? configs[4]
            : null);
        break;
      case 3:
        sidtypes =
          ((socketone_en == 1 && socketone_numsids == 2)
            ? configs[5]
            : (sockettwo_en == 1 && sockettwo_numsids == 2)
            ? configs[6]
            : null);
        break;
      case 4:
        sidtypes = configs[7];
        break;
      default:
        sidtypes = null;
    };

    return sidtypes[sidno];
  }

  @Override
  public int USBSID_getnumsids()
  {
    int numsids = 0;
    try {
      byte[] r = rwConfigCommand(Cfg.READ_NUMSIDS.get(), 1);
      numsids = (int)r[0];
    } catch (Exception E) {
      logger.severe("[USBSID] Unhandled exception occured: " + E);
      E.printStackTrace();
    }
    return numsids;
  }

  @Override
  public String USBSID_getpcbversion()
  {
    String pcbversion = "0.0";
    try {
      byte[] result = rwConfigCommand(Cfg.US_PCB_VERSION.get(), 64);
      int length = (int)result[1];
      byte[] bytes = Arrays.copyOfRange(result, 2, (2+length));
      pcbversion = new String(bytes);
    } catch (Exception E) {
      logger.severe("[USBSID] Unhandled exception occured: " + E);
      E.printStackTrace();
    }
    return pcbversion;
  }

  @Override
  public String USBSID_getfwversion()
  {
    String fwversion = "v0.0.0-ALPHA.19700101";
    try {
      byte[] result = rwConfigCommand(Cfg.USBSID_VERSION.get(), 64);
      int length = (int)result[1];
      byte[] bytes = Arrays.copyOfRange(result, 2, (2+length));
      fwversion = new String(bytes);
    } catch (Exception E) {
      logger.severe("[USBSID] Unhandled exception occured: " + E);
      E.printStackTrace();
    }
    return fwversion;
  }

  @Override
  public void USBSID_delay(short cycles)
  {
    final long cpu_cycle_in_ns = (long)((float)(1.0 / cpufrequency) * 1000000000);
    long now = (System.nanoTime() - start_time);
    long duration = (cycles * cpu_cycle_in_ns);
    long target_time = (last_time + duration);
    long target_delta = (target_time - now);

    /* logger.info("[CYC]%04d [DUR]%06d [NS]%06d [NOW]%06d [LT]%06d [TT]%06d [TD]%06d [CPU]{0}",
       (cycles & 0xFFFF),
       (duration & 0xFFFFFFFFL), delayNs,
       (now & 0xFFFFFFFFL), (last_time & 0xFFFFFFFFL),
       (target_time & 0xFFFFFFFFL), target_delta, cpu_cycle_in_ns); */

    nsleep(target_delta);
    last_time = target_time;
  }

  private void nsleep(long delayNs) {
    long start = System.nanoTime();
    long end = 0;
    do {
      end = System.nanoTime();
      /* logger.info("{0} {0} {0}", start, end, delayNs); */
    } while (start + delayNs >= end);
  }

}
