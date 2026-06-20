
# Import the XDG shell protocol
wl_mod = import('wayland')
xdg_shell_xml = wl_mod.find_protocol('xdg-shell')
xdg_shell_code = wl_mod.scan_xml(
    xdg_shell_xml,
    client: false,
    server: true,
    public: false,
    include_core_only: false)

# Build our library/dependency
bz_main_lib = static_library(
    'breezy-main',
    sources: [
        './bz_seat.c',
        './bz_graphics.c',
        './bz_input.c',
        './bz_wayland.c',
        './gles2.c',
        './wayland/bz_wl_display.c',
        './wayland/bz_wl_devices.c',
        './wayland/bz_xdg_shell.c',
        xdg_shell_code,
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
    ])
bz_main_dep = declare_dependency(link_with: bz_main_lib, include_directories: compositor_inc)
