Pod::Spec.new do |s|  
    s.name              = "mobile-ffmpeg-audio"
    s.version           = "VERSION"
    s.summary           = "Mobile FFmpeg Audio Static Framework"
    s.description       = <<-DESC
    Includes FFmpeg v4.4-dev-416 with lame v3.100, libilbc v2.0.2, libvorbis v1.3.7, opencore-amr v0.1.5, opus v1.3.1, shine v3.1.1, soxr v0.1.3, speex v1.2.0, twolame v0.4, vo-amrwbenc v0.1.3 and wavpack v5.3.0 libraries enabled.
    DESC

    s.homepage          = "https://github.com/tanersener/mobile-ffmpeg"

    s.author            = { "Taner Sener" => "tanersener@gmail.com" }
    s.license           = { :type => "LGPL-3.0", :file => "mobileffmpeg.framework/LICENSE" }

    s.platform          = :ios
    s.requires_arc      = true
    s.libraries         = 'z', 'bz2', 'c++', 'iconv'

    s.source            = { :http => "https://github.com/tanersener/mobile-ffmpeg/releases/download/vVERSION/mobile-ffmpeg-audio-VERSION-ios-framework.zip" }

    s.ios.deployment_target = '9.3'
    s.ios.frameworks    = 'AudioToolbox','AVFoundation','CoreMedia','VideoToolbox'
    s.ios.vendored_frameworks = 'mobileffmpeg.framework', 'libavcodec.framework', 'libavdevice.framework', 'libavfilter.framework', 'libavformat.framework', 'libavutil.framework', 'libswresample.framework', 'libswscale.framework', 'lame.framework', 'libilbc.framework', 'libogg.framework', 'libopencore-amrnb.framework', 'libsndfile.framework', 'libvorbis.framework', 'libvorbisenc.framework', 'libvorbisfile.framework', 'opus.framework', 'shine.framework', 'soxr.framework', 'speex.framework', 'twolame.framework', 'vo-amrwbenc.framework', 'wavpack.framework'

end  
