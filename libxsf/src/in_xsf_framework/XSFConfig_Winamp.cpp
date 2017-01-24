/*
 * xSF - Winamp-specification configuration handler
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2014-10-05
 *
 * Partially based on the vio*sf framework
 */

#include "XSFConfig.h"
#include "XSFCommon.h"
#include <winamp/in2.h>
#include <winamp/wa_ipc.h>

extern In_Module inMod;

class XSFConfigIO_Winamp : public XSFConfigIO
{
protected:
	friend class XSFConfigIO;
	std::wstring iniFilename;
	HINSTANCE hInst;

	XSFConfigIO_Winamp();
public:
	void SetValueString(const std::string &name, const std::string &value);
	std::string GetValueString(const std::string &name, const std::string &defaultValue) const;
	void SetHInstance(HINSTANCE hInstance);
	HINSTANCE GetHInstance() const;
};

XSFConfigIO *XSFConfigIO::Create()
{
	return new XSFConfigIO_Winamp();
}

XSFConfigIO_Winamp::XSFConfigIO_Winamp() : iniFilename(L"")
{
	if (SendMessage(inMod.hMainWindow, WM_WA_IPC, 0, IPC_GETVERSION) >= 0x2900)
		this->iniFilename = ConvertFuncs::StringToWString(reinterpret_cast<char *>(SendMessage(inMod.hMainWindow, WM_WA_IPC, 0, IPC_GETINIFILE)));
	else
	{
		auto executablePath = std::vector<wchar_t>(MAX_PATH / 2);

		DWORD result;
		do
		{
			executablePath.resize(executablePath.size() * 2);
			result = GetModuleFileNameW(nullptr, &executablePath[0], executablePath.size());
		} while (result == executablePath.size());

		if (!result)
			throw std::runtime_error("Unable to get path to plugin.");

		this->iniFilename = ExtractDirectoryFromPath(std::wstring(executablePath.begin(), executablePath.begin() + result)) + L"plugins.ini";
	}
}

void XSFConfigIO_Winamp::SetValueString(const std::string &name, const std::string &value)
{
	WritePrivateProfileStringW(ConvertFuncs::StringToWString(XSFConfig::commonName).c_str(), ConvertFuncs::StringToWString(name).c_str(), ConvertFuncs::StringToWString(value).c_str(), this->iniFilename.c_str());
}

std::string XSFConfigIO_Winamp::GetValueString(const std::string &name, const std::string &defaultValue) const
{
	auto value = std::vector<wchar_t>(MAX_PATH / 2);

	DWORD result;
	do
	{
		value.resize(value.size() * 2);
		result = GetPrivateProfileStringW(ConvertFuncs::StringToWString(XSFConfig::commonName).c_str(), ConvertFuncs::StringToWString(name).c_str(), ConvertFuncs::StringToWString(defaultValue).c_str(), &value[0], value.size(), this->iniFilename.c_str());
	} while (result + 1 == value.size());

	if (!result)
		throw std::runtime_error("Unable to get value from INI file.");

	return ConvertFuncs::WStringToString(std::wstring(value.begin(), value.begin() + result));
}

void XSFConfigIO_Winamp::SetHInstance(HINSTANCE hInstance)
{
	this->hInst = hInstance;
}

HINSTANCE XSFConfigIO_Winamp::GetHInstance() const
{
	return this->hInst;
}
