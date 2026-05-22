
project('breezy-os', 'c', default_options: [
    'c_std=c23',
    'warning_level=3',
    #    'buildtype=debugoptimized', # SETS -O2, which breaks some leak detections
    'buildtype=debug',
    'b_sanitize=address,undefined',
    'b_lundef=false',
])

# Set a bunch of compiler build flags for code analysis
cc = meson.get_compiler('c')
extra_warnings = cc.get_supported_arguments([
    '-Wshadow',
    '-Wformat=2',
    '-Wconversion',
    '-Wsign-conversion',
    '-Wundef',
    '-Wnull-dereference',
    '-Wdouble-promotion',
    '-Wimplicit-fallthrough',
    '-Wstack-protector',
    '-fstack-protector-strong',
])
add_project_arguments(extra_warnings, language: 'c')

subdir('compositor')
subdir('test-client')
