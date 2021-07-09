#include <provider/sysfs.hpp>

#include <fstream>
#include <filesystem>

#include <optional>
#include <concepts>
#include <charconv>

#include <config.hpp>

#include <fmt/core.h>

namespace fs = std::filesystem;

namespace {
	std::optional<uint32_t> fetch_attr(const fs::path &device_path,
			const char *attr) {
		std::ifstream file{device_path / attr};
		uint32_t value = 0;
		char buf[64];

		if (!file.good()) {
			return std::nullopt;
		}

		file.read(buf, 64);

		if (!file.good() && !file.eof()) {
			return std::nullopt;
		}

		const char *ptr = buf;
		size_t len = file.gcount();

		if (buf[0] == '0' && buf[1] == 'x') {
			ptr += 2;
			len -= 2;
		}

		std::from_chars(ptr, ptr + len, value, 16);

		return value;
	}

	uint32_t fetch_attr_or_die(const fs::path &device_path,
			const char *attr) {
		if (auto val = fetch_attr(device_path, attr)) {
			return *val;
		} else {
			fmt::print(stderr, "Failed to fetch attribute '{}' for {}",
					attr, device_path.filename());
			exit(1);
		}
	}

	pci_device fetch_device(const fs::path &device_path) {
		pci_device result;

		result.has_location = true;
		auto loc = device_path.filename();
		auto cloc = loc.c_str();
		std::from_chars(cloc, cloc + 4, result.segment, 16);
		std::from_chars(cloc + 5, cloc + 7, result.bus, 16);
		std::from_chars(cloc + 8, cloc + 10, result.slot, 16);
		std::from_chars(cloc + 11, cloc + 12, result.function, 16);

		result.vendor = fetch_attr_or_die(device_path, "vendor");
		result.device = fetch_attr_or_die(device_path, "device");

		if (auto sv = fetch_attr(device_path, "subsystem_vendor")) {
			result.has_subsystem = true;
			result.subsystem_vendor = *sv;
			result.subsystem_device = fetch_attr_or_die(device_path, "subsystem_device");
		}

		if (auto rev = fetch_attr(device_path, "revision")) {
			result.has_rev = true;
			result.rev = *rev;
		}

		return result;
	}
} // namespace anonymous

std::vector<pci_device> sysfs_provider::fetch_devices(std::error_code &ec) {
	std::vector<pci_device> devices;

	for (auto &ent : fs::directory_iterator{sysfs_dev_path, ec}) {
		auto dev = fetch_device(ent.path());
		if (dev.segment)
			has_nonzero_seg_ = true;

		devices.push_back(std::move(dev));
	}

	return devices;
}
