
subdir('wayland')

bz_main_lib = static_library(
    'breezy-main',
    sources: [
        './bz_seat.c',
        './bz_graphics.c',
        './bz_input.c',
        './bz_wayland.c',
        './gles2.c',
    ],
    include_directories: compositor_inc,
    dependencies: [
        dependency('libseat'),
        dependency('libdrm'),
        dependency('gbm'),
        dependency('egl'),
        dependency('libudev'),
        dependency('libinput'),
        dependency('xkbcommon'),
        dependency('wayland-server'),
        bz_utils_dep,
        bz_wayland_dep,
    ])
bz_main_dep = declare_dependency(link_with: bz_main_lib, include_directories: compositor_inc)
