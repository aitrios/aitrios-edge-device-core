quirc_sources = files([
	'decode.c',
	'identify.c',
	'quirc.c',
	'version_db.c'
])

quirc_includes = include_directories('.')

# Add a symlink to account for the include path used in the InitialSettingApp
# code
run_command(['mkdir', '-p', 'quirc/lib'], check : true)
run_command(['ln', '-fs', '../../quirc.h', 'quirc/lib/quirc.h'], check : true)
