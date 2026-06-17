
# Import the XDG shell protocol
wl_mod = import('wayland')
xdg_shell_xml = wl_mod.find_protocol('xdg-shell')
xdg_shell_code = wl_mod.scan_xml(
    xdg_shell_xml,
    client: true,
    server: false,
    public: false,
    include_core_only: false)

# Build our library/dependency
bz_testclient_lib = static_library(
    'breezy-test-client',
    sources: [
        './bz_wl_protocol.c',
        './bz_client_utils.c',
        xdg_shell_code,
    ],
    include_directories: testclient_inc,
    dependencies: [
        dependency('wayland-client'),
        bz_utils_dep,
    ])
bz_testclient_dep = declare_dependency(link_with: bz_testclient_lib, include_directories: testclient_inc)
