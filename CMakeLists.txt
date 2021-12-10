cmake_minimum_required(VERSION 3.10)
project(xania)

####
# Shell out to the Makefile to get all the paths it installed for us
set(CMAKE_MAKE_CONFIG_FILE ${CMAKE_CURRENT_BINARY_DIR}/config-from-make.cmake)
execute_process(
        COMMAND make cmake-print-deps CMAKE_CONFIG_FILE=${CMAKE_MAKE_CONFIG_FILE}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        RESULT_VARIABLE MAKE_CONFIG_FAIL)
if (MAKE_CONFIG_FAIL)
    message(FATAL_ERROR "Unable to get makefile configuration")
endif ()
include(${CMAKE_MAKE_CONFIG_FILE})
####

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
conan_check(VERSION 1.42.1 REQUIRED)

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
