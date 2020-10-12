#include <iostream>
#include <string>
#include <array>
#include <Windows.h>
#include <ntddscsi.h>

std::array<std::string, BusTypeMax> bus_types = {
	"UNKNOWN",
	"SCSI",
	"ATAPI",
	"ATA",
	"IEEE 1394",
	"SSA",
	"FIBRE",
	"USB",
	"RAID",
	"iSCSI",
	"SAS",
	"SATA",
	"SD",
	"MMC",
	"",
	""
	"STORAGE SPACE"
};

class device
{
private:

	HANDLE file;
	size_t taken_space;
	size_t empty_space;
	size_t total_space;
	bool inited_space;

	std::string product_ID;
	std::string revision;
	std::string bus_type;
	std::string serial_number;
	std::string vendor_ID;
	bool inited_description;

	std::string firmware_version;
	bool inited_firmware_version;

	std::string supported_DMA;
	std::string supported_uDMA;
	std::string supported_PIO;
	std::string supported_ATA;
	bool inited_supported;
public:
	device(const std::string& name);

	size_t get_taken_space();
	size_t get_total_space();
	size_t get_empty_space();

	std::string get_product_ID();
	std::string get_product_revision();
	std::string get_bus_type();
	std::string get_serial_number();
	std::string get_vendor_ID();

	std::string get_firmware_version();

	std::string get_supported_DMA();
	std::string get_supported_uDMA();
	std::string get_supported_PIO();
	std::string get_supported_ATA();

	void update_space();


	~device();
private:
	void init_space();
	void init_description();
	void init_firmware_version();
	void init_supported();

	std::string get_numbers(int count, int offset = 0);
	int get_max_number(int mask, int max);

};

std::ostream& operator<<(std::ostream& out, device& value);

int main() {

	device drive0{ "//./PhysicalDrive0" };

	std::cout << drive0;

	return 0;
}

device::device(const std::string& name)
{
	inited_space = false;
	inited_description = false;
	inited_firmware_version = false;
	file = CreateFileA(name.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, NULL, NULL);
	if (file == INVALID_HANDLE_VALUE)
	{
		throw GetLastError();
	}
}

size_t device::get_taken_space()
{
	if (!inited_space)
	{
		init_space();
	}
	return taken_space;
}

size_t device::get_total_space()
{
	if (!inited_space)
	{
		init_space();
	}
	return total_space;
}

size_t device::get_empty_space()
{
	if (!inited_space)
	{
		init_space();
	}
	return empty_space;
}

std::string device::get_product_ID()
{
	if (!inited_description)
	{
		init_description();
	}
	return product_ID;
}

std::string device::get_product_revision()
{
	if (!inited_description)
	{
		init_description();
	}
	return revision;
}

std::string device::get_bus_type()
{
	if (!inited_description)
	{
		init_description();
	}
	return bus_type;
}

std::string device::get_vendor_ID()
{
	if (!inited_description)
	{
		init_description();
	}
	return vendor_ID;
}

std::string device::get_serial_number()
{
	if (!inited_description)
	{
		init_description();
	}
	return serial_number;
}

std::string device::get_firmware_version()
{
	if (!inited_firmware_version)
	{
		init_firmware_version();
	}
	return firmware_version;
}

std::string device::get_supported_DMA()
{
	if (!inited_supported)
	{
		init_supported();
	}
	return supported_DMA;

}

std::string device::get_supported_uDMA()
{
	if (!inited_supported)
	{
		init_supported();
	}
	return supported_uDMA;
}

std::string device::get_supported_PIO()
{
	if (!inited_supported)
	{
		init_supported();
	}
	return supported_PIO;
}

std::string device::get_supported_ATA()
{
	if (!inited_supported)
	{
		init_supported();
	}
	return supported_ATA;
}

void device::update_space()
{
	inited_space = false;
}

device::~device()
{
	if (file != INVALID_HANDLE_VALUE)
	{
		CloseHandle(file);
		file = INVALID_HANDLE_VALUE;
	}
}

void device::init_space()
{
	_ULARGE_INTEGER global_total_space;
	_ULARGE_INTEGER global_free_space;
	global_total_space.QuadPart = 0;
	global_free_space.QuadPart = 0;

	DWORD drives_mask = GetLogicalDrives();

	for (char var = 0; var < 'Z' - 'A'; var++) {
		if (drives_mask & (1 << var)) {
			std::string path = "?:\\";
			path[0] = var + 'A';

			_ULARGE_INTEGER total_space;
			_ULARGE_INTEGER free_space;

			GetDiskFreeSpaceExW(std::wstring(path.begin(), path.end()).c_str(), 0, &total_space, &free_space);

			if (GetDriveTypeA(path.c_str()) == DRIVE_FIXED) {
				global_total_space.QuadPart += total_space.QuadPart;
				global_free_space.QuadPart += free_space.QuadPart;
			}
		}
	}

	empty_space = global_free_space.QuadPart / (1024 * 1024 * 1024);
	total_space = global_total_space.QuadPart / (1024 * 1024 * 1024);
	taken_space = total_space - empty_space;
	inited_space = true;
}

