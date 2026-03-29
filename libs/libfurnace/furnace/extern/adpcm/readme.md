### adpcm

ADPCM encoder/decoder.

#### Compiling

	make

#### Usage

	adpcm <command> <input> <output>
	
#### Commands

| Command | Format | Description |
| --- | --- | --- |
| ad      | Yamaha ADPCM-A (YM2610) | Decode |
| ae      | Yamaha ADPCM-A (YM2610) | Encode |
| bd      | Yamaha ADPCM-B (Y8950/YM2608/YM2610) | Decode |
| be      | Yamaha ADPCM-B (Y8950/YM2608/YM2610) | Encode |
| cd      | Yamaha AICA (AICA) | Decode |
| ce      | Yamaha AICA (AICA) | Encode |
| od      | Oki/Dialogic "VOX" (MSM6295) | Decode |
| oe      | Oki/Dialogic "VOX" (MSM6295) | Encode |
| sd      | Brian Schmidt (BSMT2000/QSound) | Decode |
| se      | Brian Schmidt (BSMT2000/QSound) | Encode |
| xd      | Oki X68000 (MSM6258) | Decode |
| xe      | Oki X68000 (MSM6258) | Encode |
| zd      | Yamaha/Creative (YMZ280B) | Decode |
| ze      | Yamaha/Creative (YMZ280B) | Encode |

#### Library

| File | Prefix | Description |
| --- | --- | --- |
| bs_codec.[c/h] | bs_ | BSMT2000/QSound |
| oki_codec.[c/h] | oki_, oki6258_ | Oki/Dialogic/VOX |
| yma_codec.[c/h] | yma_ | ADPCM-A |
| ymb_codec.[c/h] | ymb_ | ADPCM-B |
| ymz_codec.[c/h] | ymz_, aica_ | YMZ280B, AICA |

#### Copyright

Ian Karlsson 2019.

Public domain.
