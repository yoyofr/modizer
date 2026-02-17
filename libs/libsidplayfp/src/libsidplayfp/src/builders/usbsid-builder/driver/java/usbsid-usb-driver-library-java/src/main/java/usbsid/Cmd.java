package usbsid;

import java.util.Arrays;
import java.util.Collections;
import java.util.Map;
import java.util.function.Function;
import java.util.stream.Collectors;

public enum Cmd {
  /* BYTE 0 - top 2 bits */
  WRITE((byte)0),   /*        0b0 ~ 0x00 */
  READ((byte)1),   /*        0b1 ~ 0x40 */
  CYCLED_WRITE((byte)2),   /*       0b10 ~ 0x80 */
  COMMAND((byte)3),   /*       0b11 ~ 0xC0 */
  /* BYTE 0 - lower 6 bits for byte count */
  /* BYTE 0 - lower 6 bits for Commands */
  PAUSE((byte)10),   /*     0b1010 ~ 0x0A */
  UNPAUSE((byte)11),   /*     0b1011 ~ 0x0B */
  MUTE((byte)12),   /*     0b1100 ~ 0x0C */
  UNMUTE((byte)13),   /*     0b1101 ~ 0x0D */
  RESET_SID((byte)14),   /*     0b1110 ~ 0x0E */
  DISABLE_SID((byte)15),   /*     0b1111 ~ 0x0F */
  ENABLE_SID((byte)16),   /*    0b10000 ~ 0x10 */
  CLEAR_BUS((byte)17),   /*    0b10001 ~ 0x11 */
  CONFIG((byte)18),   /*    0b10010 ~ 0x12 */
  RESET_MCU((byte)19),   /*    0b10011 ~ 0x13 */
  BOOTLOADER((byte)20),;   /*    0b10100 ~ 0x14 */

  private byte cmd;
  private Cmd(byte cmd) { this.cmd = cmd; }
  private static final Map<Byte, Cmd> lookup = Collections.unmodifiableMap(
      Arrays.asList(Cmd.values()).stream().collect(Collectors.toMap(Cmd::get, Function.identity())));
  public byte get() { return cmd; }
  public static Cmd getCommand(byte cmd) { return lookup.get(cmd); }
}
