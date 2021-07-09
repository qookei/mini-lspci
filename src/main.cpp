#include <fmt/core.h>

#include <vector>
#include <string>
#include <charconv>
#include <algorithm>
#include <string_view>
#include <unordered_set>
#include <unordered_map>

#include <cstring>
#include <cmath>

#include <memory>

#include <util/mapped_file.hpp>
#include <util/line_range.hpp>

#include <provider/picker.hpp>

#include <config.hpp>

struct vendor_info {
	uint32_t id;
	std::string_view name;
	size_t line;
};

struct device_info {
	std::string_view name;

	std::unordered_map<uint32_t, std::string_view> subsystem_names;
};

void collect_vendors(const line_range &lines,
		const std::unordered_set<uint16_t> &needed,
		std::unordered_map<uint16_t, vendor_info> &list) {

	for (auto [n, line] : lines) {
		if (!line.size() || !isxdigit(line[0]))
			continue;

		uint16_t id = 0xFFFF;
		std::from_chars(line.data(), line.data() + 4, id, 16);

		if (needed.contains(id)) {
			list.emplace(id, vendor_info{id, line.substr(6), n});
		}
	}
}

void collect_devices(const line_range &lines,
		std::unordered_map<uint16_t, vendor_info> &vendor_infos,
		std::unordered_map<uint32_t, device_info> &device_infos) {
	std::string line;

	vendor_info *vendor = nullptr;
	uint32_t id = 0xFFFF;

	for (auto [i, line] : lines) {
		if (!line.size() || line[0] == '#')
			continue;

		if (!vendor) {
			if (!isxdigit(line[0]))
				continue;

			for (auto &[_, v] : vendor_infos) {
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

		if (line[1] == '\t' && device_infos.count(id | (vendor->id << 16))) {
			auto &dev = device_infos[id | (vendor->id << 16)];

			uint32_t subsystem_vendor_id = 0xFFFF;
			uint32_t subsystem_device_id = 0xFFFF;
			std::from_chars(line.data() + 2, line.data() + 6, subsystem_vendor_id, 16);
			std::from_chars(line.data() + 7, line.data() + 11, subsystem_device_id, 16);

			auto it = dev.subsystem_names.find(subsystem_device_id | (subsystem_vendor_id << 16));

			if (it != dev.subsystem_names.end())
				it->second = line.substr(13);

			continue;
		}

		std::from_chars(line.data() + 1, line.data() + 5, id, 16);

		if (device_infos.contains(id | (vendor->id << 16)))
			device_infos[id | (vendor->id << 16)].name = line.substr(7);
	}
}

enum class numeric : int {
	no = 0,
	yes,
	mixed
};

void print_device(pci_device &dev,
		std::unordered_map<uint16_t, vendor_info> &vendor_infos,
		std::unordered_map<uint32_t, device_info> &device_infos,
		bool verbose, numeric mode) {

	auto ven_it = vendor_infos.find(dev.vendor);
	auto dev_it = device_infos.find(dev.device | (dev.vendor << 16));

	bool vendor_known = ven_it != vendor_infos.end() && ven_it->second.name.size();
	bool device_known = dev_it != device_infos.end() && dev_it->second.name.size();

	if (dev.has_location)
		fmt::print("{:02x}:{:02x}.{:1x}: ", dev.bus, dev.slot, dev.function);

	if (mode == numeric::yes) {
		fmt::print("{:04x}:{:04x}", dev.vendor, dev.device);
	} else {
		if (vendor_known) {
			auto &vendor = ven_it->second;

			fmt::print("{} ", vendor.name);
		}

		if (device_known) {
			auto &device = dev_it->second;

			fmt::print("{}", device.name);
		} else {
			fmt::print("Device");
		}

		if (mode == numeric::mixed) {
			fmt::print(" [{:04x}:{:04x}]", dev.vendor, dev.device);
		} else if (vendor_known && !device_known) {
			fmt::print(" {:04x}", dev.device);
		} else if (!vendor_known && !device_known) {
			fmt::print(" {:04x}:{:04x}", dev.vendor, dev.device);
		}
	}

	if (dev.has_rev && dev.rev) {
		fmt::print(" (rev {:02x})", dev.rev);
	}

	fmt::print("\n");

	if (verbose) {
		if (dev.has_subsystem && dev.subsystem_vendor != 0xFFFF) {
			fmt::print("\tSubsystem: ");

			if (mode == numeric::yes) {
				fmt::print("{:04x}:{:04x}", dev.subsystem_vendor, dev.subsystem_device);
			} else {
				auto sub_ven_it = vendor_infos.find(dev.subsystem_vendor);
				bool sub_vendor_known = sub_ven_it != vendor_infos.end() && sub_ven_it->second.name.size();

				bool sub_device_known = false;
				std::string_view sub_device{};

				if (sub_vendor_known) {
					auto &sub_vendor = sub_ven_it->second;

					fmt::print("{} ", sub_vendor.name);
				}

				if (device_known) {
					auto &device = dev_it->second;

					auto sub_dev_it = device.subsystem_names.find(dev.device | (dev.vendor << 16));
					sub_device_known = sub_dev_it != device.subsystem_names.end() && sub_dev_it->second.size();

					if (sub_device_known)
						sub_device = sub_dev_it->second;
				}

				if (sub_device_known) {
					fmt::print("{}", sub_device);
				} else {
					fmt::print("Device");
				}

				if (mode == numeric::mixed) {
					fmt::print(" [{:04x}:{:04x}]", dev.subsystem_vendor, dev.subsystem_device);
				} else if (sub_vendor_known && !sub_device_known) {
					fmt::print(" {:04x}", dev.subsystem_device);
				} else if (!sub_vendor_known && !sub_device_known) {
					fmt::print(" {:04x}:{:04x}", dev.subsystem_vendor, dev.subsystem_device);
				}
			}
		}

		fmt::print("\n\n");
	}
}

void print_version() {
	fmt::print("mini-lspci v{}\n", version);
}

void print_help() {
	print_version();

	fmt::print("\nusage:\n");
	fmt::print("\t-v             Be verbose\n");
	fmt::print("\t-n             Only show numeric values\n");
	fmt::print("\t-nn            Show numeric values and names\n");
	fmt::print("\t-p <path>      Set path to pci.ids file (default: {})\n", "/usr/share/hwdata/pci.ids");
	fmt::print("\t-P <provoder>  Select provider to use (default: {})\n", default_provider);
	fmt::print("\t-V             Show version\n");
	fmt::print("\t-h             Show usage\n");
}

int main(int argc, char **argv) {
	const char *pci_ids_path = "/usr/share/hwdata/pci.ids";
	std::string_view wanted_provider = default_provider;

	bool verbose = false;
	int numeric_level = 0;

	int i = 0;
	while ((i = getopt(argc, argv, "vnhVs:p:P:")) != -1) {
		switch (i) {
			case 'v':
				verbose = true;
				break;
			case 'n':
				numeric_level = std::min(numeric_level + 1, 2);
				break;
			case 'p':
				pci_ids_path = optarg;
				break;
			case 'P':
				wanted_provider = optarg;
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

	auto prov = get_provider(wanted_provider);

	if (!prov) {
		fmt::print(stderr, "Invalid provider '{}' selected\n", wanted_provider);
		return 1;
	}

	std::error_code ec;

	auto devices = prov->fetch_devices_sorted(ec);

	if (ec) {
		fmt::print(stderr, "Failed to fetch device: {}\n", ec.message());
		return 1;
	}

	std::unordered_set<uint16_t> needed_vendors;
	for (auto &dev : devices) {
		if (dev.subsystem_vendor != 0xFFFF)
			needed_vendors.insert(dev.subsystem_vendor);
		needed_vendors.insert(dev.vendor);
	}

	std::unordered_map<uint16_t, vendor_info> vendor_infos;
	vendor_infos.reserve(needed_vendors.size());

	std::unordered_map<uint32_t, device_info> device_infos;
	device_infos.reserve(devices.size());

	for (auto &dev : devices) {
		device_infos.try_emplace(dev.device | (dev.vendor << 16), device_info{});

		if (dev.subsystem_vendor != 0xFFFF)
			device_infos[dev.device | (dev.vendor << 16)].subsystem_names.try_emplace(dev.subsystem_device | (dev.subsystem_vendor << 16), std::string_view{});
	}

	mapped_file file{pci_ids_path};
	line_range lines{file.as_string_view()};

	collect_vendors(lines, needed_vendors, vendor_infos);
	collect_devices(lines, vendor_infos, device_infos);

	for (auto &dev : devices) {
		print_device(dev, vendor_infos, device_infos, verbose, static_cast<numeric>(numeric_level));
	}
}
