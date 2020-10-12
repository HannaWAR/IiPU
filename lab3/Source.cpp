#pragma comment(lib, "PowrProf.lib")
#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <conio.h>
#include <Windows.h>
#include <Powrprof.h>

bool to_exit = false;
bool to_hibernate = false;
bool to_sleep = false;

class battery_data
{
	std::string power_type;
	int battery_charge;
	std::string power_mode;
	std::chrono::steady_clock::time_point last_charge_time;
	size_t battery_time;
	bool has_update;
	bool was_charged;

public:
	battery_data();
	void update();
	bool was_updated();
	friend std::ostream& operator<<(std::ostream& out, battery_data& value);
};

void key_getter();

int main()
{
	battery_data data;
	std::thread thread{ key_getter };
	data.update();
	while (!to_exit)
	{
		if (to_sleep)
		{
			to_sleep = false;
			SetSuspendState(FALSE, FALSE, FALSE);
		}
		if (to_hibernate)
		{
			to_hibernate = false;
			SetSuspendState(TRUE, FALSE, FALSE);
		}
		data.update();
		if (data.was_updated())
		{
			system("cls");
			std::cout << data << std::endl;
		}
	}
	thread.join();
	return 0;
}

std::ostream& operator<<(std::ostream& out, battery_data& value)
{
	out << value.power_type << std::endl;
	if (value.battery_charge == -1)
	{
		out << "Battery charge unknown" << std::endl;
	}
	else
	{
		out << "Battery charge: " << value.battery_charge << "%" << std::endl;
	}
	out << value.power_mode << std::endl;
	if (value.power_type == "Offline" && value.was_charged)
	{
		out << "Time since last charge: " << value.battery_time / 3600 << "h " << (value.battery_time / 60) % 60 << "m " << value.battery_time % 60 << "s" << std::endl;
	}
	value.has_update = false;
	return out;
}

void key_getter()
{
	while (true)
	{
		switch (_getch())
		{
		case 'q':
			to_exit = true;
			return;
		case 'h':
			to_hibernate = true;
			break;
		case 's':
			to_sleep = true;
			break;
		}
	}
}

battery_data::battery_data()
{
	has_update = false;
	was_charged = false;
	last_charge_time = std::chrono::high_resolution_clock::now();
}

void battery_data::update()
{
	SYSTEM_POWER_STATUS batteryDescriptor;
	if (!GetSystemPowerStatus(&batteryDescriptor))
	{
		return;
	}

	std::string old_power_type = power_type;
	switch (batteryDescriptor.ACLineStatus)
	{
	case 0:
		power_type = "Offline";
		if (was_charged)
		{
			size_t old_time = battery_time;
			battery_time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - last_charge_time).count();
			if (old_time != battery_time)
			{
				has_update = true;
			}
		}
		break;
	case 1:
		power_type = "Online";
		was_charged = true;
		last_charge_time = std::chrono::high_resolution_clock::now();
		break;
	case 255:
		power_type = "Unknown status";
		break;
	}
	if (power_type != old_power_type)
	{
		has_update = true;
	}

	int old_battery_charge = battery_charge;
	if ((unsigned int)batteryDescriptor.BatteryLifePercent == 255)
	{
		battery_charge = -1;
	}
	else
	{
		battery_charge = batteryDescriptor.BatteryLifePercent;
	}
	if (battery_charge != old_battery_charge)
	{
		has_update = true;
	}

	std::string old_power_mode = power_mode;
	if (!batteryDescriptor.SystemStatusFlag)
	{
		power_mode = "Battery saver disabled";
	}
	else
	{
		power_mode = "Battery saver enabled";
	}
	if (power_mode != old_power_mode)
	{
		has_update = true;
	}
}

bool battery_data::was_updated()
{
	return has_update;
}
