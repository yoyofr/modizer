/*
 * gbsplay is a Gameboy sound player
 *
 * 2003-2025 (C) by Tobias Diedrich <ranma+gbsplay@tdiedrich.de>
 *                  Christian Garbs <mitch@cgarbs.de>
 *
 * Licensed under GNU GPL v1 or, at your option, any later version.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>

#include "common.h"
#include "cfgparser.h"
#include "libgbs.h"
#include "plugout.h"
#include "test.h"

typedef void (*cfg_parse_fn)(void* const ptr);

struct cfg_option {
	const char* const name;
	void* const ptr;
	const cfg_parse_fn parse_fn;
};

/* default values */

struct player_cfg cfg = {
	.fadeout = 3,
	.filter_type = CFG_FILTER_DMG,
	.loop_mode = LOOP_OFF,
	.play_mode = PLAY_MODE_LINEAR,
	.rate = 44100,
	.refresh_delay = 33, // ms
	.requested_endian = PLUGOUT_ENDIAN_AUTOSELECT,
	.silence_timeout = 2,
	.sound_name = PLUGOUT_DEFAULT,
	.subsong_gap = 2,
	.subsong_timeout = 2*60,
	.verbosity = 3,
};

/* forward declarations needed by configuration directives */
static void cfg_bool_as_int(void* const ptr);
static void cfg_endian(void* const ptr);
static void cfg_long(void* const ptr);
static void cfg_loop_mode(void* const ptr);
static void cfg_play_mode(void* const ptr);
static void cfg_string(void* const ptr);

/* configuration directives */
static const struct cfg_option options[] = {
	{ "endian", &cfg.requested_endian, cfg_endian },
	{ "fadeout", &cfg.fadeout, cfg_long },
	{ "filter_type", &cfg.filter_type, cfg_string },
	{ "loop", &cfg.loop_mode, cfg_bool_as_int },
	{ "loop_mode", &cfg.loop_mode, cfg_loop_mode },
	{ "output_plugin", &cfg.sound_name, cfg_string },
	{ "play_mode", &cfg.play_mode, cfg_play_mode },
	{ "rate", &cfg.rate, cfg_long },
	{ "refresh_delay", &cfg.refresh_delay, cfg_long },
	{ "silence_timeout", &cfg.silence_timeout, cfg_long },
	{ "subsong_gap", &cfg.subsong_gap, cfg_long },
	{ "subsong_timeout", &cfg.subsong_timeout, cfg_long },
	{ "verbosity", &cfg.verbosity, cfg_long },
	{ NULL, NULL, NULL }
};

struct endian_map {
	const char *keyword;
	enum plugout_endian endian;
};

static const struct endian_map endian_map[] = {
	{ "b",      PLUGOUT_ENDIAN_BIG },
	{ "big",    PLUGOUT_ENDIAN_BIG },
	{ "l",      PLUGOUT_ENDIAN_LITTLE },
	{ "little", PLUGOUT_ENDIAN_LITTLE },
	{ "n",      PLUGOUT_ENDIAN_NATIVE },
	{ "native", PLUGOUT_ENDIAN_NATIVE },
	{ NULL,     -1 }
};

struct loop_mode_map {
	const char *keyword;
	enum gbs_loop_mode loop_mode;
};

static const struct loop_mode_map loop_mode_map[] = {
	{ "none",   LOOP_OFF },
	{ "range",  LOOP_RANGE },
	{ "single", LOOP_SINGLE },
	{ NULL,     -1 }
};

struct play_mode_map {
	const char *keyword;
	enum play_mode play_mode;
};

static const struct play_mode_map play_mode_map[] = {
	{ "linear",  PLAY_MODE_LINEAR },
	{ "random",  PLAY_MODE_RANDOM },
	{ "shuffle", PLAY_MODE_SHUFFLE },
	{ NULL,     -1 }
};

static long cfg_line, cfg_char;
static FILE *cfg_file;

static long nextchar_state;

