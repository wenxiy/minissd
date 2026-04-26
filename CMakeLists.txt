cmake_minimum_required(VERSION 3.14)
project(MiniSSD VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 编译选项：开警告，方便排查问题
add_compile_options(-Wall -Wextra -Wpedantic)

# ---- NAND 底层（纯 C 静态库）----
add_library(nand_lib STATIC
    src/nand/nand_chip.c
    src/nand/nand_utils.c
)
target_include_directories(nand_lib PUBLIC include)

# ---- FTL + SSD 上层（C++ 静态库，后续 Week2 再填充源文件）----
# add_library(ssd_lib STATIC
#     src/ftl/ftl.cpp
#     src/ftl/mapping_table.cpp
#     src/ftl/garbage_collector.cpp
#     src/ftl/wear_leveler.cpp
#     src/ftl/bad_block_manager.cpp
#     src/ssd/ssd_controller.cpp
#     src/ssd/ssd_stats.cpp
# )
# target_include_directories(ssd_lib PUBLIC include)
# target_link_libraries(ssd_lib PUBLIC nand_lib)

# ---- 可执行文件 ----
add_executable(minissd
    src/main.cpp
)
target_include_directories(minissd PRIVATE include)
target_link_libraries(minissd PRIVATE nand_lib)

# ---- 测试（后续 Week4 再开启）----
# option(BUILD_TESTS "Build unit tests" ON)
# if(BUILD_TESTS)
#     enable_testing()
#     add_subdirectory(tests)
# endif()
