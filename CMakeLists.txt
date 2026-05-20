cmake_minimum_required(VERSION 3.15)
project(BlackScholes)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows: install SQLite3 via vcpkg and pass -DCMAKE_TOOLCHAIN_FILE=<vcpkg>/scripts/buildsystems/vcpkg.cmake
find_package(SQLite3 REQUIRED)

add_executable(black_scholes
    src/main.cpp
    src/black_scholes.cpp
    src/database.cpp
)

target_include_directories(black_scholes PRIVATE ${SQLite3_INCLUDE_DIRS})
target_link_libraries(black_scholes PRIVATE ${SQLite3_LIBRARIES})

# Ensure the data/ directory exists next to the binary
add_custom_command(TARGET black_scholes POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
            "${CMAKE_SOURCE_DIR}/data/charts"
    COMMENT "Creating data/ directory"
)
