package usbsid;

import java.util.Arrays;
import java.util.Collections;
import java.util.Map;
import java.util.function.Function;
import java.util.stream.Collectors;

public class Config /* implements IConfig */ {

  public enum CLK {
    DEFAULT(1000000),  /* 1 MHz     = 1 us */
    PAL(985248),       /* 0.985 MHz = 1.014973 us */
    NTSC(1022727),     /* 1.023 MHz = 0.977778 us */
    DREAN(1023440),    /* 1.023 MHz = 0.977097 us */
    NTSC2(1022730),;   /* 1.023 MHz = 0.977778 us */

    private int clk;
    private CLK(int clk) { this.clk = clk; }
    private static final Map<Integer, CLK> lookup = Collections.unmodifiableMap(
      Arrays.asList(CLK.values()).stream().collect(Collectors.toMap(CLK::get, Function.identity())));
    public int get() { return clk; }
    public static CLK getCLK(int clk) { return lookup.get(clk); }
    public static int clkID(CLK clock) {
      switch (clock) {
        case DEFAULT:
          return 0;
        case PAL:
          return 1;
        case NTSC:
          return 2;
        case DREAN:
          return 3;
        case NTSC2:
          return 4;
        default:
          break;
      }
      return 1;
    }
    public static int IDclk(int clock) {
      switch (clock) {
        case 0:
          return CLK.DEFAULT.get();
        case 1:
          return CLK.PAL.get();
        case 2:
          return CLK.NTSC.get();
        case 3:
          return CLK.DREAN.get();
        case 4:
          return CLK.NTSC2.get();
        default:
          break;
      }
      return CLK.PAL.get();
    }
  }

  public enum Cfg {
    RESET_USBSID((byte)0x20),  /* Resets the MCU including the USB connection */

    READ_CONFIG((byte)0x30),  /* Read full config as bytes */
    APPLY_CONFIG((byte)0x31),  /* Apply config from memory */
    SET_CONFIG((byte)0x32),  /* Set single config item */
    SAVE_CONFIG((byte)0x33),  /* Save and load config and then reboot */
    SAVE_NORESET((byte)0x34),  /* Save, load and apply config */
    RESET_CONFIG((byte)0x35),  /* Reset to default settings */
    WRITE_CONFIG((byte)0x36),  /* Write full config as bytes */
    READ_SOCKETCFG((byte)0x37),  /* Read socket config as bytes */
    RELOAD_CONFIG((byte)0x38),  /* Reload and apply stored config from flash */
    READ_NUMSIDS((byte)0x39),  /* Returns the number of SIDs in byte 0 */
    READ_FMOPLSID((byte)0x3A),  /* Returns the sidno for FMOpl 1~4, 0 is disable  */

    SINGLE_SID((byte)0x40),  /* Single SID Socket One */
    DUAL_SID((byte)0x41),  /* Dual SID Socket One */
    QUAD_SID((byte)0x42),  /* Four SID's in 2 sockets */
    TRIPLE_SID((byte)0x43),  /* Two SID's in socket One, One SID in socket two */
    TRIPLE_SID_TWO((byte)0x44),  /* One SID in Socket One, Two SID's in socket two */
    MIRRORED_SID((byte)0x45),  /* Socket Two is linked to Socket One */
    DUAL_SOCKET1((byte)0x46),  /* Two SID's in socket One, Socket Two disabled */
    DUAL_SOCKET2((byte)0x47),  /* Two SID's in socket Two, Socket One disabled */

    SET_CLOCK((byte)0x50),  /* Change SID clock frequency by array id */
    DETECT_SIDS((byte)0x51),  /* Try to detect the SID types per socket */
    TEST_ALLSIDS((byte)0x52),  /* Runs a very long test on all SID's */
    TEST_SID1((byte)0x53),  /* Runs a very long test on SID 1 */
    TEST_SID2((byte)0x54),  /* Runs a very long test on SID 2 */
    TEST_SID3((byte)0x55),  /* Runs a very long test on SID 3 */
    TEST_SID4((byte)0x56),  /* Runs a very long test on SID 4 */
    GET_CLOCK((byte)0x57),  /* Returns the clockrate as array id in byte 0 */
    LOCK_CLOCK((byte)0x58),  /* Locks the clockrate from being changed, saved in config */
    STOP_TESTS((byte)0x59),  /* Interrupt any running SID tests */

    LOAD_MIDI_STATE((byte)0x60),
    SAVE_MIDI_STATE((byte)0x61),
    RESET_MIDI_STATE((byte)0x63),

    USBSID_VERSION((byte)0x80),  /* Read version identifier as uint32_t */
    US_PCB_VERSION((byte)0x81),  /* Read PCB version */

    RESTART_BUS((byte)0x85),  /* Restart DMA & PIO */
    RESTART_BUS_CLK((byte)0x86),  /* Restart PIO clocks */
    SYNC_PIOS((byte)0x87),  /* Sync PIO clocks */
    TOGGLE_AUDIO((byte)0x88),  /* Toggle mono <-> stereo (v1.3+ boards only) */
    SET_AUDIO((byte)0x89),;  /* Set mono <-> stereo (v1.3+ boards only) */

    private byte cmd;
    private Cfg(byte cmd) { this.cmd = cmd; }
    private static final Map<Byte, Cfg> lookup = Collections.unmodifiableMap(
      Arrays.asList(Cfg.values()).stream().collect(Collectors.toMap(Cfg::get, Function.identity())));
    public byte get() { return cmd; }
    public static Cfg getCfgCmd(byte cmd) { return lookup.get(cmd); }
}

}
