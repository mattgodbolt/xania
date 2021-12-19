cmake_minimum_required(VERSION 3.19)
project(xania)

set(BUILD_VERSION "(local build)" CACHE STRING "Build version (build number only)")
set(BUILD_FULL_VERSION "(local build)" CACHE STRING "Full string of build version (with git info)")
set(BUILD_COMMIT_URL "https://this.is.a.local.build/1234" CACHE STRING "Full string of git URL to this build")

set(CMAKE_CXX_STANDARD 20)
add_compile_options(-Werror -Wall -Wextra)
# Regardless of the CMake settings, I always want us to have debug information available.
add_compile_options(-g)

# -static-libasan is required to prevent doorman+xania from crashing on startup (with some error about LD_PRELOAD)
# when you build with 'make'.
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=leak -fsanitize=address -fsanitize=undefined -static-libasan")

include(scripts/conan.cmake)

# See Makefile - CONAN_VERSION 
message("If the following conan check fails; please remove your .tools directory")
conan_check(VERSION 1.43.1 REQUIRED)

conan_add_remote(NAME conancenter
        INDEX 0
        URL https://center.conan.io)

conan_cmake_autodetect(CONAN_SETTINGS)

conan_cmake_install(PATH_OR_REFERENCE ${CMAKE_SOURCE_DIR}
        SETTINGS ${CONAN_SETTINGS}
        BUILD missing)

include("${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
conan_basic_setup(TARGETS)

set(CMAKE_INSTALL_MESSAGE NEVER) # prevent logging installation messages

enable_testing()

add_subdirectory(src)
add_subdirectory(area)

install(DIRECTORY html DESTINATION .)
