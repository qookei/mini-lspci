project('fmt', 'cpp',
	license : 'BSD',
	version : '8.0.1')

sources = ['fmt/src/format.cc', 'fmt/src/os.cc']

incs = include_directories('fmt/include')

libfmt = library('fmt', sources,
	include_directories : incs,
	cpp_args : '-DFMT_EXCEPTIONS=0')

fmt_dep = declare_dependency(
	link_with : libfmt,
	include_directories : incs)