static char nextchar(void)
{
	long c;

	assert(cfg_file != NULL);

	do {
		if ((c = fgetc(cfg_file)) == EOF) return 0;

		if (c == '\n') {
			cfg_char = 0;
			cfg_line++;
		} else cfg_char++;

		switch (nextchar_state) {
		case 0:
			if (c == '\\') nextchar_state = 1;
			else if (c == '#') nextchar_state = 2;
			break;
		case 1:
			nextchar_state = 0;
			if (c == 'n') c = '\n';
			break;
		case 2:
			if (c == 0 || c == '\n') nextchar_state = 0;
			break;
		}
	} while (nextchar_state != 0);

	return (char)c;
}

static long state;
static long nextstate;
static long c;
static const char *filename;

static void err_expect(const char* const s)
{
	fprintf(stderr, _("'%s' expected at %s line %ld char %ld.\n"),
	        s, filename, cfg_line, cfg_char);
	c = nextchar();
	state = 0;
	nextstate = 1;
}

static void cfg_endian(void* const ptr)
{
	char *s;
	enum plugout_endian *endian = ptr;
	const struct endian_map *entry;

	// remember this because cfg_string() already reads the nextchar()
	long cfg_line_orig = cfg_line;
	long cfg_char_orig = cfg_char;

	cfg_string(&s);

	for (entry = endian_map; entry->keyword; entry++) {
		if (strncmp(s, entry->keyword, sizeof(s)-1) == 0) {
			*endian = entry->endian;
			return;
		}
	}

	fprintf(stderr, _("'%s' is no valid endian at %s line %ld char %ld.\n"),
		s, filename, cfg_line_orig, cfg_char_orig);
}

static void cfg_loop_mode(void* const ptr)
{
	char *s;
	enum gbs_loop_mode *loop_mode = ptr;
	const struct loop_mode_map *entry;

	// remember this because cfg_string() already reads the nextchar()
	long cfg_line_orig = cfg_line;
	long cfg_char_orig = cfg_char;

	cfg_string(&s);

	for (entry = loop_mode_map; entry->keyword; entry++) {
		if (strncmp(s, entry->keyword, sizeof(s)-1) == 0) {
			*loop_mode = entry->loop_mode;
			return;
		}
	}

	fprintf(stderr, _("'%s' is no valid loop mode at %s line %ld char %ld.\n"),
		s, filename, cfg_line_orig, cfg_char_orig);
}

static void cfg_play_mode(void* const ptr)
{
	char *s;
	enum play_mode *play_mode = ptr;
	const struct play_mode_map *entry;

	// remember this because cfg_string() already reads the nextchar()
	long cfg_line_orig = cfg_line;
	long cfg_char_orig = cfg_char;

	cfg_string(&s);

	for (entry = play_mode_map; entry->keyword; entry++) {
		if (strncmp(s, entry->keyword, sizeof(s)-1) == 0) {
			*play_mode = entry->play_mode;
			return;
		}
	}

	fprintf(stderr, _("'%s' is no valid play mode at %s line %ld char %ld.\n"),
		s, filename, cfg_line_orig, cfg_char_orig);
}

static void cfg_string(void* const ptr)
{
	char s[200];
	unsigned long n = 0;

	if (!isalpha(c) && c != '-' && c != '_') {
		err_expect("[a-zA-Z_-]");
		return;
	}
	do {
		s[n++] = c;
		c = nextchar();
	} while ((isalnum(c) || c == '-' || c == '_') &&
	         n < (sizeof(s)-1));
	s[n] = 0;

	*((char**)ptr) = strdup(s);

	state = 0;
	nextstate = 1;
}

static void cfg_long(void* const ptr)
{
	char num[20];
	unsigned long n = 0;

	if (!isdigit(c)) {
		err_expect("[0-9]");
		return;
	}
	do {
		num[n++] = c;
		c = nextchar();
	} while (isdigit(c) &&
	         n < (sizeof(num)-1));
	num[n] = 0;

	*((long*)ptr) = atoi(num);

	state = 0;
	nextstate = 1;
}

