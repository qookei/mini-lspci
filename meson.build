project('mini-lspci', 'cpp',
	version : '1.1',
	license : 'zlib',
	default_options : ['cpp_std=c++20', 'warning_level=3'])

conf_data = configuration_data()

conf_data.set_quoted('VERSION', meson.project_version())

sources = [
	'src/main.cpp',

	'src/util/mapped_file.hpp',
	'src/util/line_range.hpp',

	'src/provider/provider.hpp',
	'src/provider/picker.cpp',
	'src/provider/picker.hpp'
]

use_sysfs = false
use_stdin = false
enabled_providers = []

if get_option('sysfs-provider').auto()
	use_sysfs = ['linux', 'managarm'].contains(host_machine.system())
elif get_option('sysfs-provider').enabled()
	use_sysfs = true
endif

if get_option('stdin-provider').auto() or get_option('stdin-provider').enabled()
	use_stdin = true
endif

conf_data.set_quoted('SYSFS_DEV_PATH', get_option('sysfs-dev-path'))

if use_sysfs
	sources += ['src/provider/sysfs.cpp', 'src/provider/sysfs.hpp']
	conf_data.set('SYSFS_PROVIDER_ENABLED', true)
	enabled_providers += 'sysfs'
endif

if use_stdin
	sources += ['src/provider/stdin.cpp', 'src/provider/stdin.hpp']
	conf_data.set('STDIN_PROVIDER_ENABLED', true)
	enabled_providers += 'stdin'
endif

default_provider = get_option('default-provider')

if not enabled_providers.contains(default_provider)
	error('Invalid default provider ' + default_provider)
endif

conf_data.set_quoted('DEFAULT_PROVIDER', default_provider)

configure_file(input : 'src/config.hpp.in',
	output : 'config.hpp',
	configuration : conf_data)

executable('lspci', sources,
	include_directories: 'src/',
	install : true)
