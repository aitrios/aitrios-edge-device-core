flatcc_sources = files([
	meson.project_source_root() + '/external/hash/cmetrohash64.c',
	meson.project_source_root() + '/external/hash/str_set.c',
	meson.project_source_root() + '/external/hash/ptr_set.c',
	'compiler/hash_tables/symbol_table.c',
	'compiler/hash_tables/scope_table.c',
	'compiler/hash_tables/name_table.c',
	'compiler/hash_tables/schema_table.c',
	'compiler/hash_tables/value_set.c',
	'compiler/fileio.c',
	'compiler/parser.c',
	'compiler/semantics.c',
	'compiler/coerce.c',
	'compiler/flatcc.c',
	'compiler/codegen_c.c',
	'compiler/codegen_c_reader.c',
	'compiler/codegen_c_sort.c',
	'compiler/codegen_c_builder.c',
	'compiler/codegen_c_verifier.c',
	'compiler/codegen_c_sorter.c',
	'compiler/codegen_c_json_parser.c',
	'compiler/codegen_c_json_printer.c',
	# needed for building binary schema
	'runtime/builder.c',
	'runtime/emitter.c',
	'runtime/refmap.c',
])

subdir('compiler')
subdir('cli')
subdir('runtime')
