


#include "IAudioSource.h"
#include "platform.h"

struct wav_format {
    uint16_t audio_format; // Should be 1 for PCM. 3 for IEEE Float
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate; // Number of bytes per second. sample_rate * num_channels * Bytes Per Sample
    uint16_t sample_alignment; // num_channels * Bytes Per Sample
    uint16_t bit_depth; // Number of bits per sample
};

class BinReader
{
public:
    BinReader(std::vector<uint8_t> &data)
    :m_data(data)
    {
        
    }
    
    bool EndOfFile()
    {
        return m_pos >= m_data.size();
    }
    
    void Skip(size_t count)
    {
        m_pos += count;
    }
    
    std::string ReadTag()
    {
        std::string tag;
        for (int i=0; i < 4; i++)
        {
            tag += (char)ReadByte();
        }
        return tag;
    }
    
    uint8_t ReadByte()
    {
        if (m_pos >= m_data.size())
            return 0;
        return m_data[m_pos++];
    }

    int16_t ReadInt16LE()
    {
        int16_t v;
        v  = (int16_t)ReadByte() << 0;
        v |= (int16_t)ReadByte() << 8;
        return v;
    }

    uint16_t ReadUInt16LE()
    {
        uint16_t v;
        v  = (uint16_t)ReadByte() << 0;
        v |= (uint16_t)ReadByte() << 8;
        return v;
    }

    uint32_t ReadUInt32LE()
    {
        uint32_t v;
        v  = (uint32_t)ReadByte() << 0;
        v |= (uint32_t)ReadByte() << 8;
        v |= (uint32_t)ReadByte() << 16;
        v |= (uint32_t)ReadByte() << 24;
        return v;
    }
    
    void ReadBytes(uint8_t *dest, size_t count)
    {
        for (size_t i=0; i < count; i++)
        {
            dest[i] = ReadByte();
        }
    }

protected:
    size_t                m_pos = 0;
    std::vector<uint8_t> &m_data;
    
};

static bool ReadWavFileSamples(std::string path, float &sampleRate, std::vector<Sample16> &samples)
{
    std::vector<uint8_t> fileData;
    if (!FileReadAllBytes(path, fileData))
        return false;
    
    BinReader br(fileData);
    
    // read tag
    if (br.ReadTag() != "RIFF")
        return false;
    
    uint32_t wav_size = br.ReadUInt32LE();
    (void)wav_size;
    
    if (br.ReadTag() != "WAVE")
        return false;

    
    wav_format format;
    memset(&format, 0, sizeof(format));
    while (!br.EndOfFile())
    {
        std::string chunk_tag = br.ReadTag();
        uint32_t chunk_size = br.ReadUInt32LE();

        if (chunk_tag == "fmt ")
        {
            format.audio_format = br.ReadUInt16LE();
            format.num_channels = br.ReadUInt16LE();
            format.sample_rate = br.ReadUInt32LE();
            format.byte_rate = br.ReadUInt32LE();
            format.sample_alignment = br.ReadUInt16LE();
            format.bit_depth = br.ReadUInt16LE();
        }
        else if (chunk_tag == "data")
        {
            if   (
                  format.sample_alignment == 0 ||
                  format.audio_format != 1 ||
                  format.num_channels != 2 ||
                  format.bit_depth != 16)
            {
                return false;
            }

            sampleRate = (float)format.sample_rate;

            // found data
            int numSamples = chunk_size / format.sample_alignment;
            samples.reserve(numSamples);
            
            for (int i=0; i < numSamples; i++)
            {
                int16_t left  = br.ReadInt16LE();
                int16_t right = br.ReadInt16LE();
                
                Sample16 s;
                s.ch[0] = left; //Sample::ConvertS16ToFloat(left);
                s.ch[1] = right; //Sample::ConvertS16ToFloat(right);
                samples.push_back(s);
            }
            break;
        }
        else
        {
            // skip chunk
            br.Skip(chunk_size);
        }
    }
    return true;
}





class MemoryAudioSource : public IAudioSource
{
    std::string mPath;
    std::string mDescription;
public:
    MemoryAudioSource()
    {
    }

    virtual ~MemoryAudioSource()
    {

    }
    
    virtual void Reset() override
    {
        m_pos = 0;
    }

    virtual const std::string &GetDescription() const override
    {
        return mDescription;
    }


    bool ReadFromWavFile(std::string path)
    {
        mPath = PathGetFileName(path);
        
        mDescription = "WavFile";
        mDescription += " - ";
        mDescription += mPath;

        return ReadWavFileSamples(path, m_sampleRate, m_samples);
    }
    
    Sample ReadSample()
    {
        if (m_pos >= m_samples.size())
        {
            m_pos = 0;
            if (m_samples.empty())
            {
                return Sample();
            }
        }
        
        return Sample::Convert(m_samples[m_pos++], m_sampleScale);
    }
    
    virtual void ReadAudioFrame(float dt, SampleBuffer<Sample> &samples) override
    {
        samples.SetSampleRate((float)m_sampleRate);
        
        
        int count = (int) ((float)m_sampleRate * dt);
        samples.clear();
        samples.reserve(count);
        for (int i=0 ; i < count; i++)
        {
            samples.push_back(ReadSample());
        }
      
    }

protected:
    float m_sampleRate = 0;
    float m_sampleScale = 1.0f;
    size_t m_pos = 0;
    std::vector<Sample16> m_samples;
};


IAudioSourcePtr OpenWavFileAudioSource(const char *path)
{
    auto source = std::make_shared<MemoryAudioSource>();
    if (!source->ReadFromWavFile(path))
    {
        return nullptr;
    }
    return source;
}




