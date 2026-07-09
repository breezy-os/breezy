
# Run each of our test suites
foreach t : [
    'bz_seat',
    # Wayland interfaces
    'bz_wl_compositor',
    'bz_wl_surface',
]
    exe = executable('test_' + t,
        'test_' + t + '.c',
        dependencies: [
            bz_main_dep,
            bz_utils_dep,
            unity_dep,
            fff_dep,
            dependency('libdrm'),
            dependency('libseat'),
        ],
        include_directories: compositor_inc)
    test(t, exe)
endforeach