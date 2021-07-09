#include <provider/stdin.hpp>

#include <charconv>
#include <iostream>
#include <optional>
#include <string_view>

#include <fmt/core.h>

namespace {
	std::optional<pci_device> fetch_device(std::string_view dev_str) {
		pci_device result;

		auto colon = dev_str.find(':');
		if (colon == std::string_view::npos)
			return std::nullopt;

		auto vendor = dev_str.substr(0, colon),
			device = dev_str.substr(colon + 1, dev_str.size() - colon - 1);

		if (vendor.starts_with("0x"))
			vendor.remove_prefix(2);

		if (device.starts_with("0x"))
			device.remove_prefix(2);

		if (vendor.size() != 4 || device.size() != 4)
			return std::nullopt;

		std::from_chars(vendor.data(), vendor.data() + vendor.size(), result.vendor, 16);
		std::from_chars(device.data(), device.data() + device.size(), result.device, 16);

		return result;
	}
} // namespace anonymous

std::vector<pci_device> stdin_provider::fetch_devices(std::error_code &) {
	std::vector<pci_device> devices;

	std::string str;
	while (std::cin >> str) {
		if (auto dev = fetch_device(str))
			devices.push_back(*dev);
		else
			fmt::print(stderr, "Invalid vendor:device pair '{}'", str);
	}

	return devices;
}
