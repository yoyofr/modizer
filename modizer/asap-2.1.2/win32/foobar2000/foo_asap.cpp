/*
 * foo_asap.cpp - ASAP plugin for foobar2000 1.0
 *
 * Copyright (C) 2006-2010  Piotr Fusik
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

#include <windows.h>
#include <string.h>

#include "foobar2000/SDK/foobar2000.h"

#include "asap.h"
#include "settings_dlg.h"

#define BITS_PER_SAMPLE    16
#define BUFFERED_BLOCKS    1024

/* Configuration --------------------------------------------------------- */

static const GUID song_length_guid =
	{ 0x810e12f0, 0xa695, 0x42d2, { 0xab, 0xc0, 0x14, 0x1e, 0xe5, 0xf3, 0xb3, 0xb7 } };
static cfg_int song_length(song_length_guid, -1);

static const GUID silence_seconds_guid =
	{ 0x40170cb0, 0xc18c, 0x4f97, { 0xaa, 0x6, 0xdb, 0xe7, 0x45, 0xb0, 0x7e, 0x1d } };
static cfg_int silence_seconds(silence_seconds_guid, -1);

static const GUID play_loops_guid =
	{ 0xf08d12f8, 0x58d6, 0x49fc, { 0xae, 0xc5, 0xf4, 0xd0, 0x2f, 0xb2, 0x20, 0xaf } };
static cfg_bool play_loops(play_loops_guid, false);

static const GUID mute_mask_guid =
	{ 0x8bd94472, 0x82f1, 0x4e77, { 0x95, 0x78, 0xe9, 0x84, 0x75, 0xad, 0x17, 0x4 } };
static cfg_int mute_mask(mute_mask_guid, 0);


/* Decoding -------------------------------------------------------------- */

class input_asap
{
	static input_asap *head;
	input_asap *prev;
	input_asap *next;
	service_ptr_t<file> m_file;
	char filename[MAX_PATH];
	byte module[ASAP_MODULE_MAX];
	int module_len;
	ASAP_State asap;
	ASAP_ModuleInfo module_info;

	int get_song_duration(int song, ASAP_State *ast)
	{
		int duration = module_info.durations[song];
		if (duration < 0) {
			if (silence_seconds > 0)
				ASAP_DetectSilence(ast, silence_seconds);
			return 1000 * song_length;
		}
		if (play_loops && module_info.loops[song])
			return 1000 * song_length;
		return duration;
	}

	static void copy_info(char *dest, const file_info &p_info, const char *p_name)
	{
		const char *src;
		int i = 0;
		src = p_info.meta_get(p_name, 0);
		if (src != NULL)
			for (; i < ASAP_INFO_CHARS - 1 && src[i] != '\0'; i++)
				dest[i] = src[i];
		dest[i] = '\0';
	}

public:

	static void g_set_mute_mask(int mask)
	{
		input_asap *p;
		for (p = head; p != NULL; p = p->next)
			ASAP_MutePokeyChannels(&p->asap, mask);
	}

	static bool g_is_our_content_type(const char *p_content_type)
	{
		return false;
	}

	static bool g_is_our_path(const char *p_path, const char *p_extension)
	{
		return ASAP_IsOurFile(p_path) != 0;
	}

	input_asap()
	{
		if (head != NULL)
			head->prev = this;
		prev = NULL;
		next = head;
		head = this;
	}

	~input_asap()
	{
		if (prev != NULL)
			prev->next = next;
		if (next != NULL)
			next->prev = prev;
		if (head == this)
			head = next;
	}

