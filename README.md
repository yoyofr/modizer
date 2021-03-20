Modizer
=======

iOS modules, chiptunes & vgm player.

## Building

Requires: Xcode 12 or higher

### Build FFMPEG-iOS

```
# Assuming you are in project repo root dir:
cd libs/FFmpeg-iOS-build-script-master

# This will build FFmpeg for all architectures (arm64, armv7, x86_64, i386)
# May be a bit overkill and might waste some space but will get you up
# and running
./build-ffmpeg.sh

# Move the built FFmpeg libraries to where Xcode wants it
mv FFmpeg-iOS ../.

# Make sym links since the libs are universal
cd ../FFmpeg-iOS
ln -s lib libs-iOS
ln -s lib libs-iOSsimulator
```

### Change the Development Team

- Click on the modizer project in the left Navigator pane, and click "Signing & Capabilities"
- Change the Team to your developer profile.

Build and Run!

yoyofr
