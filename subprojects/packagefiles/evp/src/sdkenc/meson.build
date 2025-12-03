# SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
#
# SPDX-License-Identifier: Apache-2.0

run_command(['mkdir', '-p', evp_build_root + '/include/sdkenc'], check: true)

if meson.is_cross_build()
	flatcc_cli_native = find_program('flatcc_cli_native')
	sdkenc_headers = custom_target(
		'sdk_fbs',
		input : ['sdk.fbs'],
		output : ['test.h'], # This value doesn't actually matter.
		command : [
			flatcc_cli_native, '-a', '--json-printer',
			'-o', evp_build_root + '/include/sdkenc',
			'@INPUT@'
		],
		build_by_default : true,
	)
else
	flatcc_cli = find_program('flatcc_cli')
	sdkenc_headers = custom_target(
		'sdk_fbs',
		input : ['sdk.fbs'],
		output : ['test.h'], # This value doesn't actually matter.
		command : [
			flatcc_cli, '-a', '--json-printer',
			'-o', evp_build_root + '/include/sdkenc',
			'@INPUT@'
		],
		build_by_default : true,
	)
endif

sdkenc_dep = declare_dependency(
	sources : sdkenc_headers
)
