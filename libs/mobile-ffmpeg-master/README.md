# MobileFFmpeg [![Financial Contributors on Open Collective](https://opencollective.com/mobile-ffmpeg/all/badge.svg?label=financial+contributors)](https://opencollective.com/mobile-ffmpeg) ![GitHub release](https://img.shields.io/badge/release-v4.4-blue.svg) ![Maven Central](https://img.shields.io/maven-central/v/com.arthenica/mobile-ffmpeg-min) ![CocoaPods](https://img.shields.io/badge/pod-v4.4-blue.svg) [![Build Status](https://travis-ci.org/tanersener/mobile-ffmpeg.svg?branch=master)](https://travis-ci.org/tanersener/mobile-ffmpeg)

FFmpeg for Android, iOS and tvOS

<img src="https://github.com/tanersener/mobile-ffmpeg/blob/master/docs/assets/mobile-ffmpeg-logo-v7.png" width="320">

### 1. Features
- Includes both `FFmpeg` and `FFprobe`
- Use binaries available at `Github`/`Maven Central`/`CocoaPods` or build your own version with external libraries you need
- Supports
    - Android, iOS and tvOS
    - FFmpeg `v3.4.x`, `v4.0.x`, `v4.1`, `v4.2` , `v4.3` and `v4.4-dev` releases
    - 29 external libraries
    
        `chromaprint`, `fontconfig`, `freetype`, `fribidi`, `gmp`, `gnutls`, `kvazaar`, `lame`, `libaom`, `libass`, `libiconv`, `libilbc`, `libtheora`, `libvorbis`, `libvpx`, `libwebp`, `libxml2`, `opencore-amr`, `openh264`, `opus`, `sdl`, `shine`, `snappy`, `soxr`, `speex`, `tesseract`, `twolame`, `vo-amrwbenc`, `wavpack`
    
    - 5 external libraries with GPL license
    
        `rubberband`, `vid.stab`, `x264`, `x265`, `xvidcore`

    - Concurrent execution

- Exposes both FFmpeg library and MobileFFmpeg wrapper library capabilities
- Includes cross-compile instructions for 47 open-source libraries
    
    `chromaprint`, `expat`, `ffmpeg`, `fontconfig`, `freetype`, `fribidi`, `giflib`, `gmp`, `gnutls`, `kvazaar`, `lame`, `leptonica`, `libaom`, `libass`, `libiconv`, `libilbc`, `libjpeg`, `libjpeg-turbo`, `libogg`, `libpng`, `libsamplerate`, `libsndfile`, `libtheora`, `libuuid`, `libvorbis`, `libvpx`, `libwebp`, `libxml2`, `nettle`, `opencore-amr`, `openh264`, `opus`, `rubberband`, `sdl`, `shine`, `snappy`, `soxr`, `speex`, `tesseract`, `tiff`, `twolame`, `vid.stab`, `vo-amrwbenc`, `wavpack`, `x264`, `x265`, `xvidcore`

- Licensed under LGPL 3.0, can be customized to support GPL v3.0

