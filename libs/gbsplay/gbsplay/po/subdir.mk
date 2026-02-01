ifeq ($(have_xgettext),yes)

pos  := $(wildcard po/*.po)
mos  := $(patsubst %.po,%.mo,$(pos))
dsts += $(mos)

ifeq ($(MAKECMDGOALS),update-po)

.PHONY: update-po
update-po: po/gbsplay.pot $(pos) $(mos)

po/gbsplay.pot: $(wildcard *.[ch])
	xgettext -k_ -kN_ --language c $+ -o - | msgen -o $@ -

po/po.d: po/subdir.mk
	for i in po/*.po; do \
	echo "$$i: po/gbsplay.pot"; \
	echo "	msgmerge --no-location --no-wrap --no-fuzzy-matching -s -q -U \$$@ \$$< && touch \$$@"; \
	done > $@

-include po/po.d

endif

%.mo: %.po
	msgfmt -o $@ $<

endif
