# SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
#
# SPDX-License-Identifier: Apache-2.0

evp_utils_sources = files([
	'chan_alloc.c',
	'chan_dealloc.c',
	'chan_dequeue.c',
	'chan_item_alloc.c',
	'chan_item_dealloc.c',
	'chan_enqueue.c',
	'chan_recv.c',
	'chan_recv_helper.c',
	'chan_tryrecv.c',
	'chan_timedrecv.c',
	'chan_send.c',
	'chan_size.c',
	'djb2.c',
	'string_map_alloc.c',
	'string_map_count.c',
	'string_map_dealloc.c',
	'string_map_forall.c',
	'string_map_insert.c',
	'string_map_lookup.c',
])

evp_utils = static_library(
	'evp-utils',
	evp_utils_sources,
	include_directories : libevp_includes
)

evp_utils_dep = declare_dependency(
	link_with : evp_utils,
	include_directories : libevp_includes
)
