#pragma comment(lib, "setupapi.lib")
#include <Windows.h>
#include <initguid.h>
#include <cfgmgr32.h>
#include <SetupAPI.h>
#include <Usbiodef.h>

#include <vector>
#include <string>
#include <thread>
#include <iostream>
#include <functional>
#include <unordered_map>
#include <algorithm>

constexpr int scan_delay = 1000;
constexpr int eject_attempt_cap = 5;

class DeviceInfo
{
public:
	DEVINST device_instance;
	std::wstring device_name;
	std::wstring device_description;

	bool operator==(const DeviceInfo& other);
};


class controller
{
private:
	std::unordered_map <std::string, std::function<void()>> command_map;
public:
	void add_command(std::string name, const std::function<void()>& function);
	bool execute(const std::string& message);
};

bool to_exit = false;
std::vector<DeviceInfo> prevList{};

bool is_numeric(const std::string& str);
std::vector<DeviceInfo> get_devices();
bool eject(DeviceInfo& info);
void list_devices();
void scanner();


int main()
{
	setlocale(LC_ALL, "Russian");

	std::thread scanThread{ scanner };
	std::string input_str;

	controller cmdList;

	cmdList.add_command("help", []()
		{
			std::cout << "Avaliable commands:" << std::endl;
			std::cout << "\tcls                  - clear screen" << std::endl;
			std::cout << "\thelp                 - shows command list" << std::endl;
			std::cout << "\texit                 - quits application" << std::endl;
			std::cout << "\tlist                 - lists avaliable USB devices" << std::endl;
			std::cout << "\teject [drive_number] - ejects the device" << std::endl;
			std::cout << std::endl;
		});

	cmdList.add_command("cls", []()
		{
			system("cls");
		});

	cmdList.add_command("exit", []()
		{
			to_exit = true;
		});

	cmdList.add_command("list", []()
		{
			list_devices();
		});

	cmdList.add_command("eject", [&input_str]()
		{
			std::string device_to_eject = input_str.c_str() + 6;

			if (!is_numeric(device_to_eject))
			{
				std::cout << "Illegal arguments" << std::endl;
				return;
			}

			int deviceListNumber = atoi(device_to_eject.c_str());

			if (deviceListNumber > prevList.size())
			{
				std::cout << "Illegal device number" << std::endl;
				std::cout << std::endl;
				return;
			}

			if (!eject(prevList[deviceListNumber - 1]))
			{
				std::cout << "Device ejection error" << std::endl;
				std::cout << std::endl;
			}
		});

	while (!to_exit)
	{
		std::cout << ">";
		std::getline(std::cin, input_str);
		if (!cmdList.execute(input_str))
		{
			std::cout << "Unknown command" << std::endl;
		}
	}
	scanThread.join();
	return 0;
}

bool is_numeric(const std::string& str)
{
	if (str.empty())
	{
		return false;
	}
	for (auto i : str)
	{
		if (i < '0' || i > '9')
		{
			return false;
		}
	}
	return true;
}

std::vector<DeviceInfo> get_devices()
{
	std::vector<DeviceInfo> result;

	HDEVINFO hDevInfo = SetupDiGetClassDevs(&GUID_DEVINTERFACE_USB_DEVICE, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

	DWORD dwIndex = 0;
	long res = 1;

	BYTE Buf[1024];
	PSP_DEVICE_INTERFACE_DETAIL_DATA pspdidd = (PSP_DEVICE_INTERFACE_DETAIL_DATA)Buf;
	SP_DEVICE_INTERFACE_DATA spdid;
	SP_DEVINFO_DATA spdd;
	spdd.cbSize = sizeof(SP_DEVINFO_DATA);
	spdid.cbSize = sizeof(spdid);

	while (SetupDiEnumDeviceInfo(hDevInfo, dwIndex++, &spdd))
	{
		DeviceInfo info{ spdd.DevInst };
		DWORD propType = 0;
		WCHAR propBuffer[512];

		SetupDiGetDeviceRegistryProperty(hDevInfo, &spdd, SPDRP_PHYSICAL_DEVICE_OBJECT_NAME, &propType, (BYTE*)propBuffer, 512, NULL);
		info.device_name = propBuffer;
		SetupDiGetDeviceRegistryProperty(hDevInfo, &spdd, SPDRP_DEVICEDESC, &propType, (BYTE*)propBuffer, 512, NULL);
		info.device_description = propBuffer;

		result.push_back(info);
	}

	SetupDiDestroyDeviceInfoList(hDevInfo);


	return result;
}

bool eject(DeviceInfo& info)
{
	//std::wcout << "Starting ejection device: " << info.device_description << std::endl;

	DEVINST DevInst = info.device_instance;

	WCHAR VetoNameW[MAX_PATH];
	VetoNameW[0] = 0;
	bool bSuccess = false;

	DEVINST DevInstParent = 0;
	DWORD res = CM_Get_Parent(&DevInstParent, DevInst, 0);
	bool try_again = false;
	for (int tries = 1; tries <= eject_attempt_cap || try_again; tries++)
	{
		//std::cout << "\tAttempt " << tries << std::endl;

		PNP_VETO_TYPE VetoType = PNP_VetoTypeUnknown;
		VetoNameW[0] = 0;

		// Trying to eject device
		if (CM_Request_Device_EjectW(DevInst, &VetoType, VetoNameW, MAX_PATH, 0) == CR_SUCCESS)
		{
			return true;
		}

		if (CM_Request_Device_EjectW(DevInstParent, &VetoType, VetoNameW, MAX_PATH, 0))
		{
			try_again = !try_again;
		}
		else
		{
			try_again = false;
		}
	}

	return false;
}

void list_devices()
{

	std::cout << "USB device listing:" << std::endl;

	if (prevList.size() == 0)
	{
		std::cout << "No USB storage devices connected" << std::endl;
	}

	for (int i = 0; i < prevList.size(); i++)
	{
		std::wcout << i + 1 << ". " << prevList[i].device_description << " (" << prevList[i].device_name << ")" << std::endl;
	}
	std::cout << std::endl;
}

void scanner()
{
	while (!to_exit)
	{
		std::vector<DeviceInfo> currentList = get_devices();
		bool has_update = false;

		for (const auto& i : currentList)
		{
			if (std::find(prevList.begin(), prevList.end(), i) == prevList.end())
			{
				has_update = true;
				break;
			}
		}

		if (!has_update)
		{
			for (const auto& i : prevList)
			{
				if (std::find(currentList.begin(), currentList.end(), i) == currentList.end())
				{
					has_update = true;
					break;
				}
			}
		}

		prevList = currentList;

		if (has_update)
		{
			system("cls");
			list_devices();
			std::cout << ">";
		}

		Sleep(scan_delay);
	}
}

void controller::add_command(std::string name, const std::function<void()>& function)
{
	if (command_map.find(name) != command_map.end())
	{
		return;
	}
	command_map[name] = function;
}

bool controller::execute(const std::string& message)
{
	std::string command_name = message.substr(0, message.find(' '));
	if (command_map.find(command_name) == command_map.end())
	{
		return false;
	}
	command_map.at(command_name)();
	return true;
}

bool DeviceInfo::operator==(const DeviceInfo& other)
{
	return device_instance == other.device_instance;
}
