all: asap.swf

include ../players/Makefile

asap.swf: ASAPPlayer.as ASAP.as ASAP_ModuleInfo.as PokeyState.as
	mxmlc -o $@ -compiler.optimize -compiler.warn-duplicate-variable-def=false -static-link-runtime-shared-libraries -target-player 10 ASAPPlayer.as
# -compiler.debug -compiler.warn-no-type-decl=false

ASAP.as: ASAP.ppas ../asap.c ../acpu.c ../apokeysnd.c ../asap.h ../asap_internal.h ../anylang.h ../players.h
	cpp -C -P -I.. $< > $@
# -DASAP_ONLY_SAP

debug:
	tail -f "${APPDATA}\Macromedia\Flash Player\Logs\flashlog.txt"

doc: ASAP.as ASAP_ModuleInfo.as PokeyState.as
	asdoc -compiler.warn-duplicate-variable-def=false -output=doc -window-title="ASAP ActionScript API" -main-title="ASAP (Another Slight Atari Player) ActionScript API" -doc-sources=ASAP.as,ASAP_ModuleInfo.as,PokeyState.as

clean:
	rm -f asap.swf ASAP.as
	rm -rf doc

.DELETE_ON_ERROR:
