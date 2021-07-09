#pragma once

#include <provider/provider.hpp>

struct stdin_provider : provider {
	std::vector<pci_device> fetch_devices(std::error_code &ec) override;

	bool has_nonzero_seg() override {
		return false;
	}
};
