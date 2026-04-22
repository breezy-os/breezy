
project('breezy-os', 'c')

global_inc = include_directories('include')

subdir('src')

executable('breezy',
    sources: ['./src/main.c'],
    include_directories: global_inc,
    dependencies: [utils_dep]
)