static void cfg_bool_as_int(void* const ptr)
{
	char num[10];
	unsigned int tmp;
	unsigned long n = 0;

	if (!isdigit(c)) {
		err_expect("[0-9]");
		return;
	}
	do {
		num[n++] = c;
		c = nextchar();
	} while (isdigit(c) &&
	         n < (sizeof(num)-1));
	num[n] = 0;

	tmp = atoi(num);
	*((int*)ptr) = tmp ? 1 : 0;

	state = 0;
	nextstate = 1;
}

void cfg_parse(const char* const fname)
{
	char option[200] = "";

	filename = fname;
	cfg_file = fopen(fname, "r");
	if (cfg_file == NULL) return;

	nextchar_state = 0;
	state = 0;
	nextstate = 1;
	cfg_line = 1;
	cfg_char = 0;
	c = nextchar();

	do {
		unsigned long n;
		switch (state) {
		case 0:
			if (isspace(c))
				while (isspace(c = nextchar()));
			state = nextstate;
			break;
		case 1:
			n = 0;
			if (!isalpha(c)) {
				err_expect("[a-zA-Z]");
				break;
			}
			do {
				option[n++] = c;
				c = nextchar();
			} while ((isalnum(c) ||
			          c == '-' ||
			          c == '_') &&
			          n < (sizeof(option)-1));
			option[n] = '\0';
			state = 0;
			nextstate = 2;
			break;
		case 2:
			if (c != '=') {
				err_expect("=");
				break;
			}
			state = 0;
			nextstate = 3;
			c = nextchar();
			break;
		case 3:
			n=0;
			while (options[n].name != NULL &&
			       strcmp(options[n].name, option) != 0) n++;
			if (options[n].parse_fn)
				options[n].parse_fn(options[n].ptr);
			else {
				fprintf(stderr, _("Unknown option %s at %s line %ld.\n"), option, fname, cfg_line);
				while ((c = nextchar()) != '\n' && c != 0);
				state = 0;
				nextstate = 1;
			}
			c = nextchar();
			break;
		}
	} while (c != 0);

	(void)fclose(cfg_file);
}

char* get_userconfig(const char* const cfgfile)
{
	char *homedir, *usercfg;
	long length;

	homedir = getenv("HOME");
	if (!homedir || !cfgfile) return NULL;

	length  = strlen(homedir) + strlen(cfgfile) + 2;
	usercfg = malloc(length);
	if (usercfg == NULL) {
		fprintf(stderr, "%s\n", _("Memory allocation failed!"));
		return NULL;
	}
	(void)snprintf(usercfg, length, "%s/%s", homedir, cfgfile);

	return usercfg;
}

/********************* test helpers *********************/

#ifdef ENABLE_TEST

#include <stdarg.h>

struct player_cfg initial_cfg;

#define TEST_GBSPLAYRC "./gbsplayrc.tmp"

#define ASSERT_STRUCT_EQUAL(fmt, field, a, b) ASSERT_EQUAL(#field " " fmt, (a).field, (b).field)

#define ASSERT_STRUCT_STRING_EQUAL(field, a, b) ASSERT_STRING_EQUAL(#field, (a).field, (b).field)

#define ASSERT_CFG_EQUAL(actual, expected) do { \
		ASSERT_STRUCT_EQUAL("%ld", fadeout,          actual, expected); \
		ASSERT_STRUCT_EQUAL("%d",  loop_mode,        actual, expected); \
		ASSERT_STRUCT_EQUAL("%d",  play_mode,        actual, expected); \
		ASSERT_STRUCT_EQUAL("%ld", rate,             actual, expected); \
		ASSERT_STRUCT_EQUAL("%ld", refresh_delay,    actual, expected); \
		ASSERT_STRUCT_EQUAL("%d",  requested_endian, actual, expected); \
		ASSERT_STRUCT_EQUAL("%ld", silence_timeout,  actual, expected); \
		ASSERT_STRUCT_EQUAL("%ld", subsong_gap,      actual, expected); \
		ASSERT_STRUCT_EQUAL("%ld", subsong_timeout,  actual, expected); \
		ASSERT_STRUCT_EQUAL("%ld", verbosity,        actual, expected); \
		ASSERT_STRUCT_STRING_EQUAL(filter_type,      actual, expected); \
		ASSERT_STRUCT_STRING_EQUAL(sound_name,       actual, expected); \
} while(0)

