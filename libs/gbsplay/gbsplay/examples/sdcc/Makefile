SDCC ?= sdcc
MAKEBIN ?= makebin
SDCCFLAGS := -mgbz80 --code-loc 0x400 --opt-code-size

all: test.gb test.gbs

clean:
	rm -f *.ihx *.map *.rel *.noi *.gb *.gbs *.sym *.lk *.lst *.asm

%.ihx: %.c
	$(SDCC) $(SDCCFLAGS) $<

%.gb: %.ihx
	$(MAKEBIN) -Z -yn TEST $< $@

%.img: %.ihx
	$(MAKEBIN) -Z -p -yn TEST $< $@

%.gbs: %.img
	./makegbs.py -r $< -s $(subst .img,.noi,$<) -o $@
