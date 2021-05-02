#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kssplay.h"

#ifdef EMSCRIPTEN
#include <emscripten.h>
#endif

#define MAX_RATE 192000
#define MAX_PATH 256

static void WORD(char *buf, uint32_t data) {
  buf[0] = data & 0xff;
  buf[1] = (data & 0xff00) >> 8;
}

static void DWORD(char *buf, uint32_t data) {
  buf[0] = data & 0xff;
  buf[1] = (data & 0xff00) >> 8;
  buf[2] = (data & 0xff0000) >> 16;
  buf[3] = (data & 0xff000000) >> 24;
}

static void chunkID(char *buf, char id[4]) {
  buf[0] = id[0];
  buf[1] = id[1];
  buf[2] = id[2];
  buf[3] = id[3];
}

static void create_wav_header(char *header, int rate, int nch, int play_time) {
  chunkID(header, "RIFF");
  DWORD(header + 4, 0); /* data length * bytePerSample + 36 */
  chunkID(header + 8, "WAVE");
  chunkID(header + 12, "fmt ");
  DWORD(header + 16, 16);
  WORD(header + 20, 1);               /* WAVE_FORMAT_PCM */
  WORD(header + 22, nch);             /* channel 1=mono,2=stereo */
  DWORD(header + 24, rate);           /* samplesPerSec */
  DWORD(header + 28, nch * 2 * rate); /* bytesPerSec */
  WORD(header + 32, 2);               /* blockSize */
  WORD(header + 34, 16);              /* bitsPerSample */
  chunkID(header + 36, "data");
  DWORD(header + 40, rate * nch * play_time * 2); /* data length * bytePerSample */
}

#define HLPMSG                                                                                                         \
  "Usage: kss2wav [Options] FILENAME.KSS \n"                                                                           \
  "Options: \n"                                                                                                        \
  "  -f<fade_time>  Fade-out duration in seconds.\n"                                                                   \
  "  -l<loop_num>   Number of loops\n"                                                                                 \
  "  -n[1|2]        Number of channels (default:1)\n"                                                                  \
  "  -o<file>       Output filename. -o- means stdout.\n"                                                                                 \
  "  -p<play_time>  Play time in seconds\n"                                                                            \
  "  -q<quality>    Rendering quality 0:LOW 1:HIGH (default:1)\n"                                                      \
  "  -r<play_freq>  Specify the frequency (default:44100)\n"                                                           \
  "  -s<song_num>   Song number to play\n"                                                                             \
  "Note: spaces are not accepted between the option character and its parameter.\n"

typedef struct {
  int rate;
  int nch;
  int bps;
  int song_num;
  int play_time;
  int fade_time;
  int loop_num;
  int quality;
  char input[MAX_PATH + 4];
  char output[MAX_PATH + 4];
  int use_stdout;
  int help;
  int error;
} Options;

static Options parse_options(int argc, char **argv) {

  Options options;
  int i, j;

  options.rate = 44100;
  options.nch = 1;
  options.song_num = 0;
  options.play_time = 60;
  options.fade_time = 5;
  options.loop_num = 1;
  options.input[0] = '\0';
  options.output[0] = '\0';
  options.use_stdout = 0;
  options.help = 0;
  options.error = 0;
  options.quality = 1;

  for (i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      switch (argv[i][1]) {
      case 'n':
        options.nch = (2 == atoi(argv[i] + 2)) ? 2 : 1;
        break;
      case 'p':
        options.play_time = atoi(argv[i] + 2);
        break;
      case 's':
        options.song_num = atoi(argv[i] + 2);
        break;
      case 'f':
        options.fade_time = atoi(argv[i] + 2);
        break;
      case 'r':
        options.rate = atoi(argv[i] + 2);
        break;
      case 'q':
        options.quality = atoi(argv[i] + 2);
        break;
      case 'l':
        options.loop_num = atoi(argv[i] + 2);
        if (options.loop_num == 0) {
          options.loop_num = 256;
        }
        break;
      case 'o':
        for (j = 0; argv[i][j + 2]; j++) {
          options.output[j] = argv[i][j + 2];
        }
        options.output[j] = '\0';
        if (options.output[0] == '-' && options.output[1] == '\0') {
          options.use_stdout = 1;
        }
        break;
      default:
        options.error = 1;
        break;
      }
    } else {
      strncpy(options.input, argv[i], MAX_PATH);
    }
  }

  /* Create Default WAV filename */
  if (options.output[0] == '\0') {
    for (i = 0; options.input[i] != '\0'; i++) {
      if (options.input[i] == '.') {
        break;
      }
      options.output[i] = options.input[i];
    }
    options.output[i] = '\0';
    strcat(options.output, ".wav");
  }

  if (options.rate > MAX_RATE) {
    options.rate = 44100;
  }

  return options;
}

