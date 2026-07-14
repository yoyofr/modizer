/**
 * furnace_chip_rename.h
 *
 * Injected via -include into every libfurnace translation unit.
 * Renames chip emulator symbols that conflict with other Modizer static libs
 * (libsnsf, liblibsidplayfp, liblibnsfplay, libgme).
 *
 * No furnace source file is modified.
 */

#ifndef FURNACE_CHIP_RENAME_H
#define FURNACE_CHIP_RENAME_H

/* ── SPC_DSP + SPC_State_Copier (SNES DSP) ── conflicts with libsnsf.a ───── */
#define SPC_DSP                 furnaceSPC_DSP
#define SPC_State_Copier        furnaceSPC_State_Copier

/* ── reSIDfp namespace + matrix template ── conflicts with liblibsidplayfp.a ── */
#define reSIDfp                 furnaceReSIDfp
#define matrix                  furnaceMatrix
#define matrix_t                furnaceMatrix_t
/* array.h declares `counter` and `counterOps` at global scope, and siddefs-fp.h
   defines residfp_version_string — all duplicated by liblibsidplayfp/liblibresidfp. */
#define counter                 furnaceCounter
#define counterOps              furnaceCounterOps
#define residfp_version_string  furnaceResidfp_version_string

/* ── ymfm namespace ── conflicts with liblibpmdmini.a (older ymfm copy) ───── */
#define ymfm                    furnaceYmfm

/* ── xgm namespace (NSFPlay) ── conflicts with liblibnsfplay.a ───────────── */
#define xgm                     furnaceXgm

/* ── SID (plain, c64/sid2) ── conflicts with libwebsid.a ────────────────── */
#define SID                     furnaceSID

/* ── OPL3 (nuked-opl3) ── conflicts with libadplug.a ────────────────────── */
#define opl3_chip               furnaceOpl3Chip
#define opl3_slot               furnaceOpl3Slot
#define opl3_channel            furnaceOpl3Channel
#define OPL3_Generate           furnaceOPL3_Generate
#define OPL3_GenerateResampled  furnaceOPL3_GenerateResampled
#define OPL3_Reset              furnaceOPL3_Reset
#define OPL3_Resample           furnaceOPL3_Resample
#define OPL3_WriteReg           furnaceOPL3_WriteReg
#define OPL3_WriteRegBuffered   furnaceOPL3_WriteRegBuffered
#define OPL3_GenerateStream     furnaceOPL3_GenerateStream
#define OPL3_Generate4Ch        furnaceOPL3_Generate4Ch
#define OPL3_Generate4ChResampled furnaceOPL3_Generate4ChResampled
#define OPL3_Generate4ChStream  furnaceOPL3_Generate4ChStream

/* ── emu2413 / OPLL ── conflicts with libgme.a ───────────────────────────── */
#define OPLL                    furnaceOPLL
#define OPLL_PATCH              furnaceOPLL_PATCH
#define OPLL_RateConv           furnaceOPLL_RateConv
#define OPLL_new                furnaceOPLL_new
#define OPLL_delete             furnaceOPLL_delete
#define OPLL_reset              furnaceOPLL_reset
#define OPLL_resetPatch         furnaceOPLL_resetPatch
#define OPLL_setRate            furnaceOPLL_setRate
#define OPLL_setQuality         furnaceOPLL_setQuality
#define OPLL_setPan             furnaceOPLL_setPan
#define OPLL_setPanFine         furnaceOPLL_setPanFine
#define OPLL_setChipType        furnaceOPLL_setChipType
#define OPLL_writeIO            furnaceOPLL_writeIO
#define OPLL_writeReg           furnaceOPLL_writeReg
#define OPLL_calcStereo         furnaceOPLL_calcStereo
#define OPLL_setPatch           furnaceOPLL_setPatch
#define OPLL_copyPatch          furnaceOPLL_copyPatch
#define OPLL_forceRefresh       furnaceOPLL_forceRefresh
#define OPLL_dumpToPatch        furnaceOPLL_dumpToPatch
#define OPLL_patchToDump        furnaceOPLL_patchToDump
#define OPLL_getDefaultPatch    furnaceOPLL_getDefaultPatch
#define OPLL_setMask            furnaceOPLL_setMask
#define OPLL_toggleMask         furnaceOPLL_toggleMask
#define OPLL_RateConv_new       furnaceOPLL_RateConv_new
#define OPLL_RateConv_reset     furnaceOPLL_RateConv_reset
#define OPLL_RateConv_putData   furnaceOPLL_RateConv_putData
#define OPLL_RateConv_delete    furnaceOPLL_RateConv_delete
#define OPLL_calc               furnaceOPLL_calc

#endif /* FURNACE_CHIP_RENAME_H */
