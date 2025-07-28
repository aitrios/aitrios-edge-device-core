project('base64', 'c')

base64_includes = include_directories('.')
base64_sources = files(['base64.c'])

base64 = static_library(
	'base64',
	base64_sources,
	include_directories : base64_includes
)

base64_dep = declare_dependency(
	link_with : base64,
	include_directories : base64_includes
)

meson.override_dependency(
	'base64',
	base64_dep
)
