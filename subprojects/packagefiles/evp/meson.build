# SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
#
# SPDX-License-Identifier: Apache-2.0

project('evp', 'c')

evp_build_root = meson.current_build_dir()
evp_source_root = meson.current_source_dir()
libevp_includes = [include_directories('include')]

subdir('include')
subdir('src')

meson.override_dependency(
	'evp_agent',
	evp_agent_dep
)

meson.override_dependency(
	'evp_utils',
	evp_utils_dep
)
