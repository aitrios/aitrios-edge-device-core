project('flatcc', 'c')

flatcc_includes = include_directories(['external', 'include', 'config'])
flatccrt_includes = include_directories('include')

subdir('src')

meson.override_find_program('flatcc_cli', flatcc_cli)
if meson.is_cross_build()
  meson.override_find_program('flatcc_cli_native', flatcc_cli_native)
endif
meson.override_dependency('flatccrt', flatccrt_dep)
