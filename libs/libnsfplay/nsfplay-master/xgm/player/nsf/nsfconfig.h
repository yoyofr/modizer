#ifndef _NSFCONFIG_H_
#define _NSFCONFIG_H_
#include "../player.h"

namespace xgm
{
  /** �f�o�C�X���̒ʂ��ԍ� */
  enum DeviceCode
  { APU = 0, DMC, FME7, MMC5, N106, VRC6, VRC7, FDS, NES_DEVICE_MAX };

  const int NES_CHANNEL_MAX = 32;

  class NSFPlayerConfig : public PlayerConfig
  {
  public:
    NSFPlayerConfig ();
    virtual ~NSFPlayerConfig();

    vcm::Value& GetDeviceConfig(int i, std::string key)
    {
      MutexGuard mg_(this);
      return data[(std::string)dname[i]+"_"+key];
    }

    vcm::Value& GetDeviceOption(int id, int opt)
    {
      MutexGuard mg_(this);
        static char itoa[] = "0123456789ABCDEF";
                
        //YOYOFR: old iOS have issues with libc++ and std::string when initializing with char for example
        //workaround
        std::string str=(std::string)(dname[id]);//+"_OPTION"+itoa[opt];
        str=str+"_OPTION";
        switch (opt) {
            case 0:str+="0";break;
            case 1:str+="1";break;
            case 2:str+="2";break;
            case 3:str+="3";break;
            case 4:str+="4'";break;
            case 5:str+="5";break;
            case 6:str+="6";break;
            case 7:str+="7";break;
            case 8:str+="8";break;
            case 9:str+="9";break;
        }
        //YOYOFR
        return data[str]; //(std::string)dname[id]+"_OPTION"+itoa[opt]];
    }

    // channel mix/pan config
    vcm::Value& GetChannelConfig(int id, std::string key)
    {
      MutexGuard mg_(this);
      if (id < 0) id = 0;
      if (id >= NES_CHANNEL_MAX) id = NES_CHANNEL_MAX-1;
      char num[5];
      vcm_itoa(id, num, 10);
      std::string str;
      str = "CHANNEL_";
      if (id < 10) str += "0";
      str += num;
      str += "_";
      str += key;
      return data[str];
    }

    /** �f�o�C�X���̖��O */
    static const char *dname[NES_DEVICE_MAX];

    // for channel/pan/mix
    static const char *channel_name[NES_CHANNEL_MAX]; // name of channel
    static int channel_device[NES_CHANNEL_MAX]; // device enum of channel
    static int channel_device_index[NES_CHANNEL_MAX]; // device channel index
    static int channel_track[NES_CHANNEL_MAX]; // trackinfo for channel
  };

}// namespace

#endif
