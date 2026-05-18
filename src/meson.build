
subdir('utils')

main_lib = static_library(
    'breezy-main',
    sources: [
        './bz_seat.c',
        './bz_graphics.c',
        './bz_input.c',
        './gles2.c',
    ],
    include_directories: global_inc,
    dependencies: [
        dependency('libseat'),
        dependency('libdrm'),
        dependency('gbm'),
        dependency('egl'),
        dependency('libudev'),
        dependency('libinput'),
        dependency('xkbcommon'),
        utils_dep,
    ])
main_dep = declare_dependency(link_with: main_lib, include_directories: global_inc)
