if meson.is_cross_build()
  flatcc_native = static_library(
      'flatcc_native',
      flatcc_sources,
      include_directories : flatcc_includes,
      native: true
  )

  flatcc_dep_native = declare_dependency(
      link_with : flatcc_native,
      include_directories : flatcc_includes
  )
endif

flatcc = static_library(
	'flatcc',
	flatcc_sources,
	include_directories : flatcc_includes
)

flatcc_dep = declare_dependency(
	link_with : flatcc,
	include_directories : flatcc_includes
)
