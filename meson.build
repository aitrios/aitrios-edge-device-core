# SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
#
# SPDX-License-Identifier: Apache-2.0

project(
	'edge device core',
	'c', 'cpp',
	version : '0.3.8',
)
cc = meson.get_compiler('c')
if meson.is_cross_build()
    sysroot_path = meson.get_cross_property('sys_root', '/')
endif

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
fs = import('fs')
if meson.is_cross_build() and fs.exists(sysroot_path + '/usr/include/onnxruntime/core/session') and fs.exists(sysroot_path + '/usr/lib/libonnxruntime.so')
  wamr_vars.add_cmake_defines({
    'WAMR_BUILD_TARGET': target_machine.cpu_family().to_upper(),
    'APP_THREAD_STACK_SIZE_DEFAULT': 131072,
    'APP_THREAD_STACK_SIZE_MIN':     131072,
    'CMAKE_PREFIX_PATH': sysroot_path + '/opt/senscord',
    'CMAKE_SYSROOT': sysroot_path,
    'onnxruntime_INCLUDE_DIR': sysroot_path + '/usr/include/onnxruntime/core/session',
    'onnxruntime_LIBRARY': sysroot_path + '/usr/lib/libonnxruntime.so',
  })
else
  wamr_vars.add_cmake_defines({
    'WAMR_BUILD_TARGET': target_machine.cpu_family().to_upper(),
    'APP_THREAD_STACK_SIZE_DEFAULT': 131072,
    'APP_THREAD_STACK_SIZE_MIN':     131072,
    'CMAKE_PREFIX_PATH': '/opt/senscord'
  })
endif
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

# libchrony, vsclient for t4r target
if get_option('target') == 't4r'
  libchrony_dep = dependency('libchrony')
  vsclient_dep = cc.find_library('vsclient', required : true)
endif

if meson.is_cross_build()
  senscord_so_dep = declare_dependency(
    link_args: [
      sysroot_path + '/opt/senscord/lib/libsenscord.so',
      sysroot_path + '/opt/senscord/lib/libsenscord_osal.so',
      sysroot_path + '/opt/senscord/lib/libsenscord_wamr.so'
    ]
  )
else
  senscord_so_dep = declare_dependency(
    link_args: [
      '/opt/senscord/lib/libsenscord.so',
      '/opt/senscord/lib/libsenscord_osal.so',
      '/opt/senscord/lib/libsenscord_wamr.so'
    ]
  )
endif

if meson.is_cross_build()
  inc_senscord = include_directories(
    sysroot_path + '/opt/senscord/include'
  )
else
  inc_senscord = include_directories(
    '/opt/senscord/include'
  )
endif

senscord_dep = declare_dependency(
  include_directories: [
  inc_senscord
]
)

subdir('src')

evp_agent_dep = dependency('evp_agent', fallback : 'evp')
evp_utils_dep = dependency('evp_utils')

libpsm = shared_library(
  'parameter_storage_manager',
  sources: [
		psm_sources,
		esf_sources,
		codec_sources,
		pl_sources,
		utility_sources,
		hal_sources,
		evp_agent_sources,
		system_app_sources,
	],
  include_directories: [
		hal_includes_public,
		hal_includes_internal,
		pl_includes_public,
		pl_includes_internal,
		utility_includes_public,
		utility_includes_internal,
		esf_includes_public,
		esf_includes_internal,
		codec_includes_public,
		evp_agent_includes,
		system_app_includes,
		inc_senscord,
	],
	link_args: ['-lm','-export-dynamic'],
	dependencies : [
		sqlite3_dep,
		parson_dep,
		wamr_dep,
		mbedtls_dep,
		mbedcrypto_dep,
		mbedx509_dep,
		flatcc_dep,
		base64_dep,
		jpeg_dep,
		quirc_dep,
		libnm_dep,
		sensor_ai_lib_dep,
		evp_agent_dep,
		evp_utils_dep,
		senscord_dep,
		senscord_so_dep,
	]+ (get_option('target') == 't4r' ? [libchrony_dep, vsclient_dep] : []),
	c_args : [
		systemapps_arguments,
		senscord_arguments,
	],
	cpp_args : senscord_arguments,
)
libpsm_dep = declare_dependency(link_with: libpsm)

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
		sqlite3_dep,
		libpsm_dep
	]+ (get_option('target') == 't4r' ? [libchrony_dep, vsclient_dep] : []),
	c_args : systemapps_arguments,
    install_rpath: '/opt/senscord/lib',
    link_args: ['-lm','-export-dynamic']
)

# A custom target to run the script packaging the output into a .deb file for
# installation on the Raspberry Pi.
mkdeb_script = join_paths(meson.source_root(), 'script', 'mk-deb.sh')
run_target(
	'deb',
	command : [
		mkdeb_script,
		'-a', target_machine.cpu_family(),
		'-V', meson.project_version(),
		'-d', meson.build_root()
	],
	depends : [edc, libpsm]
)
