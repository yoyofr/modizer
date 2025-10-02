
#pragma once

#define RAPIDJSON_HAS_STDSTRING 1
#include "../external/rapidjson/include/rapidjson/writer.h"
#include "../external/rapidjson/include/rapidjson/prettywriter.h"
#include "../external/rapidjson/include/rapidjson/filewritestream.h"
#include "../external/rapidjson/include/rapidjson/stringbuffer.h"

#include "../external/rapidjson/include/rapidjson/document.h"



class JsonWriter
{
public:
    
    JsonWriter(rapidjson::PrettyWriter<rapidjson::StringBuffer> &writer)
    :m_writer(writer)
    {
        
    }
    
    JsonWriter &Key(const char *str)
    {
        m_writer.Key(str);
        return *this;
    }
    
    JsonWriter &Key(const std::string &str)
    {
        m_writer.Key(str);
        return *this;
    }
    

    JsonWriter &Write(const char *key, const char *value)
    {
        m_writer.Key(key);
        m_writer.String(value);
        return *this;
    }

    JsonWriter &Write(const char *key, const std::string &value)
    {
        m_writer.Key(key);
        m_writer.String(value);
        return *this;
    }


    JsonWriter &Write(const char *key, int value)
    {
        m_writer.Key(key);
        m_writer.Int(value);
        return *this;
    }

    JsonWriter &Write(const char *key, double value)
    {
        m_writer.Key(key);
        m_writer.Double(value);
        return *this;
    }


    JsonWriter &Write(const char *key, float value)
    {
        m_writer.Key(key);
        m_writer.Double((double)value);
        return *this;
    }

    JsonWriter &Write(const char *key, uint64_t value)
    {
        m_writer.Key(key);
        m_writer.Uint64(value);
        return *this;
    }


    JsonWriter &Write(const char *key, int64_t value)
    {
        m_writer.Key(key);
        m_writer.Int64(value);
        return *this;
    }
    
    
    //
    
    
    
    JsonWriter &Value(const char *value)
    {
        m_writer.String(value);
        return *this;
    }

    JsonWriter &Value(const std::string &value)
    {
        m_writer.String(value);
        return *this;
    }


    JsonWriter &Value(int value)
    {
        m_writer.Int(value);
        return *this;
    }

    JsonWriter &Value(double value)
    {
        m_writer.Double(value);
        return *this;
    }


    JsonWriter &Value(float value)
    {
        m_writer.Double((double)value);
        return *this;
    }

    JsonWriter &Value(uint64_t value)
    {
        m_writer.Uint64(value);
        return *this;
    }


    JsonWriter &Value(int64_t value)
    {
        m_writer.Int64(value);
        return *this;
    }
    

    
    void StartArray()
    {
        m_writer.StartArray();
    }

    void EndArray()
    {
        m_writer.EndArray();
    }
    
    
    void StartObject()
    {
        m_writer.StartObject();
    }

    void EndObject()
    {
        m_writer.EndObject();
    }
    
    bool IsComplete()
    {
        return m_writer.IsComplete();
    }

protected:
    rapidjson::PrettyWriter<rapidjson::StringBuffer> &m_writer;
};


class JsonStringWriter : public JsonWriter
{
public:

    JsonStringWriter()
    : JsonWriter(m_stringWriter),
     m_stringWriter(m_stringBuffer)
    {

    }
    
    const char *ToCString()
    {
        return m_stringBuffer.GetString();
    }
    
    bool SaveToFile(const std::string &path)
    {
        return FileWriteAllText( path, ToCString() );
    }

private:
    rapidjson::StringBuffer m_stringBuffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> m_stringWriter;

};
