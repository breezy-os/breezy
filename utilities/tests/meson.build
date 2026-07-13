
# Run each of our test suites
foreach t : [
    'bz_list',
    'bz_logger',
]
    exe = executable('test_' + t,
        'test_' + t + '.c',
        dependencies: [
            bz_utils_dep,
            unity_dep,
            fff_dep,
        ],
        include_directories: utilities_inc)
    test(t, exe)
endforeach