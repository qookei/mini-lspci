project('mini-lspci', 'cpp',
	version : '1.1',
	license : 'zlib',
	default_options : [ 'cpp_std=c++20', 'warning_level=3' ])
executable('lspci', 'src/main.cpp', 'src/provider/sysfs.cpp',
	include_directories: 'src/',
	install : true)
