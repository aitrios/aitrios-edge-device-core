if meson.is_cross_build()
  flatcc_cli_native = executable(
      'flatcc_cli_native',
      files(['flatcc_cli.c']),
      dependencies : flatcc_dep_native,
      native: true
  )
endif

flatcc_cli = executable(
	'flatcc_cli',
	files(['flatcc_cli.c']),
	dependencies : flatcc_dep
)
