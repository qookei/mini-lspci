#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <iomanip>
#include <charconv>
#include <algorithm>

#include <cstring>
#include <cmath>

#include <sys/types.h>
#include <dirent.h>

#include <unordered_set>
#include <unordered_map>

#include <unistd.h>

std::vector<std::string> fetch_devices(const char *sysfs_pci_path) {
	DIR *directory = opendir(sysfs_pci_path);
	dirent *entry;
	std::vector<std::string> devices{};

	if (directory) {
		while ((entry = readdir(directory)))
			if (entry->d_name[0] != '.')
				devices.push_back(entry->d_name);

		closedir(directory);
	} else {
		fprintf(stderr, "failed to open sysfs devices directory\n");
	}

	return devices;
}

uint16_t fetch_device_attribute(const char *sysfs_pci_path,
		const std::string &device,
		const std::string &attribute) {
	uint16_t value = 0;
	std::ifstream file{sysfs_pci_path + device + "/" + attribute};

	if (!file.good()) {
		fprintf(stderr, "failed to fetch attribute %s for device %s\n",
				attribute.c_str(), device.c_str());
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

	std::string vendor_string;
	std::string device_string;

	std::string subsystem_vendor_string;
	std::string subsystem_device_string;
};

pci_device fetch_device(const char *sysfs_pci_path, const std::string &device) {
	pci_device result{};

	sscanf(device.c_str(), "%4x:%2x:%2x.%1x", 
			&result.segment, &result.bus, &result.slot, &result.function);

	result.vendor = fetch_device_attribute(sysfs_pci_path, device, "vendor");
	result.device = fetch_device_attribute(sysfs_pci_path, device, "device");
	result.subsystem_vendor = fetch_device_attribute(sysfs_pci_path, device, "subsystem_vendor");
	result.subsystem_device = fetch_device_attribute(sysfs_pci_path, device, "subsystem_device");

	std::ostringstream tmp{};
	tmp << std::setw(4) << std::setfill('0') << std::hex << result.vendor;
	result.vendor_string = tmp.str();

	tmp.str("");
	tmp << std::setw(4) << std::setfill('0') << std::hex << result.device;
	result.device_string = tmp.str();

	tmp.str("");
	tmp << std::setw(4) << std::setfill('0') << std::hex << result.subsystem_vendor;
	result.subsystem_vendor_string = tmp.str();

	tmp.str("");
	tmp << std::setw(4) << std::setfill('0') << std::hex << result.subsystem_device;
	result.subsystem_device_string = tmp.str();

	return result;
}

struct vendor_info {
	uint32_t id;
	std::string name;
	size_t line;
};

void collect_vendors(std::ifstream &file,
		const std::unordered_set<uint16_t> &needed,
		std::unordered_map<uint16_t, vendor_info> &list) {
	std::string line;

	for(size_t i = 0; std::getline(file, line); i++) {
		if (!line.size() || !isdigit(line[0]))
			continue;

		uint16_t id = 0xFFFF;
		std::from_chars(line.data(), line.data() + 4, id, 16);

		if (needed.count(id)) {
			list.emplace(id, vendor_info{id, line.substr(6), i});
		}
	}
}

void collect_devices(std::ifstream &file,
		std::unordered_map<uint16_t, vendor_info> &vendors,
		std::unordered_map<uint32_t, pci_device> &devs) {
	std::string line;

	vendor_info *vendor = nullptr;
	uint32_t id = 0xFFFF;

	for(size_t i = 0; std::getline(file, line); i++) {
		if (!line.size() || line[0] == '#')
			continue;

		if (!vendor) {
			if (!line.size() || !isdigit(line[0]))
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

		if (devs.count(id | (vendor->id << 16))) {
			auto &dev = devs[id | (vendor->id << 16)];
			dev.vendor_string = vendor->name;
			dev.device_string = line.substr(7);
		}
	}

	for (auto &[_, dev] : devs) {
		if (vendors.count(dev.subsystem_vendor))
			dev.subsystem_vendor_string = vendors[dev.subsystem_vendor].name;
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
			printf("%02x:%02x.%1x: %s %s\n",
				dev.bus, dev.slot, dev.function,
				dev.vendor_string.c_str(),
				dev.device_string.c_str());
			break;

		case numeric::mixed:
			printf("%02x:%02x.%1x: %s %s [%04x:%04x]\n",
				dev.bus, dev.slot, dev.function,
				dev.vendor_string.c_str(),
				dev.device_string.c_str(),
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
				printf("\tSubsystem: %s Device %s\n\n",
					dev.subsystem_vendor_string.c_str(),
					dev.subsystem_device_string.c_str());
				break;

			case numeric::mixed:
				printf("\tSubsystem: %s Device %s [%04x:%04x]\n\n",
					dev.subsystem_vendor_string.c_str(),
					dev.subsystem_device_string.c_str(),
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
	printf("mini-lspci v1.0\n");
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
	for (auto &dev : fetch_devices(sysfs_pci_path)) {
		auto d = fetch_device(sysfs_pci_path, dev);
		devices.emplace(static_cast<uint32_t>(d.device)
				| static_cast<uint32_t>(d.vendor << 16),
				d);
	}

	std::unordered_set<uint16_t> needed_vendors;
	for (auto &[_, dev] : devices) {
		if (dev.subsystem_vendor != 0xFFFF)
			needed_vendors.insert(dev.subsystem_vendor);
		needed_vendors.insert(dev.vendor);
	}

	std::unordered_map<uint16_t, vendor_info> vendors;
	vendors.reserve(needed_vendors.size());

	std::ifstream file{pci_ids_path};

	if (file.good()) {
		collect_vendors(file, needed_vendors, vendors);

		file.clear();
		file.seekg(0);

		collect_devices(file, vendors, devices);
	}

	for (auto &[_, dev] : devices) {
		print_device(dev, verbose, static_cast<numeric>(numeric_level));
	}
}
