cmake_minimum_required(VERSION 3.19)
project(xania)

set(BUILD_VERSION "(local build)" CACHE STRING "Build version (build number only)")
set(BUILD_FULL_VERSION "(local build)" CACHE STRING "Full string of build version (with git info)")
set(BUILD_COMMIT_URL "https://this.is.a.local.build/1234" CACHE STRING "Full string of git URL to this build")


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_compile_options(-Werror -Wall -Wextra)
# Regardless of the CMake settings, I always want us to have debug information available.
add_compile_options(-g)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=leak -fsanitize=address -fsanitize=undefined ")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG -Wno-error=array-bounds")

enable_testing()

if (USE_CONAN)
    include(conan/CMakeLists.txt)
endif ()

find_package(spdlog REQUIRED)
find_package(fmt REQUIRED)
find_package(range-v3 REQUIRED)
find_package(magic_enum REQUIRED)
find_package(date REQUIRED)
find_package(Microsoft.GSL REQUIRED)
find_package(Catch2 3 REQUIRED)
find_package(trompeloeil REQUIRED)
find_package(lyra REQUIRED)

add_subdirectory(src)
add_subdirectory(area)

install(DIRECTORY html DESTINATION .)
