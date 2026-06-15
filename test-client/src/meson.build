
bz_testclient_lib = static_library(
    'breezy-test-client',
    sources: [
        './bz_wl_protocol.c',
    ],
    include_directories: testclient_inc,
    dependencies: [
        dependency('wayland-client'),
        bz_utils_dep,
    ])
bz_testclient_dep = declare_dependency(link_with: bz_testclient_lib, include_directories: testclient_inc)
