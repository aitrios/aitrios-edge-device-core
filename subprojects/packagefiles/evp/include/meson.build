# SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
#
# SPDX-License-Identifier: Apache-2.0

version_h = configuration_data()

r = run_command([
	'git', 'describe',
	'--always',
	'--abbrev=0',
	'--dirty',
	'--match', '"NOT A TAG"'
], capture : true, check : true)
hash = r.stdout().strip()
version_h.set('AGENT_COMMIT_HASH', '"@0@"'.format(hash))

# Get AGENT_VERSION from version.mk
r_ver = run_command(
  ['sh', '-c', 'grep "^VERSION\\s*=\\s*" ../version.mk | sed -e "s/^VERSION\\s*=\\s*//"'],
  capture: true,
  check: true
)
version = r_ver.stdout().strip()
version_h.set('AGENT_VERSION', '"@0@"'.format(version))

configure_file(output : 'version.h', configuration : version_h)

# We need to generate config.h, to acheive which we're just going to rely on
# python and genconfig using the existing infrastructure
p = run_command(
	[
		'python3', '-m', 'genconfig', '--header-path',
		'@0@/include/config.h'.format(evp_build_root),
		 '--config-out', '@0@/include/.config'.format(evp_build_root),
	],
	env : [
		'srctree=@0@/src/libevp-agent/linux'.format(evp_source_root),
		'KCONFIG_CONFIG=@0@/configs/default.config'.format(evp_source_root),
	],
	check: true
)
