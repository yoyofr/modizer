JRE ?= C:/Program Files/Java/jdk1.6.0_10/jre
WTK = C:/Program Files/Java/SE_SDK/WTK2

JAVAC ?= javac -source 1.2 -d .
JAR ?= jar
JAR_OPTIMIZE ?= 7z a -mx=9 -tzip
PERL = perl
ME_CLASSES = $(WTK)/lib/cldcapi11.jar;$(WTK)/lib/midpapi20.jar;$(WTK)/lib/jsr75.jar
PREVERIFY = "$(WTK)/bin/preverify1.1"
ME_EMU = "$(WTK)/bin/emulatorw" -gui

PLAYERS_SRC_DIR = ../../players
PLAYERS_OBX_DIR = ../net/sf/asap

all: asap_midlet.jad

include ../../players/Makefile

emu: asap_midlet.jad
	$(ME_EMU) -Xdescriptor:asap_midlet.jad

asap_midlet.jad: asap_midlet.jar MANIFEST.MF
	$(PERL) mf2jad.pl asap_midlet

asap_midlet.jar: MANIFEST.MF preverified/ASAPMIDlet.class $(PLAYERS_OBX)
	$(JAR) cfm $@ MANIFEST.MF -C preverified .
	# the following is needed and is not just an optimization
	cd .. && $(JAR_OPTIMIZE) j2me/$@ $(PLAYERS:%=net/sf/asap/%.obx)
	cd preverified && $(JAR_OPTIMIZE) ../$@ .

preverified/ASAPMIDlet.class: ASAPMIDlet.class ../net/sf/asap/ASAP.class ../net/sf/asap/ASAP_ModuleInfo.class
	$(PREVERIFY) -classpath "$(ME_CLASSES);.;.." -d preverified ASAPMIDlet FileList ASAPInputStream net.sf.asap.ASAP net.sf.asap.ASAP_ModuleInfo net.sf.asap.ASAP_State net.sf.asap.PokeyState

ASAPMIDlet.class: ASAPMIDlet.java ../net/sf/asap/ASAP.class
	$(JAVAC) -classpath ".;.." -bootclasspath "$(ME_CLASSES)" ASAPMIDlet.java

../net/sf/asap/ASAP.class:
	$(MAKE) -C .. net/sf/asap/ASAP.class

../net/sf/asap/ASAP_ModuleInfo.class:
	$(MAKE) -C .. net/sf/asap/ASAP_ModuleInfo.class

clean:
	rm -f asap_midlet.jar asap_midlet.jad *.class
	rm -rf preverified

.DELETE_ON_ERROR:
