#include <pciids.hpp>
#include <util/line_range.hpp>

#include <charconv>

void pciids_parser::internalize_device(uint16_t vendor_id, uint16_t device_id,
		uint16_t sub_vendor_id, uint16_t sub_device_id) {

	auto [vit, _1] = vendors_.try_emplace(vendor_id, vendor_info{});
	auto [dit, _2] = vit->second.devices.try_emplace(device_id, device_info{});

	if (sub_vendor_id != 0xFFFF) {
		vendors_.try_emplace(sub_vendor_id, vendor_info{});
		dit->second.sub_devices.try_emplace(make_key_(sub_device_id, sub_vendor_id), std::string_view{});
	}
}

void pciids_parser::process() {
	vendor_info *vendor = nullptr;
	device_info *device = nullptr;

	for (auto line : line_range{file_.as_string_view()}) {
		if (!line.size() || line[0] == '#')
			continue;

		if (line[0] != '\t')
			vendor = nullptr;

		if (line[1] != '\t')
			device = nullptr;

		if (!vendor) {
			if (!isxdigit(line[0]))
				continue;

			uint16_t id = 0xFFFF;
			std::from_chars(line.data(), line.data() + 4, id, 16);

			auto it = vendors_.find(id);
			if (it == vendors_.end())
				continue;

			vendor = &it->second;
			vendor->name = line.substr(6);

			continue;
		}

		if (line[1] == '\t' && device) {
			uint32_t sub_vendor_id = 0xFFFF;
			uint32_t sub_device_id = 0xFFFF;
			std::from_chars(line.data() + 2, line.data() + 6, sub_vendor_id, 16);
			std::from_chars(line.data() + 7, line.data() + 11, sub_device_id, 16);

			auto it = device->sub_devices.find(make_key_(sub_vendor_id, sub_device_id));

			if (it != device->sub_devices.end())
				it->second = line.substr(13);

			continue;
		}

		if (vendor) {
			uint16_t id = 0xFFFF;
			std::from_chars(line.data() + 1, line.data() + 5, id, 16);

			auto it = vendor->devices.find(id);
			if (it == vendor->devices.end())
				continue;

			device = &it->second;

			device->name = line.substr(7);
		}
	}
}
