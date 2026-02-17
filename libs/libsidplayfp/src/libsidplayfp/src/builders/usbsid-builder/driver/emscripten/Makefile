SYSROOT := $(shell em-config CACHE)/sysroot
BUILDROOT := $(shell pwd)
RUNROOT := $(BUILDROOT)/htdocs
CC:= emcc
CXX:= em++
RUN := emrun

comma=,
list-to-csv=$(subst $() $(),$(comma),$(strip $(addsuffix ",$(addprefix ",${1}))))

# Add custom sysroot to library & macro search paths.
export LDFLAGS += -L$(SYSROOT)/lib

# Common linking flags for all targets.
export LDFLAGS += -s DYNAMIC_EXECUTION=0 -s AUTO_JS_LIBRARIES=0 -s AUTO_NATIVE_LIBRARIES=0

# Common compilation & linking flags for all langs and targets.
OPTIMIZATION = -O0 # -Os
THREADPOOL = -sPTHREAD_POOL_SIZE_STRICT=2 -sPTHREAD_POOL_SIZE=10
# THREADPOOL = -sPTHREAD_POOL_SIZE=0 -sPTHREAD_POOL_SIZE_STRICT=2 -s PTHREADS_DEBUG=1 -s USE_PTHREADS=1
COMMON_FLAGS = $(OPTIMIZATION) -flto -fexceptions -pthread $(THREADPOOL) -fPIC -I$(SYSROOT)/include/libusb-1.0
export CFLAGS += $(COMMON_FLAGS)
export CXXFLAGS += $(COMMON_FLAGS)
export LDFLAGS += $(COMMON_FLAGS)
US_LIBUSB = -lusb-1.0
EM_FLAGS = --bind -lembind -s ASSERTIONS=1 -s ASYNCIFY -s ALLOW_MEMORY_GROWTH --closure 0 -emit-llvm --target=wasm32-unknown-emscripten -Wl,-rpath -Wl,$(SYSROOT)/lib
EM_EXPORTFLAGS = # -s EXPORTED_RUNTIME_METHODS="['cwrap']" -s EXPORTED_FUNCTIONS="['_create_USBSID','_passThrough']"
EMRUN_FLAGS = # --emrun

.PHONY: clean

all: deps/libusb/configure $(SYSROOT)/lib/libusb-1.0.la src/USBSID.o src/USBSIDAdapter.o USBSID usbsid.js

clean:
	rm -f src/*.o
	rm -rf build/
	mkdir -p build

## USBSID Module

src/USBSID.o:	CXXFLAGS+=$(US_EM_FLAGS) $(US_EM_EXPORTFLAGS)
src/USBSID.o:	LDFLAGS+=$(US_LIBUSB)
src/USBSIDAdapter.o: CXXFLAGS+=$(EM_FLAGS) $(EM_EXPORTFLAGS)
src/USBSIDAdapter.o: CXXFLAGS+=-s INVOKE_RUN=0
# src/main.o: CXXFLAGS+=-s INVOKE_RUN=0

USBSID:
	mv src/*.o build/
usbsid.js:
	$(CXX) $(CXXFLAGS) $(EM_FLAGS) $(EM_EXPORTFLAGS) $(EMRUN_FLAGS) build/USBSID.o build/USBSIDAdapter.o -o htdocs/usbsid.js $(LDFLAGS) -lusb-1.0
# bitcode:
# 	$(CXX) $(CXXFLAGS) -r --static -s WASM_OBJECT_FILES=0 $(EM_FLAGS) $(EM_EXPORTFLAGS) $(EMRUN_FLAGS) build/USBSID.o build/USBSIDAdapter.o -c -o build/bc.bc $(LDFLAGS) -lusb-1.0


## Generic rules for deps

$(SYSROOT):
	mkdir -p $(@D)

deps/%/configure.ac:
	git submodule update --init $(@D)

deps/%/configure: deps/%/configure.ac | $(SYSROOT)/lib/libltdl.la
	cd $(@D) && autoreconf -fiv

deps/%/Makefile: deps/%/configure
	cd $(@D) && emconfigure ./configure --prefix=$(SYSROOT) --disable-shared $(CONFIGURE_ARGS)

## libtool

# deps/libtool/configure:
# 	mkdir -p deps/libtool
# 	curl -L https://ftpmirror.gnu.org/libtool/libtool-2.4.6.tar.gz | tar zx --strip 1 -C deps/libtool

# $(SYSROOT)/lib/libltdl.la: deps/libtool/Makefile | $(SYSROOT)
# 	$(MAKE) -C deps/libtool install

## libusb

deps/libusb/configure:
	mkdir -p deps/libusb
	curl -L https://github.com/libusb/libusb/releases/download/v1.0.27/libusb-1.0.27.tar.bz2 | tar jx --strip 1 -C deps/libusb

deps/libusb/Makefile: CONFIGURE_ARGS = --host=wasm32-emscripten

$(SYSROOT)/lib/libusb-1.0.la: deps/libusb/Makefile
	emmake make -C deps/libusb install

deps/libusb/examples/Makefile:
	emmake make LDFLAGS+='--bind -s ASYNCIFY -s ALLOW_MEMORY_GROWTH' CFLAGS+='--emrun -s EXPORTED_RUNTIME_METHODS="['callMain']"' -C deps/libusb/examples

run:
	$(RUN) --browser chrome $(RUNROOT)/index.html

runexample:
	$(RUN) --browser chrome $(BUILDROOT)/example/index.html

runpython:
	python3 -m http.server 8000 --bind 127.0.0.1 -d $(RUNROOT)/