	void open(service_ptr_t<file> p_filehint, const char *p_path, t_input_open_reason p_reason, abort_callback &p_abort)
	{
		if (p_reason == input_open_info_write) {
			int len = strlen(p_path);
			if (len >= MAX_PATH || !ASAP_CanSetModuleInfo(p_path))
				throw exception_io_unsupported_format();
			memcpy(filename, p_path, len + 1);
		}
		if (p_filehint.is_empty())
			filesystem::g_open(p_filehint, p_path, filesystem::open_mode_read, p_abort);
		m_file = p_filehint;
		module_len = m_file->read(module, sizeof(module), p_abort);
		if (!ASAP_GetModuleInfo(&module_info, p_path, module, module_len))
			throw exception_io_unsupported_format();
		if (p_reason == input_open_decode)
			if (!ASAP_Load(&asap, p_path, module, module_len))
				throw exception_io_unsupported_format();
	}

	t_uint32 get_subsong_count()
	{
		return module_info.songs;
	}

	t_uint32 get_subsong(t_uint32 p_index)
	{
		return p_index;
	}

	void get_info(t_uint32 p_subsong, file_info &p_info, abort_callback &p_abort)
	{
		int duration = get_song_duration(p_subsong, NULL);
		if (duration >= 0)
			p_info.set_length(duration / 1000.0);
		p_info.info_set_int("channels", module_info.channels);
		p_info.info_set_int("subsongs", module_info.songs);
		if (module_info.author[0] != '\0')
			p_info.meta_set("composer", module_info.author);
		p_info.meta_set("title", module_info.name);
		if (module_info.date[0] != '\0')
			p_info.meta_set("date", module_info.date);
	}

	t_filestats get_file_stats(abort_callback &p_abort)
	{
		return m_file->get_stats(p_abort);
	}

	void decode_initialize(t_uint32 p_subsong, unsigned p_flags, abort_callback &p_abort)
	{
		int duration = get_song_duration(p_subsong, &asap);
		ASAP_PlaySong(&asap, p_subsong, duration);
		ASAP_MutePokeyChannels(&asap, mute_mask);
	}

	bool decode_run(audio_chunk &p_chunk, abort_callback &p_abort)
	{
		int channels = module_info.channels;
		int buffered_bytes = BUFFERED_BLOCKS * channels * (BITS_PER_SAMPLE / 8);
		static
#if BITS_PER_SAMPLE == 8
			byte
#else
			short
#endif
			buffer[BUFFERED_BLOCKS * 2];

		buffered_bytes = ASAP_Generate(&asap, buffer, buffered_bytes,
			(ASAP_SampleFormat) BITS_PER_SAMPLE);
		if (buffered_bytes == 0)
			return false;
		p_chunk.set_data_fixedpoint(buffer, buffered_bytes, ASAP_SAMPLE_RATE,
			channels, BITS_PER_SAMPLE,
			channels == 2 ? audio_chunk::channel_config_stereo : audio_chunk::channel_config_mono);
		return true;
	}

	void decode_seek(double p_seconds, abort_callback &p_abort)
	{
		ASAP_Seek(&asap, (int) (p_seconds * 1000));
	}

	bool decode_can_seek()
	{
		return true;
	}

	bool decode_get_dynamic_info(file_info &p_out, double &p_timestamp_delta)
	{
		return false;
	}

	bool decode_get_dynamic_info_track(file_info &p_out, double &p_timestamp_delta)
	{
		return false;
	}

	void decode_on_idle(abort_callback &p_abort)
	{
		m_file->on_idle(p_abort);
	}

	void retag_set_info(t_uint32 p_subsong, const file_info &p_info, abort_callback &p_abort)
	{
		copy_info(module_info.author, p_info, "composer");
		copy_info(module_info.name, p_info, "title");
		copy_info(module_info.date, p_info, "date");
	}

	void retag_commit(abort_callback &p_abort)
	{
		byte out_module[ASAP_MODULE_MAX];
		int out_len = ASAP_SetModuleInfo(&module_info, module, module_len, out_module);
		if (out_len <= 0)
			throw exception_io_unsupported_format();
		m_file.release();
		filesystem::g_open(m_file, filename, filesystem::open_mode_write_new, p_abort);
		m_file->write(out_module, out_len, p_abort);
	}
};

