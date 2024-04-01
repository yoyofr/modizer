#!/usr/bin/env bash
#
# Ensure that the WAV plugout produces valid data.
#
# 2022 (C) by Christian Garbs <mitch@cgarbs.de>
# Licensed under GNU GPL v1 or, at your option, any later version.

set -e

wav_header_length=44

log() {
    echo "> $*"
}

die() {
    echo
    echo "ERROR: $*" >&2
    exit 1
}

BEGIN_TEST() {
    echo
    echo "TEST: $*"
    echo
}

TEST_OK() {
    echo
    echo "test OK"
    echo
}

assert_that_equals() {
    local what=$1 actual=$2 expected=$3
    if [ "$actual" != "$expected" ]; then
	die "assert_that_equals on $what failed: expected \`$expected', but got \`$actual'"
    fi
}

assert_that_file_equals() {
    local actual=$1 expected=$2
    if ! diff -q "$actual" "$expected"; then
	die "file content differs between \`$actual' and \`$expected'"
    fi
}

run_gbsplay() {
    # - play only one second, use additional arguments
    # - include current directory in LD_LIBRARY_PATH for shared lib builds
    LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH ./gbsplay -qqq -c examples/gbsplayrc_sample -t 1 -f 1 "$@" examples/nightmode.gbs 1 1
}

split_result_wav() {
    dd ibs=$wav_header_length skip=1  if=gbsplay-1.wav of=gbsplay-1.wav.audio
    dd ibs=$wav_header_length count=1 if=gbsplay-1.wav of=gbsplay-1.wav.header
}

set_file_length() {
    local var=$1 file=$2
    printf -v "$var" '%d' "$(wc -c < "$file")"
}

write_byte() {
    local val=$1
    local hex
    printf -v hex "%02x" $(( val & 0xff ))
    printf "\x$hex"
}

write_uint32_t_le() {
    local val=$1
    write_byte $(( val       ))
    write_byte $(( val >>  8 ))
    write_byte $(( val >> 16 ))
    write_byte $(( val >> 32 ))
}

write_uint16_t_le() {
    local val=$1
    write_byte $(( val       ))
    write_byte $(( val >>  8 ))
}


BEGIN_TEST "WAV audio data should match stdout audio stream in little endian"

log "render raw audio data in little endian via stdout plugin"
run_gbsplay -El -o stdout > gbsplay-1.raw
set_file_length raw_length gbsplay-1.raw

log "render as wav"
run_gbsplay -o wav
split_result_wav

log "compare file sizes"
set_file_length header_length gbsplay-1.wav.header
set_file_length audio_length  gbsplay-1.wav.audio
assert_that_equals "header length" "$header_length" $wav_header_length
assert_that_equals "audio length"  "$audio_length"  $raw_length

log "compare audio data"
assert_that_file_equals gbsplay-1.wav.audio gbsplay-1.raw

TEST_OK

###

BEGIN_TEST "WAV header should be as expected"

(
    # RIFF header
    printf "RIFF"                            # chunk id
    write_uint32_t_le $(( 36 + raw_length )) # chunk size
    printf "WAVE"                            # format

    # fmt subchunk
    printf "fmt "           # subchunk id
    write_uint32_t_le 16    # subchunk size
    write_uint16_t_le 1     # PCM, linear, uncompressed
    write_uint16_t_le 2     # stereo
    write_uint32_t_le 48000 # samplerate
    write_uint32_t_le $(( 48000 * 2 * 16 / 8 )) # byte rate
    write_uint16_t_le $(( 2 * 16 / 8 ))         # bytes per sample (all channels)
    write_uint16_t_le 16    # bits per sample (single channel)

    # data subchunk header
    printf "data"                   # subchunk id
    write_uint32_t_le "$raw_length" # subchunk size
) > gbsplay-1.wav.header.expected
assert_that_file_equals gbsplay-1.wav.header gbsplay-1.wav.header.expected

TEST_OK

###

BEGIN_TEST "WAV audio data should ignore endian setting"

mv gbsplay-1.wav.header gbsplay-1.wav.header.orig
mv gbsplay-1.wav.audio  gbsplay-1.wav.audio.orig

log "render as wav with explicit -El option"
run_gbsplay -El -o wav
split_result_wav
assert_that_file_equals gbsplay-1.wav.header gbsplay-1.wav.header.orig
assert_that_file_equals gbsplay-1.wav.audio  gbsplay-1.wav.audio.orig

log "explicit -Eb option should fail"
if run_gbsplay -Eb -o wav; then
    die "Expected command to fail"
fi

TEST_OK

###

BEGIN_TEST "halving the sample rate should halve audio length"

log "render as wav in 24000kHz"
run_gbsplay -El -r 24000 -o stdout > gbsplay-1.raw
set_file_length raw_length_24000 gbsplay-1.raw
run_gbsplay -El -r 24000 -o wav
split_result_wav
set_file_length audio_length_24000 gbsplay-1.wav.audio
assert_that_equals "audio length" "$audio_length_24000" "$raw_length_24000"

TEST_OK
