#!/usr/bin/env python3

from optparse import OptionParser
import re
import sys


SYM_INIT = '_gbs_init'
SYM_PLAY = '_gbs_play'


def parse_syms(f):
  syms = {}
  lines = f.readlines()
  for line in lines:
    m = re.match(r'^DEF\s+(\S+)\s+(\S+)', line)
    if not m:
      continue
    syms[m.group(1)] = int(m.group(2), 0)
  return syms


def word(val):
  return bytes([val & 0xff, val >> 8])


def string(s):
  padded = bytes(s.encode('ascii')) + 32*bytes([0])
  return padded[:32]


def convert(
    rom, syms, first_song=None, num_songs=None,
    timer_modulo=None, timer_control=None,
    title=None, author=None, copyright=None):

  for required in (SYM_INIT, SYM_PLAY):
    if required not in syms:
      raise ValueError(f'Missing symbol for entry point: {required}')

  data = [
    b'GBS',
    bytes([
      1,  # GBS version (1)
      num_songs or 1,
      first_song or 1,
    ]),
    word(0x0400),          # load address
    word(syms[SYM_INIT]),  # init address
    word(syms[SYM_PLAY]),  # play address
    word(0xe000),          # stack pointer
    bytes([
      timer_modulo or 0,
      timer_control or 0,
    ]),
    string(title or 'Untitled'),
    string(author or 'Anonymous'),
    string(copyright or 'Copyleft'),
    rom.read()[0x400:],
  ]
  return b''.join(data)


def main(options):
  if not (options.symfile and options.romfile and options.outfile):
    raise ValueError('Usage: makegbs.py -r <romfile> -s <symfile> -o <outfile>')

  with open(options.symfile, 'r', encoding='utf8') as symf:
    syms = parse_syms(symf)

  with open(options.romfile, 'rb') as rom:
    gbs = convert(rom, syms)

  with open(options.outfile, 'wb') as outf:
    outf.write(gbs)


if __name__ == '__main__':
  parser = OptionParser()
  parser.add_option('-r', '--rom', dest='romfile',
                    help='ROM file to read')
  parser.add_option('-s', '--sym', dest='symfile',
                    help='Symbol file to read')
  parser.add_option('-o', '--out', dest='outfile',
                    help='Output file to write')
  parser.add_option('-a', '--author', dest='author',
                    help='Author string')
  parser.add_option('-t', '--title', dest='title',
                    help='Title string')
  parser.add_option('-c', '--copyright', dest='copyright',
                    help='Copyright string')

  (options, _) = parser.parse_args()
  main(options)
