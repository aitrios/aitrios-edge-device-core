# SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
#
# SPDX-License-Identifier: Apache-2.0

# Ideally we would compile all objects in subdirectories, but the structure of
# the project makes that quite difficult. Instead the subdir() directives will
# compile what sources they can and collate sources and include directory paths
# for the final target.

subdir('manager')
subdir('evp-agent')

subdir('sensor/src')
if get_option('test_build')
    subdir('sensor/test')
endif

subdir('app')
