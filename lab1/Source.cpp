#pragma comment(lib, "setupapi.lib")
#include <Windows.h>
#include <setupapi.h>
#include <iostream>

const size_t WCHAR_SIZE = sizeof(TCHAR) / sizeof(char);

class pci_dev_data
{
private:
	HDEVINFO dev_info = INVALID_HANDLE_VALUE;
	SP_DEVINFO_DATA dev_data;
	long long current_index = -1;
	bool is_associated = false;
public:
	pci_dev_data();

	bool associate(DWORD index);
	std::wstring get_name();
	std::wstring get_vendorID();
	std::wstring get_deviceID();

	~pci_dev_data();
};

int main()
{
	setlocale(LC_ALL, "Russian");

	pci_dev_data source;
	DWORD i = 0;
	while (source.associate(i++))
	{
		std::wcout << "device " << i << ":" << std::endl;
		std::wcout << "\tname:     " << source.get_name() << std::endl;
		std::wcout << "\tvendorID: " << source.get_vendorID() << std::endl;
		std::wcout << "\tdeviceID: " << source.get_deviceID() << std::endl;
	}

	system("pause");

	return 0;
}

pci_dev_data::pci_dev_data()
{
	dev_info = SetupDiGetClassDevsW(nullptr, TEXT("PCI"), nullptr, DIGCF_PRESENT | DIGCF_ALLCLASSES);
	if (dev_info == INVALID_HANDLE_VALUE)
	{
		throw std::exception{};
	}
}

bool pci_dev_data::associate(DWORD index)
{
	dev_data.cbSize = sizeof(SP_DEVINFO_DATA);
	return is_associated = SetupDiEnumDeviceInfo(dev_info, index, &dev_data);
}

std::wstring pci_dev_data::get_name()
{
	if (!is_associated)
	{
		throw std::exception{};
	}

	TCHAR buffer[1024];
	if (!SetupDiGetDeviceRegistryPropertyW(dev_info, &dev_data, SPDRP_DEVICEDESC, nullptr, (BYTE*)buffer, 1024 * WCHAR_SIZE, nullptr))
	{
		throw std::exception{};
	}

	return std::wstring{ buffer };
}

std::wstring pci_dev_data::get_vendorID()
{
	if (!is_associated)
	{
		throw std::exception{};
	}

	TCHAR buffer[1024];
	if (!SetupDiGetDeviceRegistryPropertyW(dev_info, &dev_data, SPDRP_HARDWAREID, nullptr, (BYTE*)buffer, 1024 * WCHAR_SIZE, nullptr))
	{
		throw std::exception{};
	}
	return std::wstring{ buffer }.substr(std::wstring{ buffer }.find(L"VEN_") + 4, 4);
}

std::wstring pci_dev_data::get_deviceID()
{
	if (!is_associated)
	{
		throw std::exception{};
	}

	TCHAR buffer[1024];
	if (!SetupDiGetDeviceRegistryPropertyW(dev_info, &dev_data, SPDRP_HARDWAREID, nullptr, (BYTE*)buffer, 1024 * WCHAR_SIZE, nullptr))
	{
		throw std::exception{};
	}
	return std::wstring{ buffer }.substr(std::wstring{ buffer }.find(L"DEV_") + 4, 4);
}

pci_dev_data::~pci_dev_data()
{
	if (dev_info != INVALID_HANDLE_VALUE)
	{
		SetupDiDestroyDeviceInfoList(dev_info);
		dev_info = INVALID_HANDLE_VALUE;
	}
}
