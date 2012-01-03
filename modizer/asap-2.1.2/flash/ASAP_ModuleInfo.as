/*
 * ASAP_ModuleInfo.as - file information
 *
 * Copyright (C) 2009-2010  Piotr Fusik
 *
 * This file is part of ASAP (Another Slight Atari Player),
 * see http://asap.sourceforge.net
 *
 * ASAP is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * ASAP is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ASAP; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

package
{
	/** Information about a file recognized by ASAP. */
	public class ASAP_ModuleInfo
	{
		/** Music author's name. */
		public var author : String;
		/** Music title. */
		public var name : String;
		/** Music creation date. */
		public var date : String;
		/** 1 for mono or 2 for stereo. */
		public var channels : int;
		/** Number of subsongs. */
		public var songs : int;
		/** 0-based index of the "main" subsong. */
		public var default_song : int;
		/** Lengths of songs, in milliseconds, -1 = unspecified. */
		public const durations : Array = new Array(32);
		/** Information about finite vs infinite songs.
		    Each element of the array represents one song, and is:
		    <ul>
		    <li> <code>true</code> if the song loops</li>
		    <li> <code>false</code> if the song stops</li>
		    </ul> */
		public const loops : Array = new Array(32);
		/** @private */
		internal var ntsc : Boolean;
		/** @private */
		internal var type : int;
		/** @private */
		internal var fastplay : int;
		/** @private */
		internal var music : int;
		/** @private */
		internal var init : int;
		/** @private */
		internal var player : int;
		/** @private */
		internal var covox_addr : int;
		/** @private */
		internal var header_len : int;
		/** @private */
		internal const song_pos : Array = new Array(32);
	}
}
