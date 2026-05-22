
wl_mod = import('wayland')

xdg_shell_xml = wl_mod.find_protocol('xdg-shell')
xdg_shell_code = wl_mod.scan_xml(
    xdg_shell_xml,
    client: false,
    server: true,
    public: false,
    include_core_only: false)

bz_wayland_lib = static_library('bz-wayland',
    'bz_wl_display.c',
    'bz_wl_devices.c',
    'bz_xdg_shell.c',
    xdg_shell_code,
    include_directories: global_inc)

bz_wayland_dep = declare_dependency(link_with: bz_wayland_lib, include_directories: global_inc)