#### 1.1 Android
- Builds `arm-v7a`, `arm-v7a-neon`, `arm64-v8a`, `x86` and `x86_64` architectures
- Supports `zlib` and `MediaCodec` system libraries
- Camera access on [supported devices](https://developer.android.com/ndk/guides/stable_apis#camera)
- Builds shared native libraries (.so)
- Creates Android archive with .aar extension
- Supports `API Level 16+`

#### 1.2 iOS
- Builds `armv7`, `armv7s`, `arm64`, `arm64e`, `i386`, `x86_64` and `x86_64` (Mac Catalyst) architectures
- Supports `bzip2`, `iconv`, `libuuid`, `zlib` system libraries and `AudioToolbox`, `VideoToolbox`, `AVFoundation` system frameworks
- Objective-C API
- Camera access
- `ARC` enabled library
- Built with `-fembed-bitcode` flag
- Creates static frameworks, static xcframeworks and static universal (fat) libraries (.a)
- Supports `iOS SDK 9.3` or later
 
#### 1.3 tvOS
- Builds `arm64` and `x86_64` architectures
- Supports `bzip2`, `iconv`, `libuuid`, `zlib` system libraries and `AudioToolbox`, `VideoToolbox` system frameworks
- Objective-C API
- `ARC` enabled library
- Built with `-fembed-bitcode` flag
- Creates static frameworks and static universal (fat) libraries (.a)
- Supports `tvOS SDK 9.2` or later

### 2. Using

Prebuilt binaries are available at [Github](https://github.com/tanersener/mobile-ffmpeg/releases), [Maven Central](https://repo1.maven.org/maven2) and [CocoaPods](https://cocoapods.org).

#### 2.1 Packages

There are eight different `mobile-ffmpeg` packages. Below you can see which system libraries and external libraries are enabled in each of them. 

Please remember that some parts of `FFmpeg` are licensed under the `GPL` and only `GPL` licensed `mobile-ffmpeg` packages include them.

<table>
<thead>
<tr>
<th align="center"></th>
<th align="center"><sup>min</sup></th>
<th align="center"><sup>min-gpl</sup></th>
<th align="center"><sup>https</sup></th>
<th align="center"><sup>https-gpl</sup></th>
<th align="center"><sup>audio</sup></th>
<th align="center"><sup>video</sup></th>
<th align="center"><sup>full</sup></th>
<th align="center"><sup>full-gpl</sup></th>
</tr>
</thead>
<tbody>
<tr>
<td align="center"><sup>external libraries</sup></td>
<td align="center">-</td>
<td align="center"><sup>vid.stab</sup><br><sup>x264</sup><br><sup>x265</sup><br><sup>xvidcore</sup></td>
<td align="center"><sup>gmp</sup><br><sup>gnutls</sup></td>
<td align="center"><sup>gmp</sup><br><sup>gnutls</sup><br><sup>vid.stab</sup><br><sup>x264</sup><br><sup>x265</sup><br><sup>xvidcore</sup></td>
<td align="center"><sup>lame</sup><br><sup>libilbc</sup><br><sup>libvorbis</sup><br><sup>opencore-amr</sup><br><sup>opus</sup><br><sup>shine</sup><br><sup>soxr</sup><br><sup>speex</sup><br><sup>twolame</sup><br><sup>vo-amrwbenc</sup><br><sup>wavpack</sup></td>
<td align="center"><sup>fontconfig</sup><br><sup>freetype</sup><br><sup>fribidi</sup><br><sup>kvazaar</sup><br><sup>libaom</sup><br><sup>libass</sup><br><sup>libiconv</sup><br><sup>libtheora</sup><br><sup>libvpx</sup><br><sup>libwebp</sup><br><sup>snappy</sup></td>
<td align="center"><sup>fontconfig</sup><br><sup>freetype</sup><br><sup>fribidi</sup><br><sup>gmp</sup><br><sup>gnutls</sup><br><sup>kvazaar</sup><br><sup>lame</sup><br><sup>libaom</sup><br><sup>libass</sup><br><sup>libiconv</sup><br><sup>libilbc</sup><br><sup>libtheora</sup><br><sup>libvorbis</sup><br><sup>libvpx</sup><br><sup>libwebp</sup><br><sup>libxml2</sup><br><sup>opencore-amr</sup><br><sup>opus</sup><br><sup>shine</sup><br><sup>snappy</sup><br><sup>soxr</sup><br><sup>speex</sup><br><sup>twolame</sup><br><sup>vo-amrwbenc</sup><br><sup>wavpack</sup></td>
<td align="center"><sup>fontconfig</sup><br><sup>freetype</sup><br><sup>fribidi</sup><br><sup>gmp</sup><br><sup>gnutls</sup><br><sup>kvazaar</sup><br><sup>lame</sup><br><sup>libaom</sup><br><sup>libass</sup><br><sup>libiconv</sup><br><sup>libilbc</sup><br><sup>libtheora</sup><br><sup>libvorbis</sup><br><sup>libvpx</sup><br><sup>libwebp</sup><br><sup>libxml2</sup><br><sup>opencore-amr</sup><br><sup>opus</sup><br><sup>shine</sup><br><sup>snappy</sup><br><sup>soxr</sup><br><sup>speex</sup><br><sup>twolame</sup><br><sup>vid.stab</sup><br><sup>vo-amrwbenc</sup><br><sup>wavpack</sup><br><sup>x264</sup><br><sup>x265</sup><br><sup>xvidcore</sup></td>
</tr>
<tr>
<td align="center"><sup>android system libraries</sup></td>
<td align="center" colspan=8><sup>zlib</sup><br><sup>MediaCodec</sup></td>
</tr>
<tr>
<td align="center"><sup>ios system libraries</sup></td>
<td align="center" colspan=8><sup>zlib</sup><br><sup>AudioToolbox</sup><br><sup>AVFoundation</sup><br><sup>iconv</sup><br><sup>VideoToolbox</sup><br><sup>bzip2</sup></td>
</tr>
<tr>
<td align="center"><sup>tvos system libraries</sup></td>
<td align="center" colspan=8><sup>zlib</sup><br><sup>AudioToolbox</sup><br><sup>iconv</sup><br><sup>VideoToolbox</sup><br><sup>bzip2</sup></td>
</tr>
</tbody>
</table>

 - `libilbc`, `opus`, `snappy`, `x264` and `xvidcore` are supported since `v1.1`

 - `libaom` and `soxr` are supported since `v2.0`

 - `chromaprint`, `vid.stab` and `x265` are supported since `v2.1`

 - `sdl`, `tesseract`, `twolame` external libraries; `zlib`, `MediaCodec` Android system libraries; `bzip2`, `zlib` iOS system libraries and `AudioToolbox`, `VideoToolbox`, `AVFoundation` iOS system frameworks are supported since `v3.0`

 - Since `v4.2`, `chromaprint`, `sdl` and `tesseract` libraries are not included in binary releases. You can still build them and include in your releases
 
 - `AVFoundation` is not available on `tvOS`, `VideoToolbox` is not available on `tvOS` LTS releases
 
 - Since `v4.3.1`, `iOS` and `tvOS` releases started to use `iconv` system library instead of `iconv` external library 

 - `vo-amrwbenc` is supported since `v4.4`

#### 2.2 Android
  - For versions `4.4` and `4.4.LTS`, add `mavenCentral()` to your `build.gradle` and make sure that it is listed
    before `jcenter()`
  - For `4.3.2` and older releases, add `jcenter()`
    ```
    repositories {
        mavenCentral()
    }
    ```

1. Add MobileFFmpeg dependency to your `build.gradle` in `mobile-ffmpeg-<package name>` pattern.
    ```
    dependencies {
        implementation 'com.arthenica:mobile-ffmpeg-full:4.4'
    }
    ```

2. Execute synchronous FFmpeg commands.
    ```
    import com.arthenica.mobileffmpeg.Config;
    import com.arthenica.mobileffmpeg.FFmpeg;

    int rc = FFmpeg.execute("-i file1.mp4 -c:v mpeg4 file2.mp4");
   
    if (rc == RETURN_CODE_SUCCESS) {
        Log.i(Config.TAG, "Command execution completed successfully.");
    } else if (rc == RETURN_CODE_CANCEL) {
        Log.i(Config.TAG, "Command execution cancelled by user.");
    } else {
        Log.i(Config.TAG, String.format("Command execution failed with rc=%d and the output below.", rc));
        Config.printLastCommandOutput(Log.INFO);
    }
    ```

3. Execute asynchronous FFmpeg commands.
    ```
    import com.arthenica.mobileffmpeg.Config;
    import com.arthenica.mobileffmpeg.FFmpeg;

    long executionId = FFmpeg.executeAsync("-i file1.mp4 -c:v mpeg4 file2.mp4", new ExecuteCallback() {

        @Override
        public void apply(final long executionId, final int returnCode) {
            if (returnCode == RETURN_CODE_SUCCESS) {
                Log.i(Config.TAG, "Async command execution completed successfully.");
            } else if (returnCode == RETURN_CODE_CANCEL) {
                Log.i(Config.TAG, "Async command execution cancelled by user.");
            } else {
                Log.i(Config.TAG, String.format("Async command execution failed with returnCode=%d.", returnCode));
            }
        }
    });
    ```

4. Execute FFprobe commands.
    ```
    import com.arthenica.mobileffmpeg.Config;
    import com.arthenica.mobileffmpeg.FFprobe;

    int rc = FFprobe.execute("-i file1.mp4");
   
    if (rc == RETURN_CODE_SUCCESS) {
        Log.i(Config.TAG, "Command execution completed successfully.");
    } else {
        Log.i(Config.TAG, String.format("Command execution failed with rc=%d and the output below.", rc));
        Config.printLastCommandOutput(Log.INFO);
    }
    ```

5. Check execution output later.
    ```
    int rc = Config.getLastReturnCode();
 
    if (rc == RETURN_CODE_SUCCESS) {
        Log.i(Config.TAG, "Command execution completed successfully.");
    } else if (rc == RETURN_CODE_CANCEL) {
        Log.i(Config.TAG, "Command execution cancelled by user.");
    } else {
        Log.i(Config.TAG, String.format("Command execution failed with rc=%d and the output below.", rc));
        Config.printLastCommandOutput(Log.INFO);
    }
    ```

6. Stop ongoing FFmpeg operations.
    - Stop all executions
        ```
        FFmpeg.cancel();
        ```
    - Stop a specific execution
        ```
        FFmpeg.cancel(executionId);
        ```

7. Get media information for a file.
    ```
    MediaInformation info = FFprobe.getMediaInformation("<file path or uri>");
    ```

8. Record video using Android camera.
    ```
    FFmpeg.execute("-f android_camera -i 0:0 -r 30 -pixel_format bgr0 -t 00:00:05 <record file path>");
    ```

9. Enable log callback.
    ```
    Config.enableLogCallback(new LogCallback() {
        public void apply(LogMessage message) {
            Log.d(Config.TAG, message.getText());
        }
    });
    ```

10. Enable statistics callback.
    ```
    Config.enableStatisticsCallback(new StatisticsCallback() {
        public void apply(Statistics newStatistics) {
            Log.d(Config.TAG, String.format("frame: %d, time: %d", newStatistics.getVideoFrameNumber(), newStatistics.getTime()));
        }
    });
    ```
11. Ignore the handling of a signal.
    ```
    Config.ignoreSignal(Signal.SIGXCPU);
    ```

12. List ongoing executions.
    ```
    final List<FFmpegExecution> ffmpegExecutions = FFmpeg.listExecutions();
    for (int i = 0; i < ffmpegExecutions.size(); i++) {
        FFmpegExecution execution = ffmpegExecutions.get(i);
        Log.d(TAG, String.format("Execution %d = id:%d, startTime:%s, command:%s.", i, execution.getExecutionId(), execution.getStartTime(), execution.getCommand()));
    }
    ```

13. Set default log level.
    ```
    Config.setLogLevel(Level.AV_LOG_FATAL);
    ```

14. Register custom fonts directory.
    ```
    Config.setFontDirectory(this, "<folder with fonts>", Collections.EMPTY_MAP);
    ```

#### 2.3 iOS / tvOS
1. Add MobileFFmpeg dependency to your `Podfile` in `mobile-ffmpeg-<package name>` pattern.

    - iOS
    ```
    pod 'mobile-ffmpeg-full', '~> 4.4'
    ```

    - tvOS
    ```
    pod 'mobile-ffmpeg-tvos-full', '~> 4.4'
    ```

2. Execute synchronous FFmpeg commands.
    ```
    #import <mobileffmpeg/MobileFFmpegConfig.h>
    #import <mobileffmpeg/MobileFFmpeg.h>

    int rc = [MobileFFmpeg execute: @"-i file1.mp4 -c:v mpeg4 file2.mp4"];
   
    if (rc == RETURN_CODE_SUCCESS) {
        NSLog(@"Command execution completed successfully.\n");
    } else if (rc == RETURN_CODE_CANCEL) {
        NSLog(@"Command execution cancelled by user.\n");
    } else {
        NSLog(@"Command execution failed with rc=%d and output=%@.\n", rc, [MobileFFmpegConfig getLastCommandOutput]);
    }
    ```

3. Execute asynchronous FFmpeg commands.
    ```
    #import <mobileffmpeg/MobileFFmpegConfig.h>
    #import <mobileffmpeg/MobileFFmpeg.h>

    long executionId = [MobileFFmpeg executeAsync:@"-i file1.mp4 -c:v mpeg4 file2.mp4" withCallback:self];

    - (void)executeCallback:(long)executionId :(int)returnCode {
        if (rc == RETURN_CODE_SUCCESS) {
            NSLog(@"Async command execution completed successfully.\n");
        } else if (rc == RETURN_CODE_CANCEL) {
            NSLog(@"Async command execution cancelled by user.\n");
        } else {
            NSLog(@"Async command execution failed with rc=%d.\n", rc);
        }
    }
    ```

4. Execute FFprobe commands.
    ```
    #import <mobileffmpeg/MobileFFmpegConfig.h>
    #import <mobileffmpeg/MobileFFprobe.h>

    int rc = [MobileFFprobe execute: @"-i file1.mp4"];
   
    if (rc == RETURN_CODE_SUCCESS) {
        NSLog(@"Command execution completed successfully.\n");
    } else if (rc == RETURN_CODE_CANCEL) {
        NSLog(@"Command execution cancelled by user.\n");
    } else {
        NSLog(@"Command execution failed with rc=%d and output=%@.\n", rc, [MobileFFmpegConfig getLastCommandOutput]);
    }
    ```
    
5. Check execution output later.
    ```
    int rc = [MobileFFmpegConfig getLastReturnCode];
    NSString *output = [MobileFFmpegConfig getLastCommandOutput];

    if (rc == RETURN_CODE_SUCCESS) {
        NSLog(@"Command execution completed successfully.\n");
    } else if (rc == RETURN_CODE_CANCEL) {
        NSLog(@"Command execution cancelled by user.\n");
    } else {
        NSLog(@"Command execution failed with rc=%d and output=%@.\n", rc, output);
    }
    ```

6. Stop ongoing FFmpeg operations.
    - Stop all executions
        ```
        [MobileFFmpeg cancel];

        ```
    - Stop a specific execution
        ```
        [MobileFFmpeg cancel:executionId];
        ```

7. Get media information for a file.
    ```
    MediaInformation *mediaInformation = [MobileFFprobe getMediaInformation:@"<file path or uri>"];
    ```

8. Record video and audio using iOS camera. This operation is not supported on `tvOS` since `AVFoundation` is not available on `tvOS`.

    ```
    [MobileFFmpeg execute: @"-f avfoundation -r 30 -video_size 1280x720 -pixel_format bgr0 -i 0:0 -vcodec h264_videotoolbox -vsync 2 -f h264 -t 00:00:05 %@", recordFilePath];
    ```

9. Enable log callback.
    ```
    [MobileFFmpegConfig setLogDelegate:self];

    - (void)logCallback:(long)executionId :(int)level :(NSString*)message {
        dispatch_async(dispatch_get_main_queue(), ^{
            NSLog(@"%@", message);
        });
    }
    ```

10. Enable statistics callback.
    ```
    [MobileFFmpegConfig setStatisticsDelegate:self];

    - (void)statisticsCallback:(Statistics *)newStatistics {
        dispatch_async(dispatch_get_main_queue(), ^{
            NSLog(@"frame: %d, time: %d\n", newStatistics.getVideoFrameNumber, newStatistics.getTime);
        });
    }
    ```

11. Ignore the handling of a signal.
    ```
    [MobileFFmpegConfig ignoreSignal:SIGXCPU];
    ```

12. List ongoing executions.
    ```
    NSArray* ffmpegExecutions = [MobileFFmpeg listExecutions];
    for (int i = 0; i < [ffmpegExecutions count]; i++) {
        FFmpegExecution* execution = [ffmpegExecutions objectAtIndex:i];
        NSLog(@"Execution %d = id: %ld, startTime: %@, command: %@.\n", i, [execution getExecutionId], [execution getStartTime], [execution getCommand]);
    }
    ```

13. Set default log level.
    ```
    [MobileFFmpegConfig setLogLevel:AV_LOG_FATAL];
    ```

14. Register custom fonts directory.
    ```
    [MobileFFmpegConfig setFontDirectory:@"<folder with fonts>" with:nil];
    ```

#### 2.4 Manual Installation
##### 2.4.1 Android

You can import `MobileFFmpeg` aar packages in `Android Studio` using the `File` -> `New` -> `New Module` -> `Import .JAR/.AAR Package` menu.

##### 2.4.2 iOS / tvOS

iOS and tvOS frameworks can be installed manually using the [Importing Frameworks](https://github.com/tanersener/mobile-ffmpeg/wiki/Importing-Frameworks) guide. 
If you want to use universal binaries please refer to [Using Universal Binaries](https://github.com/tanersener/mobile-ffmpeg/wiki/Using-Universal-Binaries) guide.   
    
#### 2.5 Test Application
You can see how MobileFFmpeg is used inside an application by running test applications provided.
There is an `Android` test application under the `android/test-app` folder, an `iOS` test application under the 
`ios/test-app` folder and a `tvOS` test application under the `tvos/test-app` folder. 

All applications are identical and supports command execution, video encoding, accessing https, encoding audio, 
burning subtitles, video stabilisation, pipe operations and concurrent command execution.

<img src="https://github.com/tanersener/mobile-ffmpeg/blob/master/docs/assets/android_test_app.gif" width="240">

### 3. Versions

`MobileFFmpeg` version number is aligned with `FFmpeg` since version `4.2`. 

In previous versions, `MobileFFmpeg` version of a release and `FFmpeg` version included in that release was different. 
The following table lists `FFmpeg` versions used in `MobileFFmpeg` releases.
  
- `dev` part in `FFmpeg` version number indicates that `FFmpeg` source is pulled from the `FFmpeg` `master` branch. 
Exact version number is obtained using `git describe --tags`. 

|  MobileFFmpeg Version | FFmpeg Version | Release Date |
| :----: | :----: |:----: |
| [4.4](https://github.com/tanersener/mobile-ffmpeg/releases/tag/v4.4) | 4.4-dev-416 | Jul 25, 2020 |
| [4.4.LTS](https://github.com/tanersener/mobile-ffmpeg/releases/tag/v4.4.LTS) | 4.4-dev-416 | Jul 24, 2020 |
| [4.3.2](https://github.com/tanersener/mobile-ffmpeg/releases/tag/v4.3.2) | 4.3-dev-2955 | Apr 15, 2020 |
| [4.3.1](https://github.com/tanersener/mobile-ffmpeg/releases/tag/v4.3.1) | 4.3-dev-1944 | Jan 25, 2020 |
| [4.3.1.LTS](https://github.com/tanersener/mobile-ffmpeg/releases/tag/v4.3.1.LTS) | 4.3-dev-1944 | Jan 25, 2020 |
| [4.3](https://github.com/tanersener/mobile-ffmpeg/releases/tag/v4.3) | 4.3-dev-1181 | Oct 27, 2019 |
| [4.2.2](https://github.com/tanersener/mobile-ffmpeg/releases/tag/v4.2.2) | 4.2-dev-1824 | July 3, 2019 |
| [4.2.2.LTS](https://github.com/tanersener/mobile-ffmpeg/releases/tag/v4.2.2.LTS) | 4.2-dev-1824 | July 3, 2019 |
| [4.2.1](https://github.com/tanersener/mobile-ffmpeg/releases/tag/v4.2.1) | 4.2-dev-1156 | Apr 2, 2019 |
| [4.2](https://github.com/tanersener/mobile-ffmpeg/releases/tag/v4.2) | 4.2-dev-480 | Jan 3, 2019 |
| [4.2.LTS](https://github.com/tanersener/mobile-ffmpeg/releases/tag/v4.2.LTS) | 4.2-dev-480 | Jan 3, 2019 |
| [3.1](https://github.com/tanersener/mobile-ffmpeg/releases/tag/v3.1) | 4.1-10 | Dec 11, 2018 |
| [3.0](https://github.com/tanersener/mobile-ffmpeg/releases/tag/v3.0) | 4.1-dev-1517 | Oct 25, 2018 |
| [2.2](https://github.com/tanersener/mobile-ffmpeg/releases/tag/v2.2) | 4.0.3 | Nov 10, 2018 |
| [2.1.1](https://github.com/tanersener/mobile-ffmpeg/releases/tag/v2.1.1) | 4.0.2 | Sep 19, 2018 |
| [2.1](https://github.com/tanersener/mobile-ffmpeg/releases/tag/v2.1) | 4.0.2 | Sep 5, 2018 |
| [2.0](https://github.com/tanersener/mobile-ffmpeg/releases/tag/v2.0) | 4.0.1 | Jun 30, 2018 |
| [1.2](https://github.com/tanersener/mobile-ffmpeg/releases/tag/v1.2) | 3.4.4 | Aug 30, 2018|
| [1.1](https://github.com/tanersener/mobile-ffmpeg/releases/tag/v1.1) | 3.4.2 | Jun 18, 2018 |
| [1.0](https://github.com/tanersener/mobile-ffmpeg/releases/tag/v1.0) | 3.4.2 | Jun 6, 2018 |

### 4. LTS Releases

Starting from `v4.2`, `MobileFFmpeg` binaries are published in two different variants: `Main Release` and `LTS Release`. 

- Main releases include complete functionality of the library and support the latest SDK/API features.

- LTS releases are customized to support a wider range of devices. They are built using older API/SDK versions, so some features are not available on them.

This table shows the differences between two variants.

|        | Main Release | LTS Release |
| :----: | :----: | :----: |
| Android API Level | 24 | 16 | 
| Android Camera Access | Yes | - |
| Android Architectures | arm-v7a-neon<br/>arm64-v8a<br/>x86<br/>x86-64 | arm-v7a<br/>arm-v7a-neon<br/>arm64-v8a<br/>x86<br/>x86-64 |
| Xcode Support | 10.1 | 7.3.1 |
| iOS SDK | 12.1 | 9.3 |
| iOS AVFoundation | Yes | - |
| iOS Architectures | arm64<br/>arm64e<sup>1</sup><br/>x86-64<br/>x86-64-mac-catalyst<sup>2</sup> | armv7<br/>arm64<br/>i386<br/>x86-64 |
| tvOS SDK | 10.2 | 9.2 |
| tvOS Architectures | arm64<br/>x86-64 | arm64<br/>x86-64 |

<sup>1</sup> - Included until `v4.3.2`

<sup>2</sup> - Included since `v4.3.2`

### 5. Building

Build scripts from `master` and `development` branches are tested periodically. See the latest status from the table below.

| branch | status |
| :---: | :---: |
|  master | [![Build Status](https://travis-ci.org/tanersener/mobile-ffmpeg.svg?branch=master)](https://travis-ci.org/tanersener/mobile-ffmpeg) |
|  development | [![Build Status](https://travis-ci.org/tanersener/mobile-ffmpeg.svg?branch=development)](https://travis-ci.org/tanersener/mobile-ffmpeg) |


#### 5.1 Prerequisites
1. Use your package manager (apt, yum, dnf, brew, etc.) to install the following packages.

    ```
    autoconf automake libtool pkg-config curl cmake gcc gperf texinfo yasm nasm bison autogen patch git
    ```
Some of these packages are not mandatory for the default build.
Please visit [Android Prerequisites](https://github.com/tanersener/mobile-ffmpeg/wiki/Android-Prerequisites), 
[iOS Prerequisites](https://github.com/tanersener/mobile-ffmpeg/wiki/iOS-Prerequisites) and 
[tvOS Prerequisites](https://github.com/tanersener/mobile-ffmpeg/wiki/tvOS-Prerequisites) for the details.

2. Android builds require these additional packages.
    - **Android SDK 4.1 Jelly Bean (API Level 16)** or later
    - **Android NDK r21** or later with LLDB and CMake

3. iOS builds need these extra packages and tools.
    - **Xcode 7.3.1** or later
    - **iOS SDK 9.3** or later
    - **Command Line Tools**

4. tvOS builds need these extra packages and tools.
    - **Xcode 7.3.1** or later
    - **tvOS SDK 9.2** or later
    - **Command Line Tools**

#### 5.2 Build Scripts
Use `android.sh`, `ios.sh` and `tvos.sh` to build MobileFFmpeg for each platform. 

All three scripts support additional options and 
can be customized to enable/disable specific external libraries and/or architectures. Please refer to wiki pages of
[android.sh](https://github.com/tanersener/mobile-ffmpeg/wiki/android.sh), 
[ios.sh](https://github.com/tanersener/mobile-ffmpeg/wiki/ios.sh) and 
[tvos.sh](https://github.com/tanersener/mobile-ffmpeg/wiki/tvos.sh) to see all available build options.
##### 5.2.1 Android 
```
export ANDROID_HOME=<Android SDK Path>
export ANDROID_NDK_ROOT=<Android NDK Path>
./android.sh
```

<img src="https://github.com/tanersener/mobile-ffmpeg/blob/master/docs/assets/android_custom.gif" width="600">

##### 5.2.2 iOS
```
./ios.sh
```

<img src="https://github.com/tanersener/mobile-ffmpeg/blob/master/docs/assets/ios_custom.gif" width="600">

##### 5.2.3 tvOS
```
./tvos.sh
```

<img src="https://github.com/tanersener/mobile-ffmpeg/blob/master/docs/assets/tvos_custom.gif" width="600">


##### 5.2.4 Building LTS Binaries

Use `--lts` option to build lts binaries for each platform.

#### 5.3 Build Output

All libraries created by the top level build scripts (`android.sh`, `ios.sh` and `tvos.sh`) can be found under 
the `prebuilt` directory.

- `Android` archive (.aar file) is located under the `android-aar` folder
- `iOS` frameworks are located under the `ios-framework` folder
- `iOS` xcframeworks are located under the `ios-xcframework` folder
- `iOS` universal binaries are located under the `ios-universal` folder
- `tvOS` frameworks are located under the `tvos-framework` folder
- `tvOS` universal binaries are located under the `tvos-universal` folder

#### 5.4 GPL Support
It is possible to enable GPL licensed libraries `x264`, `xvidcore` since `v1.1`; `vid.stab`, `x265` since `v2.1` and
`rubberband` since `v4.3.2` from the top level build scripts. Their source code is not included in the repository and
downloaded when enabled.

#### 5.5 External Libraries
`build` directory includes build scripts of all external libraries. Two scripts exist for each external library, 
one for `Android` and one for `iOS / tvOS`. Each of these two scripts contains options/flags used to cross-compile the 
library on the specified mobile platform. 

CPU optimizations (`ASM`) are enabled for most of the external libraries. Details and exceptions can be found under the 
[ASM Support](https://github.com/tanersener/mobile-ffmpeg/wiki/ASM-Support) wiki page.

### 6. Documentation

A more detailed documentation is available at [Wiki](https://github.com/tanersener/mobile-ffmpeg/wiki).

### 7. Contributors

#### 7.1 Code Contributors

This project exists thanks to all the people who contribute. [[Contribute](CONTRIBUTING.md)].
<a href="https://github.com/tanersener/mobile-ffmpeg/graphs/contributors"><img src="https://opencollective.com/mobile-ffmpeg/contributors.svg?width=890&button=false" /></a>

#### 7.2 Financial Contributors

Become a financial contributor and help us sustain our community. [[Contribute](https://opencollective.com/mobile-ffmpeg/contribute)]

##### 7.2.1 Individuals

<a href="https://opencollective.com/mobile-ffmpeg"><img src="https://opencollective.com/mobile-ffmpeg/individuals.svg?width=890"></a>

##### 7.2.2 Organizations

Support this project with your organization. Your logo will show up here with a link to your website. [[Contribute](https://opencollective.com/mobile-ffmpeg/contribute)]

<a href="https://opencollective.com/mobile-ffmpeg/organization/0/website"><img src="https://opencollective.com/mobile-ffmpeg/organization/0/avatar.svg"></a>
<a href="https://opencollective.com/mobile-ffmpeg/organization/1/website"><img src="https://opencollective.com/mobile-ffmpeg/organization/1/avatar.svg"></a>
<a href="https://opencollective.com/mobile-ffmpeg/organization/2/website"><img src="https://opencollective.com/mobile-ffmpeg/organization/2/avatar.svg"></a>
<a href="https://opencollective.com/mobile-ffmpeg/organization/3/website"><img src="https://opencollective.com/mobile-ffmpeg/organization/3/avatar.svg"></a>
<a href="https://opencollective.com/mobile-ffmpeg/organization/4/website"><img src="https://opencollective.com/mobile-ffmpeg/organization/4/avatar.svg"></a>
<a href="https://opencollective.com/mobile-ffmpeg/organization/5/website"><img src="https://opencollective.com/mobile-ffmpeg/organization/5/avatar.svg"></a>
<a href="https://opencollective.com/mobile-ffmpeg/organization/6/website"><img src="https://opencollective.com/mobile-ffmpeg/organization/6/avatar.svg"></a>
<a href="https://opencollective.com/mobile-ffmpeg/organization/7/website"><img src="https://opencollective.com/mobile-ffmpeg/organization/7/avatar.svg"></a>
<a href="https://opencollective.com/mobile-ffmpeg/organization/8/website"><img src="https://opencollective.com/mobile-ffmpeg/organization/8/avatar.svg"></a>
<a href="https://opencollective.com/mobile-ffmpeg/organization/9/website"><img src="https://opencollective.com/mobile-ffmpeg/organization/9/avatar.svg"></a>

### 8. License

`MobileFFmpeg` is licensed under the LGPL v3.0. However, if source code is built using the optional `--enable-gpl` flag
or prebuilt binaries with `-gpl` postfix are used, then MobileFFmpeg is subject to the GPL v3.0 license.

The source code of all external libraries included is in compliance with their individual licenses.

`openh264` source code included in this repository is licensed under the 2-clause BSD License but this license does 
not cover the `MPEG LA` licensing fees. If you build `mobile-ffmpeg` with `openh264` and distribute that library, then 
you are subject to pay `MPEG LA` licensing fees. Refer to [OpenH264 FAQ](https://www.openh264.org/faq.html) page for 
the details. Please note that `mobile-ffmpeg` does not publish a binary with `openh264` inside.

`strip-frameworks.sh` script included and distributed (until v4.x) is published under the
[Apache License version 2.0](https://www.apache.org/licenses/LICENSE-2.0).

In test applications; embedded fonts are licensed under the
[SIL Open Font License](https://opensource.org/licenses/OFL-1.1), other digital assets are published in the public
domain.

Please visit [License](https://github.com/tanersener/mobile-ffmpeg/wiki/License) page for the details.

### 9. Patents

It is not clearly explained in their documentation but it is believed that `FFmpeg`, `kvazaar`, `x264` and `x265`
include algorithms which are subject to software patents. If you live in a country where software algorithms are
patentable then you'll probably need to pay royalty fees to patent holders. We are not lawyers though, so we recommend
that you seek legal advice first. See [FFmpeg Patent Mini-FAQ](https://ffmpeg.org/legal.html).

`openh264` clearly states that it uses patented algorithms. Therefore, if you build mobile-ffmpeg with openh264 and
distribute that library, then you are subject to pay MPEG LA licensing fees. Refer to
[OpenH264 FAQ](https://www.openh264.org/faq.html) page for the details.

### 10. Contributing

Feel free to submit issues or pull requests. 

Please note that `master` branch includes only the latest released source code. Changes planned for the next release 
are implemented under the `development` branch. Therefore, if you want to create a pull request, please open it against
the `development`.

### 11. See Also

- [libav gas-preprocessor](https://github.com/libav/gas-preprocessor/raw/master/gas-preprocessor.pl)
- [FFmpeg API Documentation](https://ffmpeg.org/doxygen/4.0/index.html)
- [FFmpeg Wiki](https://trac.ffmpeg.org/wiki/WikiStart)
- [FFmpeg External Library Licenses](https://www.ffmpeg.org/doxygen/4.0/md_LICENSE.html)