test void save_initial_cfg() {
	initial_cfg = cfg;
	initial_cfg.filter_type = strdup(cfg.filter_type);
	initial_cfg.sound_name  = strdup(cfg.sound_name);
};
TEST(save_initial_cfg);

test void restore_initial_cfg() {
	cfg = initial_cfg;
	cfg.filter_type = strdup(initial_cfg.filter_type);
	cfg.sound_name  = strdup(initial_cfg.sound_name);
};

test void write_test_gbsplayrc_n(unsigned int n, ...) {
	FILE *fh;
	unsigned int i;
	va_list lines;

	fh = fopen(TEST_GBSPLAYRC, "w");

	va_start(lines, n);
	for (unsigned int i = 0; i < n; i++) {
		fputs(va_arg(lines, char*), fh);
		fputc('\n', fh);
	}
	va_end(lines);
	fclose(fh);
}

test void write_test_gbsplayrc(const char line[]) {
	write_test_gbsplayrc_n(1, line);
}

test void delete_test_gbsplayrc() {
	unlink(TEST_GBSPLAYRC);
}

/************************* tests ************************/

test void test_parse_missing_config_file() {
	// given
	restore_initial_cfg();

	// when
	cfg_parse("gbsplayrc-this-file-does-not-exist");

	// then
	ASSERT_CFG_EQUAL(cfg, initial_cfg);
}
TEST(test_parse_missing_config_file);

test void test_parse_empty_configuration() {
	// given
	restore_initial_cfg();
	write_test_gbsplayrc_n(4,
			       "# just a comment",
			       "",
			       "",
			       "# and some empty lines");

	// when
	cfg_parse("test/gbsplayrc-empty");

	// then
	ASSERT_CFG_EQUAL(cfg, initial_cfg);
}
TEST(test_parse_empty_configuration);

test void test_parse_complete_configuration() {
	// given
	restore_initial_cfg();
	write_test_gbsplayrc_n(12,
			       "endian=little",
			       "fadeout=0",
			       "filter_type=cgb",
			       "loop=1",
			       "output_plugin=altmidi",
			       "play_mode=shuffle",
			       "rate=12345",
			       "refresh_delay=987",
			       "silence_timeout=19",
			       "subsong_gap=23",
			       "subsong_timeout=42",
			       "verbosity=5");

	// when
	cfg_parse(TEST_GBSPLAYRC);

	// then
	ASSERT_EQUAL("fadeout %ld",         cfg.fadeout,          0L);
	ASSERT_EQUAL("loop_mode %d",        cfg.loop_mode,        LOOP_RANGE);
	ASSERT_EQUAL("play_mode %d",        cfg.play_mode,        PLAY_MODE_SHUFFLE);
	ASSERT_EQUAL("rate %ld",            cfg.rate,             12345L);
	ASSERT_EQUAL("refresh_delay %ld",   cfg.refresh_delay,    987L);
	ASSERT_EQUAL("requested_endian %d", cfg.requested_endian, PLUGOUT_ENDIAN_LITTLE);
	ASSERT_EQUAL("silence_timeout %ld", cfg.silence_timeout,  19L);
	ASSERT_EQUAL("subsong_gap %ld",     cfg.subsong_gap,      23L);
	ASSERT_EQUAL("subsong_timeout %ld", cfg.subsong_timeout,  42L);
	ASSERT_EQUAL("verbosity   %ld"    , cfg.verbosity,        5L);
	ASSERT_STRING_EQUAL("filter_type",  cfg.filter_type,      CFG_FILTER_CGB);
	ASSERT_STRING_EQUAL("sound_name",   cfg.sound_name,       "altmidi");
}
TEST(test_parse_complete_configuration);