input_asap *input_asap::head = NULL;
static input_factory_t<input_asap> g_input_asap_factory;


/* Configuration --------------------------------------------------------- */

static preferences_page_callback::ptr g_callback;

static INT_PTR CALLBACK settings_dialog_proc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		settingsDialogSet(hDlg, song_length, silence_seconds, play_loops, mute_mask);
		return TRUE;
	case WM_COMMAND:
		switch (wParam) {
		case MAKEWPARAM(IDC_UNLIMITED, BN_CLICKED):
			enableTimeInput(hDlg, FALSE);
			g_callback->on_state_changed();
			return TRUE;
		case MAKEWPARAM(IDC_LIMITED, BN_CLICKED):
			enableTimeInput(hDlg, TRUE);
			setFocusAndSelect(hDlg, IDC_MINUTES);
			g_callback->on_state_changed();
			return TRUE;
		case MAKEWPARAM(IDC_SILENCE, BN_CLICKED):
		{
			BOOL enabled = (IsDlgButtonChecked(hDlg, IDC_SILENCE) == BST_CHECKED);
			EnableWindow(GetDlgItem(hDlg, IDC_SILSECONDS), enabled);
			if (enabled)
				setFocusAndSelect(hDlg, IDC_SILSECONDS);
			g_callback->on_state_changed();
			return TRUE;
		}
		case MAKEWPARAM(IDC_MINUTES, EN_CHANGE):
		case MAKEWPARAM(IDC_SECONDS, EN_CHANGE):
		case MAKEWPARAM(IDC_SILSECONDS, EN_CHANGE):
		case MAKEWPARAM(IDC_LOOPS, BN_CLICKED):
		case MAKEWPARAM(IDC_NOLOOPS, BN_CLICKED):
		case MAKEWPARAM(IDC_MUTE1, BN_CLICKED):
		case MAKEWPARAM(IDC_MUTE1 + 1, BN_CLICKED):
		case MAKEWPARAM(IDC_MUTE1 + 2, BN_CLICKED):
		case MAKEWPARAM(IDC_MUTE1 + 3, BN_CLICKED):
		case MAKEWPARAM(IDC_MUTE1 + 4, BN_CLICKED):
		case MAKEWPARAM(IDC_MUTE1 + 5, BN_CLICKED):
		case MAKEWPARAM(IDC_MUTE1 + 6, BN_CLICKED):
		case MAKEWPARAM(IDC_MUTE1 + 7, BN_CLICKED):
			g_callback->on_state_changed();
			return TRUE;
		default:
			break;
		}
		break;
	default:
		break;
	}
	return FALSE;
}

class preferences_page_instance_asap : public preferences_page_instance
{
	HWND m_parent;
	HWND m_hWnd;

	int get_time_input()
	{
		HWND hDlg = m_hWnd;
		if (IsDlgButtonChecked(hDlg, IDC_UNLIMITED) == BST_CHECKED)
			return -1;
		UINT minutes = GetDlgItemInt(hDlg, IDC_MINUTES, NULL, FALSE);
		UINT seconds = GetDlgItemInt(hDlg, IDC_SECONDS, NULL, FALSE);
		return (int) (60 * minutes + seconds);
	}

	int get_silence_input()
	{
		HWND hDlg = m_hWnd;
		if (IsDlgButtonChecked(hDlg, IDC_SILENCE) != BST_CHECKED)
			return -1;
		return GetDlgItemInt(hDlg, IDC_SILSECONDS, NULL, FALSE);
	}

	bool get_loops_input()
	{
		return IsDlgButtonChecked(m_hWnd, IDC_LOOPS) == BST_CHECKED;
	}

