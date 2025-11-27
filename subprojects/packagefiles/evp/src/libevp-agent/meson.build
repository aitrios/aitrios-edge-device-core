# SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
#
# SPDX-License-Identifier: Apache-2.0

subdir('MQTT-C')

libevp_agent_sources = files([
	'agent_event.c',
	'backdoor.c',
	'base64.c',
	'blob.c',
	'blob_get.c',
	'blob_http.c',
	'blob_put.c',
	'blob_rpc.c',
	'blob_type_azure_blob.c',
	'blob_type_evp.c',
	'blob_type_evp_rpc.c',
	'blob_type_http.c',
	'certificates.c',
	'config.c',
	'config_pk_file.c',
	'connections.c',
	'deployment.c',
	'device_config.c',
	'device_state.c',
	'direct_command.c',
	'evp_agent.c',
	'evp_hub.c',
	'fsutil.c',
	'hash.c',
	'hub/tb/tb.c',
	'hub/hub_tb.c',
	'mstp_schema.c',
	'hub/dispatch.c',
	'https_ssl_config.c',
	'instance_config.c',
	'ioutil.c',
	'local_socket.c',
	'main_loop.c',
	'manifest.c',
	'map.c',
	'models/mstp.c',
	'module.c',
	'module_log_cap.c',
	'module_log_queue.c',
	'module_log_send_telemetry.c',
	'module_log_streaming.c',
	'module_instance.c',
	'module_instance_impl_ops.c',
	'module_impl_ops.c',
	'mqtt_pal/mbedtls.c',
	'mqtt_pal/tcp.c',
	'mqtt_pal_custom.c',
	'mqtt.c',
	'notification.c',
	'pal.c',
	'path.c',
	'platform.c',
	'proxy.c',
	'reconcile.c',
	'report.c',
	'report_refresh.c',
	'report_send.c',
	'req.c',
	'retry.c',
	'signature_verification.c',
	'sdk.c',
	'sdk_msg.c',
	'socketutil.c',
	'stream/null.c',
	'stream/stream.c',
	'stream/posix.c',
	'system_info.c',
	'tcp.c',
	'telemetry.c',
	'timeutil.c',
	'transport.c',
	'util.c',
	'work.c',
	'xlog.c',
	'xmqtt.c',
	'tls.c',
	'xpthread.c',
	'webclient/webclient.c',
	'webclient_mbedtls.c',
	'netlib/netlib_parseurl.c',
	'sys/sys_collect_responses.c',
	'sys/sys_common.c',
	'sys/sys_client_alloc.c',
	'sys/sys_client_dealloc.c',
	'sys/sys_collect_states.c',
	'sys/sys_collect_telemetry.c',
	'sys/sys_ddc_dealloc.c',
	'sys/SYS_get_blob.c',
	'sys/sys_group_alloc.c',
	'sys/sys_group_dealloc.c',
	'sys/sys_is_sysapp.c',
	'sys/sys_notify_config.c',
	'sys/sys_notify_ddc.c',
	'sys/SYS_process_event.c',
	'sys/sys_process_events.c',
	'sys/SYS_put_blob.c',
	'sys/SYS_put_blob_mstp.c',
	'sys/SYS_reason_tostr.c',
	'sys/sys_register.c',
	'sys/SYS_register_command_cb.c',
	'sys/SYS_result_tostr.c',
	'sys/SYS_send_telemetry.c',
	'sys/SYS_set_response_cb.c',
	'sys/SYS_set_configuration_cb.c',
	'sys/SYS_set_state.c',
	'sys/sys_state_dealloc.c',
	'sys/sys_telemetry_dealloc.c',
	'sys/sys_unregister.c',
])

# CONFIG_EVP_MODULE_IMPL_WASM
libevp_agent_sources += files([
	'module_api_wasm.c',
	'module_impl_wasm.c',
	'module_instance_impl_wasm.c',
	'sdk_callback_wasm.c',
	'sdk_local_wasm.c',
])

# CONFIG_EVP_SDK_LOCAL
libevp_agent_sources += files([
	'sdk_common.c',
	'sdk_local.c',
	'sdk_execute.c',
	'sdk_local_native.c',
	'sdk_callback_native.c',
])

# CONFIG_EVP_SDK_SOCKET
libevp_agent_sources += files([
	'sdk_remote.c',
	'sdk_worker.c',
	'sdkrpc/server.c'
])

# CONFIG_EVP_MODULE_IMPL_NOOP
libevp_agent_sources += files([
	'module_instance_impl_noop.c'
])

# CONFIG_EVP_MODULE_IMPL_OBJ
libevp_agent_sources += files([
	'module_impl_obj.c'
])

# CONFIG_EVP_MODULE_IMPL_SPAWN
libevp_agent_sources += files([
	'module_impl_spawn.c',
	'module_instance_impl_spawn.c',
])

# CONFIG_EVP_MODULE_IMPL_PYTHON
libevp_agent_sources += files([
	'module_impl_python.c',
	'module_instance_impl_python.c',
])

keyval = import('keyval')
config_data = keyval.load('@0@/include/.config'.format(evp_build_root))

# CONFIG_EVP_TWINS_PERSISTENCE
if config_data.get('CONFIG_EVP_TWINS_PERSISTENCE', '') == 'y'
	libevp_agent_sources += files([
		'persist.c'
	])
else
	libevp_agent_sources += files([
		'nopersist.c'
	])
endif

flatcc_dep = dependency('flatccrt', fallback : ['flatcc', 'flatccrt'])
mbedtls_dep = dependency('mbedtls', fallback : ['mbedtls', 'mbedtls'])
mbedcrypto_dep = dependency('mbedcrypto', fallback : ['mbedtls', 'mbedcrypto'])
mbedx509_dep = dependency('mbedx509', fallback : ['mbedtls', 'mbedx509'])
parson_dep = dependency('parson', fallback : ['parson', 'parson'], static : true)
wamr_dep = dependency('vmlib', fallback : ['wasm-micro-runtime', 'vmlib'])

evp_agent = static_library(
	'evp-agent',
	libevp_agent_sources,
	include_directories : libevp_includes,
	dependencies : [
		flatcc_dep,
		mbedtls_dep,
		mbedcrypto_dep,
		mbedx509_dep,
		parson_dep,
		wamr_dep,
		sdkenc_dep,
	]
)

evp_agent_dep = declare_dependency(
	link_with : evp_agent,
	include_directories : libevp_includes
)