void device::init_description()
{
	STORAGE_PROPERTY_QUERY storageProtertyQuery;
	storageProtertyQuery.QueryType = PropertyStandardQuery;
	storageProtertyQuery.PropertyId = StorageDeviceProperty;

	STORAGE_DEVICE_DESCRIPTOR* descriptor = reinterpret_cast<STORAGE_DEVICE_DESCRIPTOR*>(new byte[1024]);
	descriptor->Size = 1024;

	if (!DeviceIoControl(file, IOCTL_STORAGE_QUERY_PROPERTY, &storageProtertyQuery, sizeof(storageProtertyQuery), descriptor, 1024, NULL, 0))
	{
		throw GetLastError();
	}

	product_ID = reinterpret_cast<char*>(descriptor) + descriptor->ProductIdOffset;
	revision = reinterpret_cast<char*>(descriptor) + descriptor->ProductRevisionOffset;
	bus_type = bus_types[descriptor->BusType];
	serial_number = reinterpret_cast<char*>(descriptor) + descriptor->SerialNumberOffset + 5;

	if (descriptor->VendorIdOffset)
	{
		vendor_ID = reinterpret_cast<char*>(descriptor) + descriptor->VendorIdOffset;
	}
	else
	{
		vendor_ID = "UNKNOWN";
	}
	inited_description = true;
}

void device::init_firmware_version()
{
	char buf[1024];
	if (!DeviceIoControl(file, IOCTL_STORAGE_FIRMWARE_GET_INFO, buf, sizeof(buf), buf, sizeof(buf), NULL, NULL))
	{
		firmware_version = "UNKNOWN";
	}
	else
	{
		firmware_version = buf + 0x28;
	}
}

void device::init_supported()
{
	UCHAR identifyDataBuffer[512 + sizeof(ATA_PASS_THROUGH_EX)];

	ATA_PASS_THROUGH_EX& PTE = *(ATA_PASS_THROUGH_EX*)identifyDataBuffer;
	PTE.Length = sizeof(PTE);
	PTE.AtaFlags = ATA_FLAGS_DATA_IN;
	PTE.DataTransferLength = 512;
	PTE.TimeOutValue = 10;
	PTE.DataBufferOffset = sizeof(ATA_PASS_THROUGH_EX);

	IDEREGS* ideRegs = (IDEREGS*)PTE.CurrentTaskFile;
	ideRegs->bCommandReg = 0xEC;

	if (!DeviceIoControl(file, IOCTL_ATA_PASS_THROUGH, &PTE, sizeof(identifyDataBuffer), &PTE, sizeof(identifyDataBuffer), NULL, NULL)) {
		throw GetLastError();
	}

	WORD* data = (WORD*)(identifyDataBuffer + sizeof(ATA_PASS_THROUGH_EX));

	supported_DMA = get_numbers(get_max_number(data[63], 0x4));

	supported_uDMA = get_numbers(get_max_number(data[88], 0x40));

	supported_PIO = get_numbers(get_max_number(data[64], 0x2), 3);

	supported_ATA.clear();
	for (int i = 3; i < 8; ++i)
	{
		if (data[80] & (1 << i))
		{
			supported_ATA += std::to_string(i + 1) + ' ';
		}
	}
}

std::string device::get_numbers(int count, int offset)
{
	std::string result;
	if (!count)
	{
		result = "NONE";
		return result;
	}
	for (int i = 0; i < count + offset; ++i)
	{
		result += std::to_string(i) + ' ';
	}
	return result;
}

int device::get_max_number(int mask, int max)
{
	int result = 0;
	int ind = 0;
	for (int i = 1; i <= max; i <<= 1)
	{
		++ind;
		if (mask & i)
		{
			result = ind;
		}
	}
	return result;
}

std::ostream& operator<<(std::ostream& out, device& value)
{
	out << "Product ID:          " << value.get_product_ID() << std::endl;			// OK
	out << "Revision:            " << value.get_product_revision() << std::endl;	// OK
	out << "Vendor ID:           " << value.get_vendor_ID() << std::endl;			// OK
	out << "Serial Number:       " << value.get_serial_number() << std::endl;		// OK
	out << "Firmware version:    " << value.get_firmware_version() << std::endl;	// OK
	out << "Storage data: " << std::endl;										// OK
	out << "\tTotal space: " << value.get_total_space() << " GiB" << std::endl;	// OK
	out << "\tTaken space: " << value.get_taken_space() << " GiB" << std::endl;	// OK
	out << "\tEmpty space: " << value.get_empty_space() << " GiB" << std::endl;	// OK
	out << "Bus type:            " << value.get_bus_type() << std::endl;			// Ok
	out << "Supported modes:" << std::endl;
	out << "\tDMA:         " << value.get_supported_DMA() << std::endl;
	out << "\tuDMA:        " << value.get_supported_uDMA() << std::endl;
	out << "\tPIO:         " << value.get_supported_PIO() << std::endl;
	out << "\tATA:         " << value.get_supported_ATA() << std::endl;
	return out;
}
