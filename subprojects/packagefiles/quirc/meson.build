project('quirc', 'c', version : '1.2')

subdir('lib')

quirc = static_library(
	'quirc',
	quirc_sources,
	include_directories : quirc_includes
)

quirc_dep = declare_dependency(
	link_with : quirc,
	include_directories : quirc_includes
)

meson.override_dependency(
	'quirc',
	quirc_dep
)
