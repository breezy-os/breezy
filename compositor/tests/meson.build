
wl_mod = import('wayland')

# Import the XDG Shell protocol extension
xdg_shell_xml = wl_mod.find_protocol('xdg-shell')
xdg_shell_code = wl_mod.scan_xml(
    xdg_shell_xml,
    client: false,
    server: true,
    public: false,
    include_core_only: false)

# Import the WP Viewporter protocol extension
viewporter_xml = wl_mod.find_protocol('viewporter')
viewporter_code = wl_mod.scan_xml(
    viewporter_xml,
    client: false,
    server: true,
    public: false,
    include_core_only: false)


# (This isn't actually used anywhere, but is needed so our test helpers don't show up as
#   a file of nothing but red squiggles inside IntelliJ.)
bz_compositor_utils_lib = static_library(
    'compositor-test-utils',
    sources: [ 'helpers/bz_test_resources.c' ],
    include_directories: compositor_inc,
    dependencies: [
        bz_utils_dep,
        dependency('libdrm'),
    ],
)

# Run each of our test suites
foreach t : [
    'bz_seat',
    # Wayland interfaces (alphabetized!)
    'bz_wl_compositor',
    'bz_wl_data_device_manager',
    'bz_wl_keyboard',
    'bz_wl_pointer',
    'bz_wl_region',
    'bz_wl_seat',
    'bz_wl_subcompositor',
    'bz_wl_subsurface',
    'bz_wl_surface',
    'bz_wp_viewport',
    'bz_wp_viewporter',
    'bz_xdg_wm_base',
    'bz_xdg_surface',
    'bz_xdg_toplevel',
]
    exe = executable('test_' + t,
        'test_' + t + '.c',
        sources: [
            xdg_shell_code,
            viewporter_code,
        ],
        dependencies: [
            bz_main_dep,
            bz_utils_dep,
            unity_dep,
            fff_dep,
            dependency('libdrm'),
            dependency('libseat'),
            dependency('wayland-server'),
        ],
        include_directories: compositor_inc)
    test(t, exe)
endforeach