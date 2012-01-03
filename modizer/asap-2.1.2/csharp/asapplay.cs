/*
 * asapplay.cs - .NET ASAP player
 *
 * Copyright (C) 2010  Piotr Fusik
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

using System;
using System.IO;
using System.Media;

using Sf.Asap;

public class ASAPWavStream : Stream
{
	readonly ASAP asap = new ASAP();
	readonly byte[] buffer = new byte[8192];
	int buffer_pos = 0;
	int buffer_len;

	public ASAPWavStream(string inputFilename, int song, int duration)
	{
		Stream s = File.OpenRead(inputFilename);
		byte[] module = new byte[ASAP.ModuleMax];
		int module_len = s.Read(module, 0, module.Length);
		s.Close();
		asap.Load(inputFilename, module, module_len);
		ASAP_ModuleInfo module_info = asap.GetModuleInfo();
		if (song < 0)
			song = module_info.default_song;
		if (duration < 0) {
			duration = module_info.durations[song];
			if (duration < 0)
				duration = 180 * 1000;
		}
		asap.PlaySong(song, duration);
		asap.GetWavHeader(buffer, ASAP_SampleFormat.S16LE);
		buffer_len = ASAP.WavHeaderBytes;
	}

	public override int Read(byte[] outputBuffer, int offset, int count)
	{
		int i = buffer_pos;
		if (i >= buffer_len) {
			buffer_len = asap.Generate(buffer, ASAP_SampleFormat.S16LE);
			if (buffer_len == 0)
				return 0;
			i = 0;
		}
		if (count > buffer_len - i)
			count = buffer_len - i;
		Array.Copy(buffer, i, outputBuffer, offset, count);
		buffer_pos = i + count;
		return count;
	}

	public override bool CanRead
	{
		get
		{
			return true;
		}
	}

	public override bool CanSeek
	{
		get
		{
			return false;
		}
	}

	public override bool CanWrite
	{
		get
		{
			return false;
		}
	}

	public override void Flush()
	{
		throw new NotSupportedException();
	}

	public override long Length
	{
		get
		{
			throw new NotSupportedException();
		}
	}

	public override long Position
	{
		get
		{
			throw new NotSupportedException();
		}
		set
		{
			throw new NotSupportedException();
		}
	}

	public override long Seek(long offset, SeekOrigin origin)
	{
		throw new NotSupportedException();
	}

	public override void SetLength(long value)
	{
		throw new NotSupportedException();
	}

	public override void Write(byte[] buffer, int offset, int count)
	{
		throw new NotSupportedException();
	}
}

public class asapplay
{
	static int song = -1;
	static int duration = -1;

	static void PrintHelp()
	{
		Console.Write(
			"Usage: asapplay [OPTIONS] INPUTFILE...\n" +
			"Each INPUTFILE must be in a supported format:\n" +
			"SAP, CMC, CM3, CMR, CMS, DMC, DLT, MPT, MPD, RMT, TMC, TM8 or TM2.\n" +
			"Options:\n" +
			"-s SONG     --song=SONG        Select subsong number (zero-based)\n" +
			"-t TIME     --time=TIME        Set output length (MM:SS format)\n" +
			"-h          --help             Display this information\n" +
			"-v          --version          Display version information\n"
		);
	}

	static void SetSong(string s)
	{
		song = int.Parse(s);
	}

	static void SetTime(string s)
	{
		duration = ASAP.ParseDuration(s);
	}

	static void ProcessFile(string inputFilename)
	{
		Stream s = new ASAPWavStream(inputFilename, song, duration);
		new SoundPlayer(s).PlaySync();
		song = -1;
		duration = -1;
	}

	public static int Main(string[] args)
	{
		bool noInputFiles = true;
		for (int i = 0; i < args.Length; i++) {
			string arg = args[i];
			if (arg[0] != '-') {
				ProcessFile(arg);
				noInputFiles = false;
			}
			else if (arg == "-s")
				SetSong(args[++i]);
			else if (arg.StartsWith("--song="))
				SetSong(arg.Substring(7));
			else if (arg == "-t")
				SetTime(args[++i]);
			else if (arg.StartsWith("--time="))
				SetTime(arg.Substring(7));
			else if (arg == "-h" || arg == "--help") {
				PrintHelp();
				noInputFiles = false;
			}
			else if (arg == "-v" || arg == "--version") {
				Console.WriteLine("asapplay (.NET) " + ASAP.Version);
				noInputFiles = false;
			}
			else
				throw new ArgumentException("unknown option: " + arg);
		}
		if (noInputFiles) {
			PrintHelp();
			return 1;
		}
		return 0;
	}
}
