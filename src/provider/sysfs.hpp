#pragma once

#include <provider/provider.hpp>

struct sysfs_provider : provider {
	std::vector<pci_device> fetch_devices(std::error_code &ec) override;

	bool has_nonzero_seg() override {
		return has_nonzero_seg_;
	}

private:
	bool has_nonzero_seg_ = false;
};
