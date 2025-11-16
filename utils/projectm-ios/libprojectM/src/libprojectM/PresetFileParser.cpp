#include "PresetFileParser.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <vector>

extern int mdz_pmMilkPermissiveEvalCode;

namespace libprojectM {

auto PresetFileParser::Read(const std::string& presetFile) -> bool
{
    std::ifstream presetStream(presetFile.c_str(), std::ios_base::in | std::ios_base::binary);
    return Read(presetStream);
}

auto PresetFileParser::Read(std::istream& presetStream) -> bool
{
    if (!presetStream.good())
    {
        return false;
    }

    presetStream.seekg(0, presetStream.end);
    auto fileSize = presetStream.tellg();
    presetStream.seekg(0, presetStream.beg);

    if (static_cast<size_t>(fileSize) > maxFileSize)
    {
        return false;
    }

    std::vector<char> presetFileContents(fileSize);
    presetStream.read(presetFileContents.data(), fileSize);

    if (presetStream.fail() || presetStream.bad())
    {
        return false;
    }

    size_t startPos{0}; //!< Starting position of current line
    size_t pos{0};      //!< Current read position

    auto parseLineIfDataAvailable = [this, &pos, &startPos, &presetFileContents]() {
        if (pos > startPos)
        {
            auto beg = presetFileContents.begin();
            std::string line(beg + startPos, beg + pos);
            ParseLine(line);
        }
    };

    while (pos < presetFileContents.size())
    {
        switch (presetFileContents[pos])
        {
            case '\r':
            case '\n':
                // EOL, skip over CRLF
                parseLineIfDataAvailable();
                startPos = pos + 1;
                break;

            case '\0':
                // Null char is not expected. Could be a random binary file.
                return false;
        }

        ++pos;
    }

    parseLineIfDataAvailable();

    return !m_presetValues.empty();
}



auto PresetFileParser::GetCode(const std::string& keyPrefix) const -> std::string
{
    auto lowerKey = ToLower(keyPrefix);

    std::stringstream code;                        //!< The parsed code
    std::string key(lowerKey.length() + 5, '\0'); //!< Allocate a string that can hold up to 5 digits.

    key.replace(0, lowerKey.length(), lowerKey);

    bool skipCode=false;
    for (int index{1}; index <= 99999; ++index)
    {
        key.replace(lowerKey.length(), 5, std::to_string(index));
        if (m_presetValues.find(key) == m_presetValues.end())
        {
            break;
        }
        
        auto line = m_presetValues.at(key);

        // Remove backtick char in shader code
        if (!line.empty() && line.at(0) == '`')
        {
            line.erase(0, 1);
        }

        //YOYOFR
        bool removeEndl=false;
#if 1
        //------------------
        //Manage comment
        //------------------
        if (skipCode) {
            size_t pos=line.find("*/");
            if (pos!=std::string::npos) {
                //end of comment found
                line.erase(0,pos+2);
                skipCode=false;
            }
        }
        
        if (!skipCode) {
            size_t pos=line.find("//");
            if (pos!=std::string::npos) {
                //comment, erase rest of the line
                line.erase(pos,line.length()-pos);
            } else {
                pos=line.find("/*");
                 if (pos!=std::string::npos) {
                     //is there an end comment on the same line ?
                     size_t pos2=line.find("*/",pos+2);
                     if (pos2!=std::string::npos) {
                         line.erase(pos,pos2+2-pos);
                     } else {
                         //no end of comment found on same line, move into skipCode mode
                         skipCode=true;
                     }
                 }
            }
        }
        
        if (skipCode) continue;
        
        //------------------
        // Permissive mode: allow to merge line together under certain conditions: no comment, no ';' at the end of line, ...
        // Allow several milk preset to compile as the code is sometime broken on 2 lines in the middle of a litteral
        
        if (mdz_pmMilkPermissiveEvalCode) {
            //Check if last char is a ';'
            int pos=(int)line.length()-1;
            while (pos>=0) {
                if (line.at(pos)==' ') pos--;
                else if (line.at(pos)=='\t') pos--;
                else break;
            }
            if (pos>0) {
                char last_char=line.at(pos);
                if ((line.find("//")==std::string::npos) &&
                    (line.find("/*")==std::string::npos) &&
                    (line.find("*/")==std::string::npos) &&
                    !( (pos>=3) && (line.at(pos-3)=='e')&&(line.at(pos-2)=='l')&&(line.at(pos-1)=='s')&&(line.at(pos)=='e') ) &&
                    (line.at(0)!='#') &&
                    (last_char!=';') &&
                    (last_char!='(') &&
                    (last_char!=')') &&
                    (last_char!='{') &&
                    (last_char!='}') &&
                    (last_char!=',')) {
                    removeEndl=true;
                }
            }
        }
#endif
        if (removeEndl) code << line;
        else code << line << std::endl;
        //
    }
    

    auto codeStr = code.str();
    
    return codeStr;
}

auto PresetFileParser::GetInt(const std::string& key, int defaultValue) -> int
{
    auto lowerKey = ToLower(key);
    if (m_presetValues.find(lowerKey) != m_presetValues.end())
    {
        try
        {
            return std::stoi(m_presetValues.at(lowerKey));
        }
        catch (std::logic_error&)
        {
        }
    }

    return defaultValue;
}

auto PresetFileParser::GetFloat(const std::string& key, float defaultValue) -> float
{
    auto lowerKey = ToLower(key);
    if (m_presetValues.find(lowerKey) != m_presetValues.end())
    {
        try
        {
            return std::stof(m_presetValues.at(lowerKey));
        }
        catch (std::logic_error&)
        {
        }
    }

    return defaultValue;
}

auto PresetFileParser::GetBool(const std::string& key, bool defaultValue) -> bool
{
    return GetInt(key, static_cast<int>(defaultValue)) > 0;
}

auto PresetFileParser::GetString(const std::string& key, const std::string& defaultValue) -> std::string
{
    auto lowerKey = ToLower(key);
    if (m_presetValues.find(lowerKey) != m_presetValues.end())
    {
        return m_presetValues.at(lowerKey);
    }

    return defaultValue;
}

const std::map<std::string, std::string>& PresetFileParser::PresetValues() const
{
    return m_presetValues;
}

void PresetFileParser::ParseLine(const std::string& line)
{
    // Search for first delimiter, either space or equal
    auto varNameDelimiterPos = line.find_first_of(" =");

    if (varNameDelimiterPos == std::string::npos || varNameDelimiterPos == 0)
    {
        // Empty line, delimiter at start of line or no delimiter found, skip.
        return;
    }

    // Convert key to lower case, as INI functions are not case-sensitive.
    std::string varName(ToLower(std::string(line.begin(), line.begin() + varNameDelimiterPos)));
    std::string value(line.begin() + varNameDelimiterPos + 1, line.end());

    // Only add first occurrence to mimic Milkdrop behaviour
    if (!varName.empty() && m_presetValues.find(varName) == m_presetValues.end())
    {
        m_presetValues.emplace(std::move(varName), std::move(value));
    }
}

auto PresetFileParser::ToLower(std::string str) -> std::string
{
    std::transform(str.begin(), str.end(), str.begin(),
                   [](unsigned char c){ return std::tolower(c); }
    );

    return str;
}

} // namespace libprojectM
