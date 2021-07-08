#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <iomanip>
#include <charconv>
#include <algorithm>
#include <string_view>
#include <unordered_set>
#include <unordered_map>
#include <filesystem>

#include <cstring>
#include <cmath>

#include <sys/types.h>
#include <unistd.h>

#include <util/mapped_file.hpp>
#include <util/line_range.hpp>

namespace fs = std::filesystem;

uint16_t fetch_device_attribute(const fs::path &attr_path) {
	uint16_t value = 0;
	std::ifstream file{attr_path};

	if (!file.good()) {
		fprintf(stderr, "failed to fetch attribute %s for device %s\n",
				attr_path.filename().c_str(), attr_path.parent_path().filename().c_str());
		return 0xFFFF;
	}

	file >> std::hex >> value;

	return value;
}

struct pci_device {
	unsigned int segment;
	unsigned int bus;
	unsigned int slot;
	unsigned int function;

	uint16_t vendor;
	uint16_t device;
	uint16_t subsystem_vendor;
	uint16_t subsystem_device;

	std::string_view vendor_string;
	std::string_view device_string;

	std::string_view subsystem_vendor_string;
	std::string_view subsystem_device_string;
};

pci_device fetch_device(const fs::path &device_path) {
	pci_device result{};

	sscanf(device_path.filename().c_str(), "%4x:%2x:%2x.%1x",
			&result.segment, &result.bus, &result.slot, &result.function);

	result.vendor = fetch_device_attribute(device_path / "vendor");
	result.device = fetch_device_attribute(device_path / "device");
	result.subsystem_vendor = fetch_device_attribute(device_path / "subsystem_vendor");
	result.subsystem_device = fetch_device_attribute(device_path / "subsystem_device");

	return result;
}

struct vendor_info {
	uint32_t id;
	std::string_view name;
	size_t line;
};

struct device_info {
	uint16_t id;
	std::string_view name;
};

void collect_vendors(const line_range &lines,
		const std::unordered_set<uint16_t> &needed,
		std::unordered_map<uint16_t, vendor_info> &list) {

	for (auto [n, line] : lines) {
		if (!line.size() || !isxdigit(line[0]))
			continue;

		uint16_t id = 0xFFFF;
		std::from_chars(line.data(), line.data() + 4, id, 16);

		if (needed.count(id)) {
			list.emplace(id, vendor_info{id, line.substr(6), n});
		}
	}
}

void collect_devices(const line_range &lines,
		std::unordered_map<uint16_t, vendor_info> &vendors,
		std::unordered_map<uint32_t, pci_device> &devs) {
	std::string line;

	vendor_info *vendor = nullptr;
	uint32_t id = 0xFFFF;

	for (auto [i, line] : lines) {
		if (!line.size() || line[0] == '#')
			continue;

		if (!vendor) {
			if (!isxdigit(line[0]))
				continue;

			for (auto &[_, v] : vendors) {
				if (i == v.line) {
					vendor = &v;
					break;
				}
			}

			continue;
		}

		if (line[0] != '\t') {
			vendor = nullptr;
			continue;
		}

		if (line[1] == '\t' && devs.count(id | (vendor->id << 16))) {
			auto &dev = devs[id | (vendor->id << 16)];

			uint16_t subsystem_vendor_id = 0xFFFF;
			uint16_t subsystem_device_id = 0xFFFF;
			std::from_chars(line.data() + 2, line.data() + 6, subsystem_vendor_id, 16);
			std::from_chars(line.data() + 7, line.data() + 11, subsystem_device_id, 16);

			if (dev.subsystem_vendor == subsystem_vendor_id
					&& dev.subsystem_device == subsystem_device_id) {
				dev.subsystem_device_string = line.substr(13);

			}

			continue;
		}

		std::from_chars(line.data() + 1, line.data() + 5, id, 16);

		if (devs.count(id | (vendor->id << 16)))
			devs[id | (vendor->id << 16)].device_string = line.substr(7);
	}

	for (auto &[_, dev] : devs) {
		if (vendors.count(dev.subsystem_vendor))
			dev.subsystem_vendor_string = vendors[dev.subsystem_vendor].name;
		if (vendors.count(dev.vendor))
			dev.vendor_string = vendors[dev.vendor].name;
	}
}

enum class numeric : int {
	no = 0,
	yes,
	mixed
};

