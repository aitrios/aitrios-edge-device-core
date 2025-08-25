# SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
#
# SPDX-License-Identifier: Apache-2.0

project(
	'edge device core',
	'c', 'cpp',
	version : '0.1.8',
)
cc = meson.get_compiler('c')

# First we need to set up some dependencies for OSS projects that we will
# consume. These will probably come from our subproject wraps rather than from
# packages.

base64_dep = dependency('base64')
parson_dep = dependency('parson', fallback : ['parson', 'parson'], static : true)
jpeg_dep = dependency('libjpeg')
quirc_dep = dependency('quirc', fallback : 'quirc')

# wamr is a cmake project
cmake = import('cmake')
wamr_vars = cmake.subproject_options()
wamr_vars.append_link_args('-lm')
wamr_vars.add_cmake_defines({
  'WAMR_BUILD_TARGET': target_machine.cpu_family().to_upper(),
  'APP_THREAD_STACK_SIZE_DEFAULT': 131072,
  'APP_THREAD_STACK_SIZE_MIN':     131072,
  'CMAKE_PREFIX_PATH': '/opt/senscord'
})
wamr = cmake.subproject('wasm-micro-runtime', options : wamr_vars)
wamr_dep = wamr.dependency('vmlib')
wasm_iwasm_inc = include_directories('subprojects/wasm-micro-runtime/core/iwasm/include')
wasm_micro_runtime_utils_includes = [include_directories('subprojects/wasm-micro-runtime/core/shared/utils')]
wasm_micro_runtime_platform_includes = [include_directories('subprojects/wasm-micro-runtime/core/shared/platform/include')]
meson.override_dependency('vmlib', wamr_dep)

# As is flatcc
flatcc_dep = dependency('flatccrt', fallback : ['flatcc', 'flatccrt'])
flatcc_cli = find_program('flatcc_cli')
if meson.is_cross_build()
  flatcc_cli_native = find_program('flatcc_cli_native')
endif

# And mbed tls
mbedtls_vars = cmake.subproject_options()
mbedtls_vars.add_cmake_defines({'CMAKE_POSITION_INDEPENDENT_CODE':true})
mbedtls = cmake.subproject('mbedtls', options : mbedtls_vars)
mbedtls_dep = mbedtls.dependency('mbedtls')
mbedcrypto_dep = mbedtls.dependency('mbedcrypto')
mbedx509_dep = mbedtls.dependency('mbedx509')
meson.override_dependency('mbedtls', mbedtls_dep)
meson.override_dependency('mbedcrypto', mbedcrypto_dep)
meson.override_dependency('mbedx509', mbedx509_dep)

# SQLite comes from libsqlite3-dev which has a package config file
sqlite3_dep = dependency('sqlite3')

# likewise libnm, which comes from libnm-dev and has a package config file
libnm_dep = dependency('libnm')

senscord_so_dep = declare_dependency(
  link_args: [
    '/opt/senscord/lib/libsenscord.so',
    '/opt/senscord/lib/libsenscord_osal.so',
    '/opt/senscord/lib/libsenscord_wamr.so'
  ]
)

inc_senscord = include_directories(
  '/opt/senscord/include'
)

senscord_dep = declare_dependency(
  include_directories: [
  inc_senscord
]
)

subdir('src')

evp_agent_dep = dependency('evp_agent', fallback : 'evp')
evp_utils_dep = dependency('evp_utils')

edc = executable(
	'edge_device_core',
	[
		esf_sources,
		evp_agent_sources,
		system_app_sources
	],
	include_directories : [
		esf_includes_internal,
		system_app_includes,
		esf_includes_public,
		utility_includes_public,
		evp_agent_includes,
		codec_includes_public,
		hal_includes_public,
		senscord_includes_public
	],
	dependencies : [
		wamr_dep,
		parson_dep,
		mbedtls_dep,
		mbedcrypto_dep,
		mbedx509_dep,
		flatcc_dep,
		jpeg_dep,
		utility_dep,
		porting_layer_dep,
		codecs_dep,
		hal_dep,
		evp_agent_dep,
		evp_utils_dep,
		senscord_dep,
		senscord_so_dep,
		quirc_dep,
		sqlite3_dep
	],
	c_args : systemapps_arguments,
    install_rpath: '/opt/senscord/lib',
    link_args: ['-lm','-export-dynamic']
)

# A custom target to run the script packaging the output into a .deb file for
# installation on the Raspberry Pi.
run_target(
	'deb',
	command : [
		'script/mk-deb.sh',
		'-a', target_machine.cpu_family(),
		'-V', meson.project_version(),
		'-d', meson.build_root().split('/')[-1]
	]
)
