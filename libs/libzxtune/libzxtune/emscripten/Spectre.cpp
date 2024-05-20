/**
* Impl of ZxTune interface used by SpectreZX player
* code based on "foobar2000 plugin"
*
* Copyright (C) 2015 Juergen Wothke
*
* LICENSE
* 
* This library is free software; you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2.1 of the License, or (at
* your option) any later version. This library is distributed in the hope
* that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
* warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* 
* You should have received a copy of the GNU General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
**/

//local includes
#define ZXTUNE_API ZXTUNE_API_EXPORT
#include "Spectre.h"
//common includes
#include <contract.h>
#include "cycle_buffer.h"
#include "error_tools.h"
#include "progress_callback.h"
//library includes
#include "binary/container.h"
#include "binary/container_factories.h"
#include "core/core_parameters.h"
#include "core/data_location.h"
#include "core/module_attrs.h"
#include "core/module_detect.h"
#include "core/module_open.h"
#include "core/module_holder.h"
#include "core/module_player.h"
#include "core/module_types.h"
#include "parameters/container.h"
#include "platform/version/api.h"
#include "sound/sound_parameters.h"
#include "sound/service.h"
#include "sound/render_params.h"

#include "core/plugins/enumerator.h"


//std includes
#include <map>
#include <iostream>
#include <string>
#include <stdexcept>

//boost includes
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <boost/static_assert.hpp>
#include <boost/range/end.hpp>
#include <boost/type_traits/is_signed.hpp>


#include "player.h"

// -------------------- SongInfo -------------------

static const char *EMPTY= "";

class SongInfo::SongInfoImpl {
public:
	SongInfoImpl() {}
	virtual ~SongInfoImpl() {}

	void set_codec(std::string c)		{ _codec= c.empty() ? std::string(EMPTY) : c; }
	const char *get_codec()		 		{ return _codec.c_str(); }

	void set_author(std::string a) 		{ _author= a.empty() ? std::string(EMPTY) : a;  }
	const char *get_author()		 	{ return _author.c_str(); }

	void set_title(std::string t) 		{ _title= t.empty() ? std::string(EMPTY) : t; }
	const char *get_title()		 		{ return _title.c_str(); }

	void set_sub_path(std::string n)	{ _subPath= n.empty() ? std::string(EMPTY) : n; }
	const char *get_subpath()		 	{ return _subPath.c_str(); } // archive path to specific sub-song
	
	void set_comment(std::string c) 	{ _comment= c.empty() ? std::string(EMPTY) : c; }
	const char *get_comment()		 	{ return _comment.c_str(); }
	
	void set_program(std::string p) 	{ _program= p.empty() ? std::string(EMPTY) : p; }
	const char *get_program()			{ return _program.c_str(); }
	
	void set_track_number(int n) 		{ _track_number = std::to_string(n);}
	const char * get_track_number()		{ return _track_number.c_str(); }
	
	void set_total_tracks(int n) 		{ _total_tracks= std::to_string(n);}
	const char * get_total_tracks() 	{ return _total_tracks.c_str();;}
	
	void reset() {
		_codec = std::string(EMPTY);
		_author = std::string(EMPTY);
		_title = std::string(EMPTY);
		_subPath = std::string(EMPTY);
		_comment = std::string(EMPTY);
		_program = std::string(EMPTY);
		_track_number = std::string(EMPTY);
		_total_tracks = std::string(EMPTY);
	}
	
private:
	std::string _codec;
	std::string _author;
	std::string _title;
	std::string _subPath;
	std::string _comment;
	std::string _program;
	std::string _track_number;
	std::string _total_tracks;
};

SongInfo::SongInfo() : _pimpl(new SongInfoImpl()) {}
SongInfo::~SongInfo() 						{ delete _pimpl; _pimpl= 0; }
const char *SongInfo::get_codec()		 	{ return _pimpl->get_codec(); }
const char *SongInfo::get_author()		 	{ return _pimpl->get_author(); }
const char *SongInfo::get_title()		 	{ return _pimpl->get_title(); }
const char *SongInfo::get_subpath()			{ return _pimpl->get_subpath(); }
const char *SongInfo::get_comment()		 	{ return _pimpl->get_comment(); }
const char *SongInfo::get_program()		 	{ return _pimpl->get_program(); }
const char *SongInfo::get_track_number()	{ return _pimpl->get_track_number(); }
const char *SongInfo::get_total_tracks()	{ return _pimpl->get_total_tracks(); }
void SongInfo::reset()		 	{ _pimpl->reset(); }


// -------------------- ZxTuneWrapper -------------------

class ZxTuneWrapper::ZxTuneWrapperImpl {
private:
	unsigned int _frameDurationMs;
public:
	ZxTuneWrapperImpl(std::string modulePath, const void* data, size_t size, int sampleRate) {
		_sampleRate = sampleRate;
		_moduleFilePath = modulePath; 
		_inputFile = createData(data, size);
	}
	
