
bz_utils_lib = static_library(
    'breezy-utils',
    sources: [
        'bz_logger.c',
        'bz_list.c',
    ],
    include_directories: utilities_inc)

bz_utils_dep = declare_dependency(link_with: bz_utils_lib, include_directories: utilities_inc)