/* manpage says: an integer; 0 = false = no loop mode; everything else = true = range loop mode */
test void test_loop_0() {
	// given
	write_test_gbsplayrc("loop=0");
	cfg.loop_mode = LOOP_RANGE;

	// when
	cfg_parse(TEST_GBSPLAYRC);

	// then
	ASSERT_EQUAL("loop_mode %d", cfg.loop_mode, LOOP_OFF);
}
TEST(test_loop_0);

test void test_loop_1() {
	// given
	write_test_gbsplayrc("loop=1");
	cfg.loop_mode = LOOP_SINGLE;

	// when
	cfg_parse(TEST_GBSPLAYRC);

 	// then
	ASSERT_EQUAL("loop_mode %d", cfg.loop_mode, LOOP_RANGE);
}
TEST(test_loop_1);

test void test_loop_2() {
	// given
	write_test_gbsplayrc("loop=2");
	cfg.loop_mode = LOOP_OFF;

	// when
	cfg_parse(TEST_GBSPLAYRC);

	// then
	ASSERT_EQUAL("loop_mode %d", cfg.loop_mode, LOOP_RANGE);
}
TEST(test_loop_2);

test void test_loop_mode_none() {
	// given
	write_test_gbsplayrc("loop_mode=none");
	cfg.loop_mode = LOOP_RANGE;

	// when
	cfg_parse(TEST_GBSPLAYRC);

	// then
	ASSERT_EQUAL("loop_mode %d", cfg.loop_mode, LOOP_OFF);
}
TEST(test_loop_mode_none);

test void test_loop_mode_range() {
	// given
	write_test_gbsplayrc("loop_mode=range");
	cfg.loop_mode = LOOP_SINGLE;

	// when
	cfg_parse(TEST_GBSPLAYRC);

	// then
	ASSERT_EQUAL("loop_mode %d", cfg.loop_mode, LOOP_RANGE);
}
TEST(test_loop_mode_range);

test void test_loop_mode_single() {
	// given
	write_test_gbsplayrc("loop_mode=single");
	cfg.loop_mode = LOOP_RANGE;

	// when
	cfg_parse(TEST_GBSPLAYRC);

	// then
	ASSERT_EQUAL("loop_mode %d", cfg.loop_mode, LOOP_SINGLE);
}
TEST(test_loop_mode_single);

test void test_invalid_loop_mode() { // old setting is kept, warning in stderr
	// given
	write_test_gbsplayrc("loop_mode=INVALID");
	cfg.loop_mode = LOOP_RANGE;

	// when
	cfg_parse(TEST_GBSPLAYRC);

	// then
	ASSERT_EQUAL("loop_mode %d", cfg.loop_mode, LOOP_RANGE);
}
TEST(test_invalid_loop_mode);

test void test_loop_mode_after_loop() {
	// given
	write_test_gbsplayrc_n(2,
			       "loop=1",
			       "loop_mode=single");
	cfg.loop_mode = LOOP_RANGE;

	// when
	cfg_parse(TEST_GBSPLAYRC);

	// then
	ASSERT_EQUAL("loop_mode %d", cfg.loop_mode, LOOP_SINGLE);
}
TEST(test_loop_mode_after_loop);

test void test_loop_after_loop_mode() {
	// given
	write_test_gbsplayrc_n(2,
			       "loop_mode=single",
			       "loop=0");
	cfg.loop_mode = LOOP_RANGE;

	// when
	cfg_parse(TEST_GBSPLAYRC);

	// then
	ASSERT_EQUAL("loop_mode %d", cfg.loop_mode, LOOP_OFF);
}
TEST(test_loop_after_loop_mode);

test void test_endian_b() {
	// given
	write_test_gbsplayrc("endian=b");
	cfg.requested_endian = PLUGOUT_ENDIAN_NATIVE;

	// when
	cfg_parse(TEST_GBSPLAYRC);

	// then
	ASSERT_EQUAL("endian %d", cfg.requested_endian, PLUGOUT_ENDIAN_BIG);
}
TEST(test_endian_b);

test void test_endian_big() {
	// given
	write_test_gbsplayrc("endian=big");
	cfg.requested_endian = PLUGOUT_ENDIAN_NATIVE;

	// when
	cfg_parse(TEST_GBSPLAYRC);

	// then
	ASSERT_EQUAL("endian %d", cfg.requested_endian, PLUGOUT_ENDIAN_BIG);
}
TEST(test_endian_big);

