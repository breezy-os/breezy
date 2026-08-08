
testclient_inc = include_directories('include')

subdir('src')

executable('test-client',
           './src/main.c',
           include_directories: testclient_inc,
           dependencies: [
               dependency('wayland-client'),
               dependency('xkbcommon'),
               bz_utils_dep,
               bz_testclient_dep,
           ],
)
