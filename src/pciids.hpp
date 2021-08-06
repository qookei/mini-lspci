#pragma once

#include <util/mapped_file.hpp>

#include <unordered_map>
#include <unordered_set>
#include <string_view>
#include <cassert>

struct pciids_parser {
	pciids_parser(const char *path)
	: file_{path} { }

	void internalize_device(uint16_t vendor_id, uint16_t device_id,
			uint16_t sub_vendor_id, uint16_t sub_device_id);

	void process();

private:
	mapped_file file_;

	struct device_info {
		std::string_view name;
		std::unordered_map<uint32_t, std::string_view> sub_devices;
	};

	struct vendor_info {
		std::string_view name;
		std::unordered_map<uint16_t, device_info> devices;
	};

	std::unordered_map<uint16_t, vendor_info> vendors_;

	static uint32_t make_key_(uint32_t vendor, uint32_t device) {
		return (device | (vendor << 16));
	}

public:
	struct name_pair {
		uint16_t vendor_id;
		uint16_t device_id;

		bool vendor_known = false;
		bool device_known = false;
		std::string_view vendor_name{};
		std::string_view device_name{};
	};

	struct device {
		name_pair main;
		name_pair sub;
	};

	device get_device(uint16_t vendor_id, uint16_t device_id,
			uint16_t sub_vendor_id, uint16_t sub_device_id) const {
		name_pair main{vendor_id, device_id};
		name_pair sub{sub_vendor_id, sub_device_id};

		if (auto it = vendors_.find(sub_vendor_id); it != vendors_.end() && it->second.name.size()) {
			sub.vendor_known = true;
			sub.vendor_name = it->second.name;
		}

		if (auto it = vendors_.find(vendor_id); it != vendors_.end() && it->second.name.size()) {
			auto &[_, vendor] = *it;

			main.vendor_known = true;
			main.vendor_name = vendor.name;

			if (auto it = vendor.devices.find(device_id); it != vendor.devices.end() && it->second.name.size()) {
				auto &[_, device] = *it;

				main.device_known = true;
				main.device_name = device.name;

				if (auto it = device.sub_devices.find(make_key_(sub_vendor_id, sub_device_id)); it != device.sub_devices.end() && it->second.size()) {
					auto &[_, sub_device] = *it;

					sub.device_known = true;
					sub.device_name = sub_device;
				}
			}
		}

		return device{main, sub};
	}
};
