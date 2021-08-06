#include <pciids.hpp>

#include <charconv>

void pciids_parser::internalize_device(uint16_t vendor_id, uint16_t device_id,
		uint16_t sub_vendor_id, uint16_t sub_device_id) {
	if (sub_vendor_id != 0xFFFF)
		needed_vendors_.insert(sub_vendor_id);
	needed_vendors_.insert(vendor_id);

	device_infos_.try_emplace(make_key_(vendor_id, device_id), device_info{});

	if (sub_vendor_id != 0xFFFF)
		device_infos_[make_key_(vendor_id, device_id)].subsystem_names.try_emplace(make_key_(sub_vendor_id, sub_device_id), std::string_view{});
}

void pciids_parser::finalize() {
	assert(!finalized_);

	collect_vendors_();
	collect_devices_();

	finalized_ = true;
}

void pciids_parser::collect_vendors_() {
	for (auto [n, line] : lines_) {
		if (!line.size() || !isxdigit(line[0]))
			continue;

		uint16_t id = 0xFFFF;
		std::from_chars(line.data(), line.data() + 4, id, 16);

		if (needed_vendors_.contains(id))
			vendor_infos_.emplace(id, vendor_info{id, line.substr(6), n});
	}
}

void pciids_parser::collect_devices_() {
	vendor_info *vendor = nullptr;
	uint32_t id = 0xFFFF;

	for (auto [i, line] : lines_) {
		if (!line.size() || line[0] == '#')
			continue;

		if (!vendor) {
			if (!isxdigit(line[0]))
				continue;

			for (auto &[_, v] : vendor_infos_) {
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

		if (line[1] == '\t' && device_infos_.contains(make_key_(vendor->id, id))) {
			auto &dev = device_infos_[id | (vendor->id << 16)];

			uint32_t sub_vendor_id = 0xFFFF;
			uint32_t sub_device_id = 0xFFFF;
			std::from_chars(line.data() + 2, line.data() + 6, sub_vendor_id, 16);
			std::from_chars(line.data() + 7, line.data() + 11, sub_device_id, 16);

			auto it = dev.subsystem_names.find(make_key_(sub_vendor_id, sub_device_id));

			if (it != dev.subsystem_names.end())
				it->second = line.substr(13);

			continue;
		}

		std::from_chars(line.data() + 1, line.data() + 5, id, 16);

		if (device_infos_.contains(make_key_(vendor->id, id)))
			device_infos_[make_key_(vendor->id, id)].name = line.substr(7);
	}
}
