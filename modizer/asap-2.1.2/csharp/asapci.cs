// Generated automatically with "cito". Do not edit.
namespace Sf.Asap {

	/// <summary>Information about a music file.</summary>
	public class ASAP_ModuleInfo {
		/// <summary>Author's name.</summary>
		/// <remarks>A nickname may be included in parentheses after the real name.
		/// Multiple authors are separated with <c>" &amp; "</c>.
		/// Empty string means the author is unknown.</remarks>
		public string author;
		/// <summary>Music title.</summary>
		/// <remarks>Empty string means the title is unknown.</remarks>
		public string name;
		/// <summary>Music creation date.</summary>
		/// <remarks>Some of the possible formats are:
		/// <list type="bullet">
		/// <item>YYYY</item>
		/// <item>MM/YYYY</item>
		/// <item>DD/MM/YYYY</item>
		/// <item>YYYY-YYYY</item>
		/// </list>
		/// Empty string means the date is unknown.</remarks>
		public string date;
		/// <summary>1 for mono or 2 for stereo.</summary>
		public int channels;
		/// <summary>Number of songs in the file.</summary>
		public int songs;
		/// <summary>0-based index of the "main" song.</summary>
		/// <remarks>The specified song should be played by default.</remarks>
		public int default_song;
		/// <summary>Lengths of songs.</summary>
		/// <remarks>Each element of the array represents length of one song,
		/// in milliseconds. -1 means the length is indeterminate.</remarks>
		public readonly int[] durations = new int[32];
		/// <summary>Information about finite vs infinite songs.</summary>
		/// <remarks>Each element of the array represents one song, and is:
		/// <list type="bullet">
		/// <item><see langword="true" /> if the song loops</item>
		/// <item><see langword="false" /> if the song stops</item>
		/// </list></remarks>
		public readonly bool[] loops = new bool[32];
		internal bool ntsc;
		internal int type;
		internal int fastplay;
		internal int music;
		internal int init;
		internal int player;
		internal int covox_addr;
		internal int header_len;
		internal readonly byte[] song_pos = new byte[32];
	}

	internal class PokeyState {
		internal int audctl;
		internal bool init;
		internal int poly_index;
		internal int div_cycles;
		internal int mute1;
		internal int mute2;
		internal int mute3;
		internal int mute4;
		internal int audf1;
		internal int audf2;
		internal int audf3;
		internal int audf4;
		internal int audc1;
		internal int audc2;
		internal int audc3;
		internal int audc4;
		internal int tick_cycle1;
		internal int tick_cycle2;
		internal int tick_cycle3;
		internal int tick_cycle4;
		internal int period_cycles1;
		internal int period_cycles2;
		internal int period_cycles3;
		internal int period_cycles4;
		internal int reload_cycles1;
		internal int reload_cycles3;
		internal int out1;
		internal int out2;
		internal int out3;
		internal int out4;
		internal int delta1;
		internal int delta2;
		internal int delta3;
		internal int delta4;
		internal int skctl;
		internal readonly int[] delta_buffer = new int[888];
	}

	internal class ASAP_State {
		internal int cycle;
		internal int cpu_pc;
		internal int cpu_a;
		internal int cpu_x;
		internal int cpu_y;
		internal int cpu_s;
		internal int cpu_nz;
		internal int cpu_c;
		internal int cpu_vdi;
		internal int scanline_number;
		internal int nearest_event_cycle;
		internal int next_scanline_cycle;
		internal int timer1_cycle;
		internal int timer2_cycle;
		internal int timer4_cycle;
		internal int irqst;
		internal int extra_pokey_mask;
		internal int consol;
		internal readonly byte[] covox = new byte[4];
		internal readonly PokeyState base_pokey = new PokeyState();
		internal readonly PokeyState extra_pokey = new PokeyState();
		internal int sample_offset;
		internal int sample_index;
		internal int samples;
		internal int iir_acc_left;
		internal int iir_acc_right;
		public readonly ASAP_ModuleInfo module_info = new ASAP_ModuleInfo();
		internal int tmc_per_frame;
		internal int tmc_per_frame_counter;
		internal int current_song;
		internal int current_duration;
		internal int blocks_played;
		internal int silence_cycles;
		internal int silence_cycles_counter;
		internal readonly byte[] poly9_lookup = new byte[511];
		internal readonly byte[] poly17_lookup = new byte[16385];
		internal readonly byte[] memory = new byte[65536];
	}
}
