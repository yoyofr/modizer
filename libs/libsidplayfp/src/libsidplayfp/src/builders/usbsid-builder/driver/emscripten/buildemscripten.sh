#!/usr/bin/env bash

# pkg-config amounts to => -I/usr/local/include/libusb-1.0 -L/usr/local/lib -lusb-1.0

mkdir -p build_emscripten

rm build_emscripten/*.o
rm build_emscripten/*.a
rm build_emscripten/*.bc
rm build_emscripten/*.so
#  --bind -s MODULARIZE=1 -s WASM=1 -s ENVIRONMENT=node -s BINARYEN_ASYNC_COMPILATION=0 \
em++ \
  -r \
  -static \
  -flto \
  -emit-llvm \
  --target=wasm32-unknown-emscripten \
  --verbose \
  -lembind \
  --bind \
  -s WASM_OBJECT_FILES=0 \
  USBSID.cpp \
  USBSIDInterface.cpp \
  -Wl,-rpath \
  -Wl,/home/loud/Development/c64/sidplaytrack/bitbucket.websid/lib/libusb/build/lib \
  -fPIC \
  -I/home/loud/Development/c64/sidplaytrack/bitbucket.websid/lib/libusb_em/include/libusb-1.0 \
  -L/home/loud/Development/c64/sidplaytrack/bitbucket.websid/lib/libusb_em/lib \
  -lusb-1.0 \
  --closure 0 \
  -c
  # -o build_emscripten/USBSID.bc

# em++ \
#   -r \
#   -static \
#   -flto \
#   -emit-llvm \
#   --target=wasm32-unknown-emscripten \
#   --verbose \
#   -s WASM=0 \
#   --bind \
#   -s WASM_OBJECT_FILES=0 \
#   build_emscripten/USBSID.bc \
#   -Wl,-rpath \
#   -Wl,/home/loud/Development/c64/sidplaytrack/bitbucket.websid/lib/libusb/build/lib \
#   -fPIC \
#   -I/home/loud/Development/c64/sidplaytrack/bitbucket.websid/lib/libusb_em/include/libusb-1.0 \
#   -L/home/loud/Development/c64/sidplaytrack/bitbucket.websid/lib/libusb_em/lib \
#   -lusb-1.0 \
#   -o build_emscripten/USBSID.o

#em++ --save-bc build_emscripten/ -r -emit-llvm --target=wasm32-unknown-emscripten -S --verbose USBSID.cpp -c -fPIC -I/home/loud/Development/c64/sidplaytrack/bitbucket.websid/lib/libusb/build/include/libusb-1.0 -L/home/loud/Development/c64/sidplaytrack/bitbucket.websid/lib/libusb/build/lib -lusb-1.0 -o build_emscripten/USBSID.bc
#em++ --save-bc build_emscripten/ -r -emit-llvm --target=wasm32-unknown-emscripten -S --verbose USBSIDInterface.cpp -c -fPIC -I/home/loud/Development/c64/sidplaytrack/bitbucket.websid/lib/libusb/build/include/libusb-1.0 -L/home/loud/Development/c64/sidplaytrack/bitbucket.websid/lib/libusb/build/lib -lusb-1.0  -o build_emscripten/USBSIDInterface.bc
#emar rcs build_emscripten/USBSIDPico.a build_emscripten/*.o
#em++ --verbose -o build_emscripten/USBSIDPico.so build_emscripten/*.o -L/home/loud/Development/c64/sidplaytrack/bitbucket.websid/lib/libusb/build/lib -lusb-1.0 --bind -s ASYNCIFY -s ALLOW_MEMORY_GROWTH