int main(int argc, char *argv[]) {

  char header[46];
  int i, t;

  KSSPLAY *kssplay;
  KSS *kss;
  FILE *fp;

#ifdef EMSCRIPTEN
  EM_ASM(FS.mkdir('/data'); FS.mount(NODEFS, {root : '.'}, '/data'););
#endif

  if (argc < 2) {
    fprintf(stderr, HLPMSG);
    exit(0);
  }

  Options opt = parse_options(argc, argv);

  if (opt.error) {
    fprintf(stderr, HLPMSG);
    exit(1);
  }

  fprintf(stderr, "SONG:%02d, PLAYTIME:%d(sec), FADETIME:%d(sec), RATE:%d\n", opt.song_num, opt.play_time, opt.fade_time,
         opt.rate);

  if ((kss = KSS_load_file(opt.input)) == NULL) {
    fprintf(stderr, "FILE READ ERROR!\n");
    exit(1);
  }

  /* Print title strings */
  fprintf(stderr, "[%s]", kss->idstr);
  fprintf(stderr, "%s\n", kss->title);
  if (kss->extra)
    fprintf(stderr, "%s\n", kss->extra);

  /* Open Output file */
  if (opt.use_stdout) {
    fp = stdout;
  } else if ((fp = fopen(opt.output, "wb")) == NULL) {
    fprintf(stderr, "Can't open %s\n", opt.output);
    exit(1);
  }

  create_wav_header(header, opt.rate, opt.nch, opt.play_time);

  fwrite(header, sizeof(header), 1, fp); /* Write dummy header */

  /* INIT KSSPLAY */
  kssplay = KSSPLAY_new(opt.rate, opt.nch, 16);
  KSSPLAY_set_data(kssplay, kss);
  KSSPLAY_reset(kssplay, opt.song_num, 0);

  KSSPLAY_set_device_quality(kssplay, EDSC_PSG, opt.quality);
  KSSPLAY_set_device_quality(kssplay, EDSC_SCC, opt.quality);
  KSSPLAY_set_device_quality(kssplay, EDSC_OPLL, opt.quality);

  if(opt.nch > 1) {
    KSSPLAY_set_device_pan(kssplay, EDSC_PSG, -32);
    KSSPLAY_set_device_pan(kssplay, EDSC_SCC,  32);
    kssplay->opll_stereo = 1;
    KSSPLAY_set_channel_pan(kssplay, EDSC_OPLL, 0, 1);
    KSSPLAY_set_channel_pan(kssplay, EDSC_OPLL, 1, 2);
    KSSPLAY_set_channel_pan(kssplay, EDSC_OPLL, 2, 1);
    KSSPLAY_set_channel_pan(kssplay, EDSC_OPLL, 3, 2);
    KSSPLAY_set_channel_pan(kssplay, EDSC_OPLL, 4, 1);
    KSSPLAY_set_channel_pan(kssplay, EDSC_OPLL, 5, 2);
  }

  int16_t *wavebuf = malloc(opt.rate * opt.nch * sizeof(int16_t));

  int32_t written = 0;

  /* Create WAV Data */
  for (t = 0; t < opt.play_time; t++) {

    fprintf(stderr, "%03d/%03d", t + 1, opt.play_time);
    fflush(stderr);

    KSSPLAY_calc(kssplay, wavebuf, opt.rate);
    for (i = 0; i < opt.rate * opt.nch; i++) {
      /** The following function ensures that wave data is little-endian. */
      WORD((char *)(wavebuf + i), wavebuf[i]);
    }

    /* Write 1 sec wave block to file */
    fwrite(wavebuf, sizeof(int16_t), opt.rate * opt.nch, fp);
    written += opt.rate * opt.nch * sizeof(int16_t);

    /* If looped, start fadeout */
    if ((KSSPLAY_get_loop_count(kssplay) >= opt.loop_num || (opt.play_time - opt.fade_time) <= t + 1) &&
        KSSPLAY_get_fade_flag(kssplay) == KSSPLAY_FADE_NONE) {
      KSSPLAY_fade_start(kssplay, opt.fade_time * 1000);
    }

    /* If the fade is ended or the play is stopped, break */
    if (KSSPLAY_get_fade_flag(kssplay) == KSSPLAY_FADE_END || KSSPLAY_get_stop_flag(kssplay)) {
      break;
    }

    fprintf(stderr, "\x08\x08\x08\x08\x08\x08\x08");
  }

  fseek(fp, 0, SEEK_SET);
  DWORD(header + 4, 36 + written);
  fwrite(header, sizeof(header), 1, fp); /* Write dummy header */

  fclose(fp);

  free(wavebuf);

  KSSPLAY_delete(kssplay);
  KSS_delete(kss);

  fprintf(stderr, "\n");
  return 0;
}