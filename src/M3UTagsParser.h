//
//  M3UTagsParser.h
//  modizer
//
//  Parser for the "!tags.m3u" tag files shipped with the vgmstream/txtp rips.
//  Those files are not playlists: the GME M3u_Playlist parser cannot read them (it chokes on the
//  BOM, ignores the unknown @TAGs and mangles the %TAGs into the previous global tag).
//
//  Format:
//    # @TAG value      global tag, applies to every following file
//    # %TAG value      tag applying to the next file line only
//    # $COMMAND ...    command, ignored
//    # anything else   plain comment, ignored
//    <filename>        the accumulated %TAGs apply to this file
//
//  Tag names are case insensitive, and so are the file names.
//

#ifndef M3UTagsParser_h
#define M3UTagsParser_h

#include <string>
#include <map>
#include <cstdio>

class M3U_Tags {
public:
    void clear() { mGlobal.clear(); mFiles.clear(); }
    bool loaded() const { return (!mGlobal.empty())||(!mFiles.empty()); }

    //returns false if the file cannot be read or holds no tag at all
    bool load(const char *path);

    //tag lookup, name is case insensitive, returns NULL when absent
    const char *globalTag(const char *name) const;
    const char *fileTag(const char *filename,const char *name) const;

private:
    typedef std::map<std::string,std::string> t_tagmap;

    t_tagmap mGlobal;
    std::map<std::string,t_tagmap> mFiles;   //key: lowercased file name

    static std::string lower(const std::string &str);
    static std::string trim(const std::string &str);
    static const char *lookup(const t_tagmap &tags,const std::string &name);
};

inline std::string M3U_Tags::lower(const std::string &str) {
    std::string res=str;
    for (size_t i=0;i<res.size();i++) {
        char c=res[i];
        if ((c>='A')&&(c<='Z')) res[i]=c-'A'+'a';
    }
    return res;
}

inline std::string M3U_Tags::trim(const std::string &str) {
    size_t start=0;
    size_t end=str.size();
    while ((start<end)&&((unsigned char)str[start]<=' ')) start++;
    while ((end>start)&&((unsigned char)str[end-1]<=' ')) end--;
    return str.substr(start,end-start);
}

inline const char *M3U_Tags::lookup(const t_tagmap &tags,const std::string &name) {
    t_tagmap::const_iterator it=tags.find(name);
    if (it==tags.end()) return NULL;
    return it->second.c_str();
}

inline const char *M3U_Tags::globalTag(const char *name) const {
    if (name==NULL) return NULL;
    return lookup(mGlobal,lower(name));
}

inline const char *M3U_Tags::fileTag(const char *filename,const char *name) const {
    if ((filename==NULL)||(name==NULL)) return NULL;

    std::string key=lower(filename);
    std::map<std::string,t_tagmap>::const_iterator it=mFiles.find(key);
    if (it==mFiles.end()) {
        //the tag file may reference the entries by name only, or the other way round
        size_t slash=key.find_last_of("/\\");
        if (slash!=std::string::npos) it=mFiles.find(key.substr(slash+1));
        if (it==mFiles.end()) {
            for (std::map<std::string,t_tagmap>::const_iterator i=mFiles.begin();i!=mFiles.end();++i) {
                size_t s=i->first.find_last_of("/\\");
                if ((s!=std::string::npos)&&(i->first.substr(s+1)==key)) { it=i; break; }
            }
        }
        if (it==mFiles.end()) return NULL;
    }
    return lookup(it->second,lower(name));
}

inline bool M3U_Tags::load(const char *path) {
    clear();
    if (path==NULL) return false;

    FILE *f=fopen(path,"rb");
    if (f==NULL) return false;

    std::string content;
    char buffer[4096];
    size_t rd;
    while ((rd=fread(buffer,1,sizeof(buffer),f))>0) content.append(buffer,rd);
    fclose(f);

    //skip the UTF-8 BOM if any
    size_t pos=0;
    if ((content.size()>=3)&&
        ((unsigned char)content[0]==0xEF)&&((unsigned char)content[1]==0xBB)&&((unsigned char)content[2]==0xBF)) pos=3;

    t_tagmap pending;   //%TAGs, applying to the next file line only

    while (pos<content.size()) {
        size_t eol=content.find('\n',pos);
        std::string line=(eol==std::string::npos?content.substr(pos):content.substr(pos,eol-pos));
        pos=(eol==std::string::npos?content.size():eol+1);

        line=trim(line);  //also drops the CR of the CRLF endings
        if (line.empty()) continue;

        if (line[0]=='#') {
            std::string comment=trim(line.substr(1));
            if (comment.empty()) continue;

            char marker=comment[0];
            if ((marker!='@')&&(marker!='%')) continue;  //plain comment or $COMMAND

            std::string body=comment.substr(1);
            size_t sep=body.find_first_of(" \t");
            if (sep==std::string::npos) continue;

            std::string name=lower(trim(body.substr(0,sep)));
            std::string value=trim(body.substr(sep+1));
            if (name.empty()||value.empty()) continue;

            if (marker=='@') mGlobal[name]=value;
            else pending[name]=value;
            continue;
        }

        //file line: the pending %TAGs apply to it
        if (!pending.empty()) {
            mFiles[lower(line)]=pending;
            pending.clear();
        }
    }

    return loaded();
}

#endif /* M3UTagsParser_h */