test void test_endian_l() {
	// given
	write_test_gbsplayrc("endian=l");
	cfg.requested_endian = PLUGOUT_ENDIAN_BIG;

	// when
	cfg_parse(TEST_GBSPLAYRC);

	// then
	ASSERT_EQUAL("endian %d", cfg.requested_endian, PLUGOUT_ENDIAN_LITTLE);
}
TEST(test_endian_l);

test void test_endian_little() {
	// given
	write_test_gbsplayrc("endian=little");
	cfg.requested_endian = PLUGOUT_ENDIAN_BIG;

	// when
	cfg_parse(TEST_GBSPLAYRC);

	// then
	ASSERT_EQUAL("endian %d", cfg.requested_endian, PLUGOUT_ENDIAN_LITTLE);
}
TEST(test_endian_little);

test void test_endian_n() {
	// given
	write_test_gbsplayrc("endian=n");
	cfg.requested_endian = PLUGOUT_ENDIAN_LITTLE;

	// when
	cfg_parse(TEST_GBSPLAYRC);

	// then
	ASSERT_EQUAL("endian %d", cfg.requested_endian, PLUGOUT_ENDIAN_NATIVE);
}
TEST(test_endian_n);

test void test_endian_native() {
	// given
	write_test_gbsplayrc("endian=native");
	cfg.requested_endian = PLUGOUT_ENDIAN_LITTLE;

	// when
	cfg_parse(TEST_GBSPLAYRC);

	// then
	ASSERT_EQUAL("endian %d", cfg.requested_endian, PLUGOUT_ENDIAN_NATIVE);
}
TEST(test_endian_native);

test void test_invalid_endian() { // old setting is kept, warning in stderr
	// given
	write_test_gbsplayrc("endian=INVALID");
	cfg.requested_endian = PLUGOUT_ENDIAN_LITTLE;

	// when
	cfg_parse(TEST_GBSPLAYRC);

	// then
	ASSERT_EQUAL("endian %d", cfg.requested_endian, PLUGOUT_ENDIAN_LITTLE);
}
TEST(test_invalid_endian);

test void test_play_mode_linear() {
	// given
	write_test_gbsplayrc("play_mode=linear");
	cfg.play_mode = PLAY_MODE_RANDOM;

	// when
	cfg_parse(TEST_GBSPLAYRC);

	// then
	ASSERT_EQUAL("play_mode %d", cfg.play_mode, PLAY_MODE_LINEAR);
}
TEST(test_play_mode_linear);

test void test_play_mode_random() {
	// given
	write_test_gbsplayrc("play_mode=random");
	cfg.play_mode = PLAY_MODE_SHUFFLE;

	// when
	cfg_parse(TEST_GBSPLAYRC);

	// then
	ASSERT_EQUAL("play_mode %d", cfg.play_mode, PLAY_MODE_RANDOM);
}
TEST(test_play_mode_random);

test void test_play_mode_shuffle() {
	// given
	write_test_gbsplayrc("play_mode=shuffle");
	cfg.play_mode = PLAY_MODE_LINEAR;

	// when
	cfg_parse(TEST_GBSPLAYRC);

	// then
	ASSERT_EQUAL("play_mode %d", cfg.play_mode, PLAY_MODE_SHUFFLE);
}
TEST(test_play_mode_shuffle);

test void test_invalid_play_mode() { // old setting is kept, warning in stderr
	// given
	write_test_gbsplayrc("play_mode=INVALID");
	cfg.play_mode = PLAY_MODE_SHUFFLE;

	// when
	cfg_parse(TEST_GBSPLAYRC);

	// then
	ASSERT_EQUAL("play_mode %d", cfg.play_mode, PLAY_MODE_SHUFFLE);
}
TEST(test_invalid_play_mode);

TEST(delete_test_gbsplayrc); // cleanup
TEST_EOF;

#endif /* ENABLE_TEST */
