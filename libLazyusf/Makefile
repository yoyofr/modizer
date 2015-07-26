CFLAGS = -c -fPIC

OBJS = audio.o cpu.o dma.o exception.o interpreter_cpu.o interpreter_ops.o main.o pif.o registers.o tlb.o usf.o memory.o rsp/rsp.o rsp_hle/alist.o rsp_hle/alist_nead.o rsp_hle/jpeg.o rsp_hle/mp3.o rsp_hle/alist_audio.o rsp_hle/audio.o rsp_hle/hle.o rsp_hle/musyx.o rsp_hle/alist_naudio.o rsp_hle/cicx105.o rsp_hle/memory.o rsp_hle/plugin.o


OPTS = -O3
ROPTS = -O3 -DARCH_MIN_SSE2

all: liblazyusf.a

liblazyusf.a : $(OBJS)
	$(AR) rcs $@ $^

.c.o:
	$(CC) $(CFLAGS) $(OPTS) -o $@ $*.c

rsp/rsp.o: rsp/rsp.c
	$(CC) $(CFLAGS) $(ROPTS) -o $@ $^

clean:
	rm -f $(OBJS) liblazyusf.a > /dev/null

