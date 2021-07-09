#include <provider/picker.hpp>
#include <config.hpp>

#ifdef SYSFS_PROVIDER_ENABLED
#include <provider/sysfs.hpp>
#endif

#ifdef STDIN_PROVIDER_ENABLED
#include <provider/stdin.hpp>
#endif

std::unique_ptr<provider> get_provider(std::string_view name) {
#ifdef SYSFS_PROVIDER_ENABLED
	if (name == "sysfs")
		return std::make_unique<sysfs_provider>();
#endif

#ifdef STDIN_PROVIDER_ENABLED
	if (name == "stdin")
		return std::make_unique<stdin_provider>();
#endif

	return nullptr;
}
