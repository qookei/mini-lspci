#include <fmt/core.h>

#include <string_view>
#include <cmath>

#include <provider/picker.hpp>
#include <pciids.hpp>
#include <config.hpp>

enum class numeric {
	no = 0,
	yes,
	mixed
};

void internal_print_names(const pciids_parser::name_pair &name, numeric mode) {
	if (mode == numeric::yes) {
		fmt::print("{:04x}:{:04x}", name.vendor_id, name.device_id);
		return;
	}

	if (name.vendor_known) {
		fmt::print("{} ", name.vendor_name);
	}

	if (name.device_known) {
		fmt::print("{}", name.device_name);
	} else {
		fmt::print("Device");
	}

	if (mode == numeric::mixed) {
		fmt::print(" [{:04x}:{:04x}]", name.vendor_id, name.device_id);
	} else if (name.vendor_known && !name.device_known) {
		fmt::print(" {:04x}", name.device_id);
	} else if (!name.vendor_known && !name.device_known) {
		fmt::print(" {:04x}:{:04x}", name.vendor_id, name.device_id);
	}
}

void print_device(const pci_device &dev,
		const pciids_parser &parser,
		bool verbose, numeric mode) {
	auto info = parser.get_device(dev.vendor, dev.device, dev.subsystem_vendor, dev.subsystem_device);

	if (dev.has_location)
		fmt::print("{:02x}:{:02x}.{:1x}: ", dev.bus, dev.slot, dev.function);


	internal_print_names(info.main, mode);

	if (dev.has_rev && dev.rev) {
		fmt::print(" (rev {:02x})", dev.rev);
	}

	fmt::print("\n");

	if (verbose) {
		if (dev.has_subsystem && dev.subsystem_vendor != 0xFFFF) {
			fmt::print("\tSubsystem: ");

			internal_print_names(info.sub, mode);
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
	fmt::print("\t-p <path>      Set path to pci.ids file (default: {})\n", pciids_path);
	fmt::print("\t-P <provoder>  Select provider to use (default: {})\n", default_provider);
	fmt::print("\t-V             Show version\n");
	fmt::print("\t-h             Show usage\n");
}

int main(int argc, char **argv) {
	const char *wanted_pciids_path = pciids_path;
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
				wanted_pciids_path = optarg;
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

	pciids_parser parser{wanted_pciids_path};

	for (auto &dev : devices) {
		parser.internalize_device(dev.vendor, dev.device, dev.subsystem_vendor, dev.subsystem_device);
	}

	parser.process();

	for (auto &dev : devices) {
		print_device(dev, parser, verbose, static_cast<numeric>(numeric_level));
	}
}
