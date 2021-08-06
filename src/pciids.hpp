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
		std::string_view name_;
		std::unordered_map<uint32_t, std::string_view> sub_devices_;

		const std::string_view *lookup_sub(uint16_t vendor_id,
				uint16_t device_id) const {
			if (auto it = sub_devices_.find(device_id | (uint32_t{vendor_id} << 16));
					it != sub_devices_.end()
					&& it->second.size()) {
				return &it->second;
			}

			return nullptr;
		}

	};

	struct vendor_info {
		std::string_view name_;
		std::unordered_map<uint16_t, device_info> devices_;

		const device_info *lookup_device(uint16_t id) const {
			if (auto it = devices_.find(id); it != devices_.end()
					&& it->second.name_.size()) {
				return &it->second;
			}

			return nullptr;
		}
	};

	std::unordered_map<uint16_t, vendor_info> vendors_;

	const vendor_info *lookup_vendor(uint16_t id) const {
		if (auto it = vendors_.find(id); it != vendors_.end() && it->second.name_.size()) {
			return &it->second;
		}

		return nullptr;
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

		if (auto sub_vendor = lookup_vendor(sub_vendor_id)) {
			sub.vendor_known = true;
			sub.vendor_name = sub_vendor->name_;
		}

		if (auto vendor = lookup_vendor(vendor_id)) {
			main.vendor_known = true;
			main.vendor_name = vendor->name_;

			if (auto device = vendor->lookup_device(device_id)) {
				main.device_known = true;
				main.device_name = device->name_;

				if (auto sub_dev = device->lookup_sub(sub_vendor_id, sub_device_id)) {
					sub.device_known = true;
					sub.device_name = *sub_dev;
				}
			}
		}

		return device{main, sub};
	}
};