void print_device(pci_device &dev, bool verbose, numeric mode) {
	switch (mode) {
		case numeric::no:
			printf("%02x:%02x.%1x: %.*s %.*s\n",
				dev.bus, dev.slot, dev.function,
				static_cast<int>(dev.vendor_string.size()),
				dev.vendor_string.data(),
				static_cast<int>(dev.device_string.size()),
				dev.device_string.data());
			break;

		case numeric::mixed:
			printf("%02x:%02x.%1x: %.*s %.*s [%04x:%04x]\n",
				dev.bus, dev.slot, dev.function,
				static_cast<int>(dev.vendor_string.size()),
				dev.vendor_string.data(),
				static_cast<int>(dev.device_string.size()),
				dev.device_string.data(),
				dev.vendor, dev.device);
			break;
		case numeric::yes:
			printf("%02x:%02x.%1x: %04x:%04x\n",
				dev.bus, dev.slot, dev.function,
				dev.vendor, dev.device);
			break;
	}

	if (verbose) {
		switch (mode) {
			case numeric::no:
				printf("\tSubsystem: %.*s Device %.*s\n\n",
					static_cast<int>(dev.subsystem_vendor_string.size()),
					dev.subsystem_vendor_string.data(),
					static_cast<int>(dev.subsystem_device_string.size()),
					dev.subsystem_device_string.data());
				break;

			case numeric::mixed:
				printf("\tSubsystem: %.*s Device %.*s [%04x:%04x]\n\n",
					static_cast<int>(dev.subsystem_vendor_string.size()),
					dev.subsystem_vendor_string.data(),
					static_cast<int>(dev.subsystem_device_string.size()),
					dev.subsystem_device_string.data(),
					dev.subsystem_vendor, dev.subsystem_device);
				break;
			case numeric::yes:
				printf("\tSubsystem: %04x:%04x\n\n",
					dev.subsystem_vendor, dev.subsystem_device);
				break;
		}
	}
}

void print_version() {
	printf("mini-lspci v1.1\n");
}

void print_help() {
	print_version();

	printf("\nusage:\n");
	printf("\t-v               be verbose (show subsystem information)\n");
	printf("\t-n               show only numeric values\n");
	printf("\t-nn              show numeric values and names\n");
	printf("\t-s <path>        set sysfs pci/devices path (default: /sys/bus/pci/devices/)\n");
	printf("\t-p <path>        set pci.ids path (default: /usr/share/hwdata/pci.ids)\n");
	printf("\t-V               show version\n");
	printf("\t-h               show help (you are here)\n");
}

int main(int argc, char **argv) {
	const char *sysfs_pci_path = "/sys/bus/pci/devices/";
	const char *pci_ids_path = "/usr/share/hwdata/pci.ids";

	bool verbose = false;
	int numeric_level = 0;

	int i = 0;
	while ((i = getopt(argc, argv, "vnhVs:p:")) != -1) {
		switch (i) {
			case 'v':
				verbose = true;
				break;
			case 'n':
				numeric_level = std::min(numeric_level + 1, 2);
				break;
			case 's':
				sysfs_pci_path = optarg;
				break;
			case 'p':
				pci_ids_path = optarg;
				break;
			case 'h':
				print_help();
				return 0;
			case 'V':
				print_version();
				return 0;
			case '?':
				return 1;
		}
	}

	std::unordered_map<uint32_t, pci_device> devices;

	std::error_code ec;

	for (auto &ent : fs::directory_iterator{sysfs_pci_path, ec}) {
		auto d = fetch_device(ent.path());
		devices.emplace(static_cast<uint32_t>(d.device)
				| static_cast<uint32_t>(d.vendor << 16), d);
	}

	if (ec) {
		std::cerr << "Failed to enumerate sysfs directory: " << ec.message() << std::endl;
		return 1;
	}

	std::unordered_set<uint16_t> needed_vendors;
	for (auto &[_, dev] : devices) {
		if (dev.subsystem_vendor != 0xFFFF)
			needed_vendors.insert(dev.subsystem_vendor);
		needed_vendors.insert(dev.vendor);
	}

	std::unordered_map<uint16_t, vendor_info> vendors;
	vendors.reserve(needed_vendors.size());

	mapped_file file{pci_ids_path};
	line_range lines{file.as_string_view()};

	collect_vendors(lines, needed_vendors, vendors);
	collect_devices(lines, vendors, devices);

	for (auto &[_, dev] : devices) {
		print_device(dev, verbose, static_cast<numeric>(numeric_level));
	}
}

