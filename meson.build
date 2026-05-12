
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

global_inc = include_directories('include')

subdir('src')
subdir('tests')

executable('breezy',
    sources: [
        './src/bz_seat.c',
        './src/bz_graphics.c',
        './src/bz_input.c',
        './src/gles2.c',
        './src/main.c',
    ],
    include_directories: global_inc,
    dependencies: [
        dependency('libdrm'),
        dependency('libseat'),
        dependency('libinput'),
        dependency('gbm'),
        dependency('egl'),
        utils_dep,
    ]
)
