../in_snsf.dll: $(patsubst %.cpp, %.obj, $(wildcard snes9x/*.cpp snes9x/*/*.cpp *.cpp)) ../in_xsf_framework.lib
	$(WINE) link.exe /nologo /dll /machine:x86 user32.lib libucrt.lib libvcruntime.lib libcmt.lib libcpmt.lib /out:$@ $^

%.obj: %.cpp $(wildcard snes9x/*.h ../in_xsf_framework/*.h) GNUmakefile
	$(WINE) cl.exe /nologo /std:c++latest /MT /DUNICODE /D_UNICODE /DNDEBUG /O2 /EHsc /I ../in_xsf_framework /I ../in_xsf_framework/zlib /I snes9x /c /Fo$@ $<

clean:
	rm -f ../in_snsf.dll ../in_snsf.exp ../in_snsf.lib *.obj snes9x/*.obj snes9x/*/*.obj

.PHONY: clean