	int get_mute_input()
	{
		HWND hDlg = m_hWnd;
		int mask = 0;
		for (int i = 0; i < 8; i++)
			if (IsDlgButtonChecked(hDlg, IDC_MUTE1 + i) == BST_CHECKED)
				mask |= 1 << i;
		return mask;
	}

public:
	preferences_page_instance_asap(HWND parent) : m_parent(parent)
	{
		m_hWnd = CreateDialog(core_api::get_my_instance(), MAKEINTRESOURCE(IDD_SETTINGS), parent, ::settings_dialog_proc);
	}

	virtual t_uint32 get_state()
	{
		if (song_length != get_time_input()
		 || silence_seconds != get_silence_input()
		 || play_loops != get_loops_input())
			return preferences_state::changed /* | preferences_state::needs_restart_playback */ | preferences_state::resettable;
		if (mute_mask != get_mute_input())
			return preferences_state::changed | preferences_state::resettable;
		return preferences_state::resettable;
	}

	virtual HWND get_wnd()
	{
		return m_hWnd;
	}

	virtual void apply()
	{
		song_length = get_time_input();
		silence_seconds = get_silence_input();
		play_loops = get_loops_input();
		mute_mask = get_mute_input();
		input_asap::g_set_mute_mask(mute_mask);
		g_callback->on_state_changed();
	}

	virtual void reset()
	{
		settingsDialogSet(m_hWnd, -1, -1, FALSE, 0);
		g_callback->on_state_changed();
	}
};

class preferences_page_asap : public preferences_page_v3
{
public:
	virtual preferences_page_instance::ptr instantiate(HWND parent, preferences_page_callback::ptr callback)
	{
		g_callback = callback;
		return new service_impl_t<preferences_page_instance_asap>(parent);
	}

	virtual const char *get_name()
	{
		return "ASAP";
	}

	virtual GUID get_guid()
	{
		static const GUID a_guid =
			{ 0xf7c0a763, 0x7c20, 0x4b64, { 0x92, 0xbf, 0x11, 0xe5, 0x5d, 0x8, 0xe5, 0x53 } };
		return a_guid;
	}

	virtual GUID get_parent_guid()
	{
		return guid_input;
	}

	virtual bool get_help_url(pfc::string_base &p_out)
	{
		p_out = "http://asap.sourceforge.net/";
		return true;
	}
};

static service_factory_single_t<preferences_page_asap> g_preferences_page_asap_factory;


/* File types ------------------------------------------------------------ */

static const char * const names_and_masks[][2] = {
	{ "Slight Atari Player", "*.SAP" },
	{ "Chaos Music Composer", "*.CMC;*.CM3;*.CMR;*.CMS;*.DMC" },
	{ "Delta Music Composer", "*.DLT" },
	{ "Music ProTracker", "*.MPT;*.MPD" },
	{ "Raster Music Tracker", "*.RMT" },
	{ "Theta Music Composer 1.x", "*.TMC;*.TM8" },
	{ "Theta Music Composer 2.x", "*.TM2" }
};

#define N_FILE_TYPES (sizeof(names_and_masks) / sizeof(names_and_masks[0]))

class input_file_type_asap : public service_impl_single_t<input_file_type>
{
public:

	virtual unsigned get_count()
	{
		return N_FILE_TYPES;
	}

	virtual bool get_name(unsigned idx, pfc::string_base &out)
	{
		if (idx < N_FILE_TYPES) {
			out = ::names_and_masks[idx][0];
			return true;
		}
		return false;
	}

	virtual bool get_mask(unsigned idx, pfc::string_base &out)
	{
		if (idx < N_FILE_TYPES) {
			out = ::names_and_masks[idx][1];
			return true;
		}
		return false;
	}

	virtual bool is_associatable(unsigned idx)
	{
		return true;
	}
};

static service_factory_single_t<input_file_type_asap> g_input_file_type_asap_factory;

DECLARE_COMPONENT_VERSION("ASAP", ASAP_VERSION, ASAP_CREDITS "\n" ASAP_COPYRIGHT);
