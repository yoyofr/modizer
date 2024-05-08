@ neon register map:
@  0,  3,  6,  9, 12, 15 ssg1
@  1,  4,  7, 10, 13, 16 ssg2
@  2,  5,  8, 11, 14, 17 ssg3
@ 18, 19, 20, 21, 22, 23 sinc
@ 24-25 (q12): ssg1 out
@ 26-27 (q13): ssg2 out
@ 28-29 (q14): ssg3 out

.global opna_ssg_sinc_calc_neon
@ r0: resampler_index
@ r1: const int16_t *inbuf
@ r2: int32_t *outbuf

opna_ssg_sinc_calc_neon:
@ sinc table to r3
  movw r3, #:lower16:opna_ssg_sinctable
  movt r3, #:upper16:opna_ssg_sinctable
  tst r0, #1
  addeq r3, #256

@ add offset to ssg input buffer address
  bic r0, #1
  add r1, r0, lsl #2

@ initialize output register
  vmov.i64 q12, #0
  vmov.i64 q13, #0
  vmov.i64 q14, #0

@ sinc sample length
  mov r0, #128

.loop:
@
  subs r0, #24
  blo .end

@ load SSG channel data
  vld4.16 {d0-d3}, [r1]!
  vld4.16 {d3-d6}, [r1]!
  vld4.16 {d6-d9}, [r1]!
  vld4.16 {d9-d12}, [r1]!
  vld4.16 {d12-d15}, [r1]!
  vld4.16 {d15-d18}, [r1]!

@ load sinc data
  vld1.16 {d18-d21}, [r3]!
  vld1.16 {d22-d23}, [r3]!

@ multiply and accumulate
  vmlal.s16 q12, d0,  d18
  vmlal.s16 q13, d1,  d18
  vmlal.s16 q14, d2,  d18
  vmlal.s16 q12, d3,  d19
  vmlal.s16 q13, d4,  d19
  vmlal.s16 q14, d5,  d19
  vmlal.s16 q12, d6,  d20
  vmlal.s16 q13, d7,  d20
  vmlal.s16 q14, d8,  d20
  vmlal.s16 q12, d9,  d21
  vmlal.s16 q13, d10, d21
  vmlal.s16 q14, d11, d21
  vmlal.s16 q12, d12, d22
  vmlal.s16 q13, d13, d22
  vmlal.s16 q14, d14, d22
  vmlal.s16 q12, d15, d23
  vmlal.s16 q13, d16, d23
  vmlal.s16 q14, d17, d23
  b .loop

.end:
@ 8 samples left
  vld4.16 {d0-d3}, [r1]!
  vld4.16 {d3-d6}, [r1]
  vld1.16 {d18-d19}, [r3]

  vmlal.s16 q12, d0, d18
  vmlal.s16 q13, d1, d18
  vmlal.s16 q14, d2, d18
  vmlal.s16 q12, d3, d19
  vmlal.s16 q13, d4, d19
  vmlal.s16 q14, d5, d19

@ extract data from result SIMD registers
  vpadd.i32 d0, d24, d25
  vpadd.i32 d1, d26, d27
  vpadd.i32 d2, d28, d29
  vpaddl.u32 d3, d0
  vpaddl.u32 d4, d1
  vpaddl.u32 d5, d2

  vst3.32 {d3[0]-d5[0]}, [r2]

  bx lr
