#pragma once

#include <util/mapped_file.hpp>
#include <util/line_range.hpp>

#include <unordered_map>
#include <unordered_set>
#include <string_view>
#include <cassert>

struct pciids_parser {
	pciids_parser(const char *path)
	: file_{path} { }

	void internalize_device(uint16_t vendor_id, uint16_t device_id,
			uint16_t sub_vendor_id, uint16_t sub_device_id);

	void finalize();

private:
	mapped_file file_;
	line_range lines_{file_.as_string_view()};

	struct vendor_info {
		uint32_t id;
		std::string_view name;
		size_t line;
	};

	struct device_info {
		std::string_view name;

		std::unordered_map<uint32_t, std::string_view> subsystem_names;
	};

	std::unordered_set<uint16_t> needed_vendors_;
	std::unordered_map<uint16_t, vendor_info> vendor_infos_;
	std::unordered_map<uint32_t, device_info> device_infos_;

	bool finalized_ = false;

	void collect_vendors_();
	void collect_devices_();

	static uint32_t make_key_(uint32_t vendor, uint32_t device) {
		return (device | (vendor << 16));
	}

public:
	struct name_pair {
		bool vendor_known;
		bool device_known;

		uint16_t vendor_id;
		uint16_t device_id;
		std::string_view vendor_name;
		std::string_view device_name;
	};

	struct device {
		name_pair main;
		name_pair sub;
	};

	device get_device(uint16_t vendor_id, uint16_t device_id,
			uint16_t sub_vendor_id, uint16_t sub_device_id) const {
		assert(finalized_);

		name_pair main{false, false, vendor_id, device_id, "", ""};
		name_pair sub{false, false, sub_vendor_id, sub_device_id, "", ""};

		if (auto it = vendor_infos_.find(vendor_id); it != vendor_infos_.end() && it->second.name.size()) {
			main.vendor_known = true;
			main.vendor_name = it->second.name;
		}

		if (auto it = vendor_infos_.find(sub_vendor_id); it != vendor_infos_.end() && it->second.name.size()) {
			sub.vendor_known = true;
			sub.vendor_name = it->second.name;
		}

		if (auto it = device_infos_.find(make_key_(vendor_id, device_id)); it != device_infos_.end() && it->second.name.size()) {
			main.device_known = true;
			main.device_name = it->second.name;

			auto &subs = it->second.subsystem_names;
			if (auto it = subs.find(make_key_(sub_vendor_id, sub_device_id)); it != subs.end() && it->second.size()) {
				sub.device_known = true;
				sub.device_name = it->second;
			}
		}

		return device{main, sub};
	}
};