	virtual ~ZxTuneWrapperImpl() {
		close();
	}
	
	void close() {
		if(_inputFile)
		{
			_player.reset();
			_inputModules.clear();
			_moduleHolder.reset();
			_inputFile.reset();
		}
	}

	void parseModules() {
		struct ModuleDetector : public Module::DetectCallback
		{
			ModuleDetector(Modules* _mods) : modules(_mods) {}
			virtual void ProcessModule(ZXTune::DataLocation::Ptr location, ZXTune::Plugin::Ptr, Module::Holder::Ptr holder) const
			{
				ModuleDesc m;
				m.module = holder;
				m.subpath = location->GetPath()->AsString();
				modules->push_back(m);
			}
			virtual Log::ProgressCallback* GetProgress() const { return NULL; }
			virtual Parameters::Accessor::Ptr GetPluginsParameters() const { return Parameters::Container::Create(); }
			Modules* modules;
		};

		ModuleDetector md(&_inputModules);
		Module::Detect(ZXTune::CreateLocation(_inputFile), md);
		if(_inputModules.empty())
		{
			_inputFile.reset();
			throw  std::invalid_argument("io unsupported format");
		}
	}
    
    void setLoopMode(int loop) {
        Parameters::Container::Ptr params= _player->GetParameters();
        params->SetValue(Parameters::ZXTune::Sound::LOOPED, 1);
    }
		
	void decodeInitialize(unsigned int p_subsong, SongInfo & p_info) {
		std::string subpath = get_subpath(p_subsong, p_info);

		ZXTune::DataLocation::Ptr loc= ZXTune::OpenLocation(Parameters::Container::Create(), _inputFile, subpath);
		_moduleHolder = Module::Open(loc);
		if(!_moduleHolder)
			throw  std::invalid_argument("io unsupported format"); 

		Module::Information::Ptr mi = _moduleHolder->GetModuleInformation();
		if(!mi)
			throw  std::invalid_argument("io unsupported format"); 

		_player = PlayerWrapper::Create(_moduleHolder, _sampleRate);
				
		if(!_player)
			throw  std::invalid_argument("io unsupported format"); 	

	   _player->GetRenderer()->SetPosition(0);

	   
		Parameters::Container::Ptr params= _player->GetParameters();
		const Sound::RenderParameters::Ptr sound = Sound::RenderParameters::Create(params);
		_frameDurationMs = sound->FrameDuration().Get() / 1000;	// API delivers micros NOT millis
	}
	
	void get_song_info(unsigned int p_subsong, SongInfo & p_info)	{
		SongInfo::SongInfoImpl &info= *(p_info._pimpl);
	
		Module::Information::Ptr mi;
		Parameters::Accessor::Ptr props;
		std::string subpath;
		if(!_inputModules.empty())
		{
			if(p_subsong > _inputModules.size())
				throw  std::invalid_argument("io unsupported format");
			mi = _inputModules[p_subsong - 1].module->GetModuleInformation();
			props = _inputModules[p_subsong - 1].module->GetModuleProperties();
			if(!mi)
				throw  std::invalid_argument("io unsupported format");
			subpath = _inputModules[p_subsong - 1].subpath;
			
			info.set_sub_path(subpath);
		}
		else
		{
			subpath = get_subpath(p_subsong, p_info);
			Module::Holder::Ptr m = Module::Open(ZXTune::OpenLocation(Parameters::Container::Create(), _inputFile, subpath));
			if(!m)
				throw  std::invalid_argument("io unsupported format");
			mi = m->GetModuleInformation();
			props = m->GetModuleProperties();
			if(!mi)
				throw  std::invalid_argument("io unsupported format");
		}
		
		String type;
		info.set_codec(props->FindValue(Module::ATTR_TYPE, type) ? type : std::string("undefined"));
		
		String author;
		info.set_author(props->FindValue(Module::ATTR_AUTHOR, author) ? author : std::string("undefined"));

		String title;
		if(props->FindValue(Module::ATTR_TITLE, title) && !title.empty())
			info.set_title(title);
		else
			info.set_title(subpath);
		
		String comment;
		info.set_comment(props->FindValue(Module::ATTR_COMMENT, comment) ? comment : std::string("undefined"));

		String program;
		info.set_program(props->FindValue(Module::ATTR_PROGRAM, program) ? program : std::string("undefined"));
		
		if(_inputModules.size() > 1)
		{
			info.set_track_number(p_subsong);
			info.set_total_tracks(_inputModules.size());
		} else {
			info.set_track_number(0);
			info.set_total_tracks(1);
		}
	}

	int render_sound(void* buffer, size_t samples) {	
		try {
			return _player->RenderSound(reinterpret_cast<Sound::Sample*>(buffer), samples);
		}
		catch (const Error&) {
			return -1;
		}
		catch (const std::exception&) {
			return -1;
		}
	}
	
