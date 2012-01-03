JRE = C:/Program Files/Java/jdk1.6.0_10/jre

JAVAC = javac -source 1.2 -d .
#JAVAC = gcj -C -d .
JAR = jar
JAR_OPTIMIZE = 7z a -mx=9 -tzip
JAVADOC = javadoc
PP = cpp -C -P -I..

COMMON_C = ../asap.c ../acpu.c ../apokeysnd.c
COMMON_H = ../asap.h ../asap_internal.h ../anylang.h
PLAYERS_OBX_DIR = net/sf/asap

all: asap2wav.jar asap_applet.jar j2me

include ../players/Makefile

asap2wav.jar: asap2wav.MF ASAP2WAV.class net/sf/asap/ASAP.class $(PLAYERS_OBX)
	$(JAR) cfm $@ asap2wav.MF ASAP2WAV.class net/sf/asap
	$(JAR_OPTIMIZE) $@ ASAP2WAV.class net/sf/asap

ASAP2WAV.class: ASAP2WAV.java net/sf/asap/ASAP.class $(PLAYERS_OBX)
	$(JAVAC) -classpath . ASAP2WAV.java

asap_applet.jar: ASAPApplet.class net/sf/asap/ASAP.class $(PLAYERS_OBX)
	$(JAR) cf $@ ASAPApplet.class net/sf/asap
	$(JAR_OPTIMIZE) $@ ASAPApplet.class net/sf/asap

ASAPApplet.class: ASAPApplet.java net/sf/asap/ASAP.class
	$(JAVAC) -classpath "$(JRE)/lib/plugin.jar;." ASAPApplet.java

j2me:
	$(MAKE) -C j2me asap_midlet.jad

net/sf/asap/ASAP.class: ASAP.java net/sf/asap/ASAP_ModuleInfo.class
	$(JAVAC) -classpath . ASAP.java

ASAP.java: ASAP.ppjava $(COMMON_C) $(COMMON_H)
	$(PP) ASAP.ppjava > ASAP.java

net/sf/asap/ASAP_ModuleInfo.class: ASAP_ModuleInfo.java
	$(JAVAC) -classpath . ASAP_ModuleInfo.java

doc: ASAP.java ASAP_ModuleInfo.java
	$(JAVADOC) -d doc ASAP.java ASAP_ModuleInfo.java

clean:
	rm -f asap2wav.jar asap_applet.jar *.class ASAP.java
	rm -rf net doc
	$(MAKE) -C j2me clean
	$(MAKE) -C android clean

.PHONY: all j2me clean

.DELETE_ON_ERROR:
