CSC = "C:/Windows/Microsoft.NET/Framework/v2.0.50727/csc.exe" -nologo -o+ -out:$@
PP = cpp -C -P -I..
NDOC = "C:/Program Files/NDoc3/bin/NDoc3Console.exe"
SEVENZIP = 7z a -mx=9
SL = "C:/Program Files/Reference Assemblies/Microsoft/Framework/Silverlight/v4.0"

all: asap2wav.exe asapplay.exe SilverASAP.xap

include ../players/Makefile

asap2wav.exe: asap2wav.cs ASAP.cs asapci.cs
	$(CSC) asap2wav.cs ASAP.cs asapci.cs

asapplay.exe: asapplay.cs ASAP.cs asapci.cs
	$(CSC) asapplay.cs ASAP.cs asapci.cs

SilverASAP.xap: SilverASAP.dll AppManifest.xaml
	$(SEVENZIP) -tzip $@ SilverASAP.dll AppManifest.xaml

SilverASAP.dll: SilverASAP.cs ASAP.cs asapci.cs
	$(CSC) -t:library SilverASAP.cs ASAP.cs asapci.cs -nostdlib -noconfig -r:$(SL)/mscorlib.dll -r:$(SL)/system.dll -r:$(SL)/System.Net.dll -r:$(SL)/System.Windows.dll -r:$(SL)/System.Windows.Browser.dll

ASAP.cs: ASAP.ppcs ../asap.c ../acpu.c ../apokeysnd.c ../asap.h ../asap_internal.h ../anylang.h ../players.h
	$(PP) ASAP.ppcs > $@

doc: doc/ASAP.chm

doc/ASAP.chm: doc/ASAP.dll
	$(NDOC) -documenter:MSDN -CleanIntermediates=true -DocumentInheritedFrameworkMembers=false \
		-OutputTarget=HtmlHelp -HtmlHelpName=ASAP -Title="ASAP .NET API" doc/ASAP.dll

doc/ASAP.dll: ASAP.cs asapci.cs
	-mkdir doc
	$(CSC) -t:library -doc:doc/ASAP.xml ASAP.cs asapci.cs

clean:
	rm -rf asap2wav.exe asapplay.exe SilverASAP.xap SilverASAP.dll ASAP.cs doc

.DELETE_ON_ERROR:
