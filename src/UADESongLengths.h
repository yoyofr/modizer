//
//  UADESongLengths.h
//  modizer
//
//  Lookup of the UADE song lengths database (Resources/UADE/songlengths.tsv.gz).
//
//  Gzipped TSV, one line per module, 3 tab separated columns:
//    1  hash12     md5 of the uncompressed module, truncated to its 12 first hex chars, lowercase
//    2  minsubsong index of the first subsong (0 or 1 usually, depends on the player)
//    3  space separated "length_ms,songend" pairs, one per consecutive subsong
//
//  The real subsong index is minsubsong + position in the list.
//
//  songend tells how the player decided the song was over:
//    p player (most reliable)   l loop      s silence   v volume (faded out)
//    n nosound (never played)   e error     t timeout   r repeat
//  Combos are joined with '+' (p+s). Anything after the first ',' is kept as-is: a ",!" suffix
//  marks a subsong duplicating another one (ex. "p,!"), hence the head code test.
//

#ifndef UADESongLengths_h
#define UADESongLengths_h

#include <string>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <zlib.h>

class UADE_SongLengths {
public:
    UADE_SongLengths() : mLoaded(false) {}

    bool loaded() const { return mLoaded; }

    //parses the gzipped TSV, keeps a compact index in memory. Idempotent.
    bool load(const char *gzPath);

    //index of the first subsong, -1 when the module is unknown
    int minSubsong(const char *md5) const;
    //number of subsongs, 0 when the module is unknown
    int subsongCount(const char *md5) const;

    //length in ms of an absolute subsong index, -1 when unknown or not usable
    int lengthForSubsong(const char *md5,int subsong) const;
    //head songend code of an absolute subsong index, 0 when unknown
    char songEndForSubsong(const char *md5,int subsong) const;

private:
    struct t_entry {
        unsigned long long hash;
        unsigned int offset;    //index of the first length in mLengths
        unsigned short count;
        short minsubsong;
    };

    bool mLoaded;
    std::vector<t_entry> mEntries;   //sorted by hash
    std::vector<int> mLengths;
    std::vector<char> mSongEnds;     //head code of each subsong

    static unsigned long long hashOf(const char *md5);
    const t_entry *find(const char *md5) const;
    int indexOf(const t_entry *entry,int subsong) const;
};

//12 first hex chars of the md5, case insensitive, 0 when malformed
inline unsigned long long UADE_SongLengths::hashOf(const char *md5) {
    if (md5==NULL) return 0;

    unsigned long long hash=0;
    for (int i=0;i<12;i++) {
        char c=md5[i];
        int v;
        if ((c>='0')&&(c<='9')) v=c-'0';
        else if ((c>='a')&&(c<='f')) v=c-'a'+10;
        else if ((c>='A')&&(c<='F')) v=c-'A'+10;
        else return 0;
        hash=(hash<<4)|v;
    }
    return hash;
}

inline bool UADE_SongLengths::load(const char *gzPath) {
    if (mLoaded) return true;
    if (gzPath==NULL) return false;

    gzFile gzf=gzopen(gzPath,"rb");
    if (gzf==NULL) return false;

    mEntries.reserve(500000);
    mLengths.reserve(550000);
    mSongEnds.reserve(550000);

    //read line per line to avoid holding the whole uncompressed file in memory
    static const int LINE_MAX_LEN=32*1024;
    char *lineBuffer=(char*)malloc(LINE_MAX_LEN);
    if (lineBuffer==NULL) { gzclose(gzf); return false; }

    while (gzgets(gzf,lineBuffer,LINE_MAX_LEN)!=NULL) {
        const char *line=lineBuffer;
        size_t len=strlen(lineBuffer);

        while ((len>0)&&((line[len-1]=='\n')||(line[len-1]=='\r'))) len--;
        if (len<15) continue;   //12 hex + tab + at least the minsubsong

        t_entry entry;
        entry.hash=hashOf(line);
        if (entry.hash==0) continue;
        if (line[12]!='\t') continue;

        //minsubsong
        size_t i=13;
        int minsubsong=0;
        bool gotDigit=false;
        while ((i<len)&&(line[i]>='0')&&(line[i]<='9')) { minsubsong=minsubsong*10+(line[i]-'0'); i++; gotDigit=true; }
        if (!gotDigit) continue;
        if ((i>=len)||(line[i]!='\t')) continue;
        i++;

        entry.minsubsong=(short)minsubsong;
        entry.offset=(unsigned int)mLengths.size();
        entry.count=0;

        //"length_ms,songend" pairs, space separated
        while (i<len) {
            while ((i<len)&&(line[i]==' ')) i++;
            if (i>=len) break;

            int length=0;
            bool gotLength=false;
            while ((i<len)&&(line[i]>='0')&&(line[i]<='9')) { length=length*10+(line[i]-'0'); i++; gotLength=true; }
            if (!gotLength) break;

            char songend=0;
            if ((i<len)&&(line[i]==',')) {
                i++;
                if (i<len) songend=line[i];   //head code only, the rest ('+s', ',!') is dropped
            }
            while ((i<len)&&(line[i]!=' ')) i++;

            mLengths.push_back(length);
            mSongEnds.push_back(songend);
            entry.count++;
        }

        if (entry.count==0) continue;
        mEntries.push_back(entry);
    }

    free(lineBuffer);
    gzclose(gzf);

    struct {
        bool operator()(const t_entry &a,const t_entry &b) const { return a.hash<b.hash; }
    } byHash;
    std::sort(mEntries.begin(),mEntries.end(),byHash);

    mLoaded=!mEntries.empty();
    return mLoaded;
}

inline const UADE_SongLengths::t_entry *UADE_SongLengths::find(const char *md5) const {
    if (!mLoaded) return NULL;

    unsigned long long hash=hashOf(md5);
    if (hash==0) return NULL;

    size_t lo=0;
    size_t hi=mEntries.size();
    while (lo<hi) {
        size_t mid=lo+(hi-lo)/2;
        if (mEntries[mid].hash<hash) lo=mid+1;
        else hi=mid;
    }
    if ((lo<mEntries.size())&&(mEntries[lo].hash==hash)) return &mEntries[lo];
    return NULL;
}

inline int UADE_SongLengths::indexOf(const t_entry *entry,int subsong) const {
    if (entry==NULL) return -1;
    int pos=subsong-entry->minsubsong;
    if ((pos<0)||(pos>=(int)entry->count)) return -1;
    return (int)entry->offset+pos;
}

inline int UADE_SongLengths::minSubsong(const char *md5) const {
    const t_entry *entry=find(md5);
    if (entry==NULL) return -1;
    return entry->minsubsong;
}

inline int UADE_SongLengths::subsongCount(const char *md5) const {
    const t_entry *entry=find(md5);
    if (entry==NULL) return 0;
    return entry->count;
}

inline int UADE_SongLengths::lengthForSubsong(const char *md5,int subsong) const {
    const t_entry *entry=find(md5);
    int idx=indexOf(entry,subsong);
    if (idx<0) return -1;

    //'n' (nosound) and 'e' (error) mean the scanner got nothing usable
    char songend=mSongEnds[idx];
    if ((songend=='n')||(songend=='e')) return -1;

    int length=mLengths[idx];
    if (length<=0) return -1;
    return length;
}

inline char UADE_SongLengths::songEndForSubsong(const char *md5,int subsong) const {
    const t_entry *entry=find(md5);
    int idx=indexOf(entry,subsong);
    if (idx<0) return 0;
    return mSongEnds[idx];
}

#endif /* UADESongLengths_h */
