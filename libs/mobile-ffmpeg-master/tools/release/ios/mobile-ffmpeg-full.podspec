Pod::Spec.new do |s|  
    s.name              = "mobile-ffmpeg-full"
    s.version           = "VERSION"
    s.summary           = "Mobile FFmpeg Full Static Framework"
    s.description       = <<-DESC
    Includes FFmpeg v4.4-dev-416 with fontconfig v2.13.92, freetype v2.10.2, fribidi v1.0.9, gmp v6.2.0, gnutls v3.6.13, kvazaar v2.0.0, lame v3.100, libaom v1.0.0-errata1-avif-110, libass v0.14.0, libilbc v2.0.2, libtheora v1.1.1, libvorbis v1.3.7, libvpx v1.8.2, libwebp v1.1.0, libxml2 v2.9.10, opencore-amr v0.1.5, opus v1.3.1, shine v3.1.1, snappy v1.1.8, soxr v0.1.3, speex v1.2.0, twolame v0.4, vo-amrwbenc v0.1.3 and wavpack v5.3.0 libraries enabled.
    DESC

    s.homepage          = "https://github.com/tanersener/mobile-ffmpeg"

    s.author            = { "Taner Sener" => "tanersener@gmail.com" }
    s.license           = { :type => "LGPL-3.0", :file => "mobileffmpeg.framework/LICENSE" }

    s.platform          = :ios
    s.requires_arc      = true
    s.libraries         = 'z', 'bz2', 'c++', 'iconv'

    s.source            = { :http => "https://github.com/tanersener/mobile-ffmpeg/releases/download/vVERSION/mobile-ffmpeg-full-VERSION-ios-framework.zip" }

    s.ios.deployment_target = '9.3'
    s.ios.frameworks    = 'AudioToolbox','AVFoundation','CoreMedia','VideoToolbox'
    s.ios.vendored_frameworks = 'mobileffmpeg.framework', 'libavcodec.framework', 'libavdevice.framework', 'libavfilter.framework', 'libavformat.framework', 'libavutil.framework', 'libswresample.framework', 'libswscale.framework', 'expat.framework', 'fontconfig.framework', 'freetype.framework', 'fribidi.framework', 'giflib.framework', 'gmp.framework', 'gnutls.framework', 'jpeg.framework', 'kvazaar.framework', 'lame.framework', 'libaom.framework', 'libass.framework', 'libhogweed.framework', 'libilbc.framework', 'libnettle.framework', 'libogg.framework', 'libopencore-amrnb.framework', 'libpng.framework', 'libsndfile.framework', 'libtheora.framework', 'libtheoradec.framework', 'libtheoraenc.framework', 'libvorbis.framework', 'libvorbisenc.framework', 'libvorbisfile.framework', 'libvpx.framework', 'libwebp.framework', 'libwebpmux.framework', 'libwebpdemux.framework', 'libxml2.framework', 'opus.framework', 'shine.framework', 'snappy.framework', 'soxr.framework', 'speex.framework', 'tiff.framework', 'twolame.framework', 'vo-amrwbenc.framework', 'wavpack.framework'

end  
