flatccrt_sources = files([
	'builder.c',
	'emitter.c',
	'refmap.c',
	'verifier.c',
	'json_parser.c',
	'json_printer.c',
])

flatccrt = static_library(
	'flatccrt',
	flatccrt_sources,
	include_directories : flatccrt_includes
)

flatccrt_dep = declare_dependency(
	link_with : flatccrt,
	include_directories : flatccrt_includes
)
