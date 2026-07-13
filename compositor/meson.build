
compositor_inc = include_directories('include')

subdir('src')
subdir('tests')

executable('breezy', './src/main.c',
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
        bz_main_dep,
        bz_utils_dep,
    ]
)
