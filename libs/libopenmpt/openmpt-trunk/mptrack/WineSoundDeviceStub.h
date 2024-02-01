/*
 * WineSoundDeviceStub.h
 * ---------------------
 * Purpose: Stub sound device driver class connection to WineSupport Wrapper.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include "openmpt/sounddevice/SoundDeviceBase.hpp"

#include "../common/ComponentManager.h"


extern "C" {
	typedef struct OpenMPT_Wine_Wrapper_SoundDevice OpenMPT_Wine_Wrapper_SoundDevice;
};


OPENMPT_NAMESPACE_BEGIN


class ComponentWineWrapper;


namespace SoundDevice {


class SoundDeviceStub
	: public SoundDevice::IBase
{

public:

	static std::unique_ptr<SoundDevice::BackendInitializer> BackendInitializer() { return std::make_unique< SoundDevice::BackendInitializer>(); }
	static std::vector<SoundDevice::Info> EnumerateDevices(ILogger &logger, SoundDevice::SysInfo sysInfo);

public:

	SoundDeviceStub(ILogger &logger, SoundDevice::Info info, SoundDevice::SysInfo sysInfo);
	
	virtual ~SoundDeviceStub();

public:

	virtual void SetCallback(SoundDevice::ICallback *callback);
	virtual void SetMessageReceiver(SoundDevice::IMessageReceiver *receiver);

	virtual SoundDevice::Info GetDeviceInfo() const;

	virtual SoundDevice::Caps GetDeviceCaps() const;
	virtual SoundDevice::DynamicCaps GetDeviceDynamicCaps(const std::vector<uint32> &baseSampleRates);

	virtual bool Init(const SoundDevice::AppInfo &appInfo);
	virtual bool Open(const SoundDevice::Settings &settings);
	virtual bool Close();
	virtual bool Start();
	virtual void Stop();

	virtual FlagSet<RequestFlags> GetRequestFlags() const;

	virtual bool IsInited() const;
	virtual bool IsOpen() const;
	virtual bool IsAvailable() const;
	virtual bool IsPlaying() const;

	virtual bool IsPlayingSilence() const;
	virtual void StopAndAvoidPlayingSilence();
	virtual void EndPlayingSilence();

	virtual bool OnIdle();
	
	virtual SoundDevice::Settings GetSettings() const;
	virtual SampleFormat GetActualSampleFormat() const;
	virtual SoundDevice::BufferAttributes GetEffectiveBufferAttributes() const;

	virtual SoundDevice::TimeInfo GetTimeInfo() const;
	virtual SoundDevice::StreamPosition GetStreamPosition() const;

	virtual bool DebugIsFragileDevice() const;
	virtual bool DebugInRealtimeCallback() const;

	virtual SoundDevice::Statistics GetStatistics() const;

	virtual bool OpenDriverSettings();

private:

	ComponentHandle<ComponentWineWrapper> w;
	OpenMPT_Wine_Wrapper_SoundDevice * impl;

};


} // namespace SoundDevice

OPENMPT_NAMESPACE_END
