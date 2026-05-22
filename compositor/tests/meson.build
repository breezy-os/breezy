
# Configure Unity as a dependency
unity_inc = include_directories('unity')
unity_lib = static_library('unity', 'unity/unity.c', include_directories: unity_inc)
unity_dep = declare_dependency(link_with: unity_lib, include_directories: unity_inc)

# Configure FFF as a dependency
fff_dep = declare_dependency(include_directories: include_directories('fff'))

# Run each of our test suites
foreach t : [
    'bz_list',
    'bz_logger',
    'bz_seat',
]
    exe = executable('test_' + t,
        'test_' + t + '.c',
        dependencies: [
            bz_main_dep,
            unity_dep,
            fff_dep,
            dependency('libdrm'),
            dependency('libseat'),
        ],
        include_directories: global_inc)
    test(t, exe)
endforeach