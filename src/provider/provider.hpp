#pragma once

#include <vector>
#include <system_error>

struct pci_device {
	bool has_location = false;
	unsigned int segment = 0;
	unsigned int bus = 0;
	unsigned int slot = 0;
	unsigned int function = 0;

	uint16_t vendor = 0xFFFF;
	uint16_t device = 0xFFFF;

	bool has_rev = false;
	uint8_t rev = 0;

	bool has_subsystem = false;
	uint16_t subsystem_vendor = 0xFFFF;
	uint16_t subsystem_device = 0xFFFF;

	bool has_class_code = false;
	uint16_t class_code = 0;
};

struct provider {
	virtual ~provider() = default;

	virtual std::vector<pci_device> fetch_devices(std::error_code &ec) = 0;
	virtual bool has_nonzero_seg() = 0;

	std::vector<pci_device> fetch_devices_sorted(std::error_code &ec) {
		auto devices = fetch_devices(ec);

		std::sort(devices.begin(), devices.end(),
				[] (const auto &a, const auto &b) {
			if (a.segment != b.segment)
				return a.segment < b.segment;
			if (a.bus != b.bus)
				return a.bus < b.bus;
			if (a.slot != b.slot)
				return a.slot < b.slot;
			if (a.function != b.function)
				return a.function < b.function;

			return false;
		});

		return devices;
	}
};