	int get_current_position() {
		return _player->GetRenderer()->GetTrackState()->Frame() * _frameDurationMs;
	}
	
	int get_max_position() {
		return _moduleHolder->GetModuleInformation()->FramesCount() * _frameDurationMs;
	}
    
    int get_channels_count() {
        return _player->GetRenderer()->GetHWChannels();;
    }
    
    const char *get_channel_name(int chan) {
        return _player->GetRenderer()->GetHWChannelName(chan);
    }
    
    const char *get_system_name() {
        return _player->GetRenderer()->GetHWSystemName();
    }
	
	void seek_position(int ms) {
	   _player->GetRenderer()->SetPosition(ms/_frameDurationMs);
	}
	
	int getSampleRate() {
		return _sampleRate;
	}
    
    char *get_all_extension() {
        std::cout<<"Player plugins\n";
        const ZXTune::PlayerPluginsEnumerator::Ptr usedPlugins = ZXTune::PlayerPluginsEnumerator::Create();
        for (ZXTune::PlayerPlugin::Iterator::Ptr iter = usedPlugins->Enumerate(); iter->IsValid(); iter->Next())
        {
          const ZXTune::PlayerPlugin::Ptr plugin = iter->Get();
            std::cout<<plugin->GetDescription()->Id()<<",";
//          if (DataLocation::Ptr result = plugin->Open(coreParams, location, subPath))
//          {
//            return result;
//          }
        }
        std::cout<<"\nArchive plugins\n";
        const ZXTune::ArchivePluginsEnumerator::Ptr usedAPlugins = ZXTune::ArchivePluginsEnumerator::Create();
        for (ZXTune::ArchivePlugin::Iterator::Ptr iter = usedAPlugins->Enumerate(); iter->IsValid(); iter->Next())
        {
          const ZXTune::ArchivePlugin::Ptr plugin = iter->Get();
            std::cout<<plugin->GetDescription()->Description()<<"\n";
//          if (DataLocation::Ptr result = plugin->Open(coreParams, location, subPath))
//          {
//            return result;
//          }
        }
        return NULL;
    }
protected:
	std::string get_subpath(unsigned int p_subsong, SongInfo & p_info) {
		const char* subpath = p_info.get_subpath();
		if(subpath && strlen(subpath))
			return std::string(subpath);
		return std::string();
	}
	
	Binary::Container::Ptr createData(const void* data, size_t size) {
		try	{
			Binary::Container::Ptr result = Binary::CreateContainer(data, size);	// use my existing "aligned" buffer
			return result;
		} catch (const std::exception& e) {
			std::cerr << "ERROR in createData() "<< e.what()<<std::endl;		
			return Binary::Container::Ptr();
		}
	}
	
private:
	int 					_sampleRate;
	std::string				_moduleFilePath;
	Binary::Container::Ptr	_inputFile;

	struct ModuleDesc
	{
		Module::Holder::Ptr module;
		std::string subpath;
	};
	typedef std::vector<ModuleDesc> Modules;
	Modules					_inputModules;
	Module::Holder::Ptr		_moduleHolder;
	PlayerWrapper::Ptr		_player;
};

ZxTuneWrapper::ZxTuneWrapper(std::string p, const void* data, size_t size, int sampleRate) : _pimpl(new ZxTuneWrapper::ZxTuneWrapperImpl(p, data, size, sampleRate)) {
}

ZxTuneWrapper::~ZxTuneWrapper() { 
	delete _pimpl; _pimpl = 0;
}

void ZxTuneWrapper::parseModules() {
	_pimpl->parseModules();
}

void ZxTuneWrapper::setLoopMode(int loop) {
    _pimpl->setLoopMode(loop);
}


void ZxTuneWrapper::decodeInitialize(unsigned int p_subsong, SongInfo & p_info) {
	_pimpl->decodeInitialize(p_subsong, p_info);
}

void ZxTuneWrapper::get_song_info(unsigned int p_subsong, SongInfo & p_info) {
	_pimpl->get_song_info(p_subsong, p_info);
}

int ZxTuneWrapper::render_sound(void* buffer, size_t samples) {
	return _pimpl->render_sound(buffer, samples);
}

int ZxTuneWrapper::get_current_position() {
	return _pimpl->get_current_position();
}

void ZxTuneWrapper::seek_position(int ms) {
	return _pimpl->seek_position(ms);
}

int ZxTuneWrapper::get_max_position() {
	return _pimpl->get_max_position();
}

int ZxTuneWrapper::getSampleRate() {
	return _pimpl->getSampleRate();
}

int ZxTuneWrapper::get_channels_count() {
    return _pimpl->get_channels_count();
}

const char *ZxTuneWrapper::get_channel_name(int chan) {
    return _pimpl->get_channel_name(chan);
}

const char *ZxTuneWrapper::get_system_name() {
    return _pimpl->get_system_name();
}


char *ZxTuneWrapper::get_all_extension() {
    return _pimpl->get_all_extension();
}
