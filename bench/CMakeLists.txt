cmake_minimum_required(VERSION 3.11)
project(litehtml_bench LANGUAGES C CXX)

# Build litehtml as a subdirectory
set(LITEHTML_BUILD_TESTING OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_BINARY_DIR}/litehtml)

# Benchmark executable
add_executable(litehtml_bench
    litehtml_bench.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../containers/test/test_container.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../containers/test/Font.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../containers/test/Bitmap.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../containers/test/lodepng.cpp
)

target_include_directories(litehtml_bench PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../containers/test
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(litehtml_bench PRIVATE litehtml)

set_target_properties(litehtml_bench PROPERTIES
    CXX_STANDARD 17
    C_STANDARD 99
)

# Copy test data to build directory
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/test_page.html ${CMAKE_CURRENT_BINARY_DIR}/test_page.html COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/test_styles.css ${CMAKE_CURRENT_BINARY_DIR}/test_styles.css COPYONLY)
