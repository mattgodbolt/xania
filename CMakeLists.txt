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

set(CMAKE_CXX_STANDARD 17)
add_compile_options(-Werror -Wall -Wextra)

# -static-libasan is required to prevent doorman+xania from crashing on startup (with some error about LD_PRELOAD)
# when you build with 'make'.
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=leak -fsanitize=address -fsanitize=undefined -static-libasan")

include(scripts/conan.cmake)

conan_check(VERSION 1.27.1 REQUIRED)

conan_add_remote(NAME bincrafters INDEX 1
        URL https://api.bintray.com/conan/bincrafters/public-conan)
conan_cmake_run(CONANFILE conanfile.txt
        BASIC_SETUP CMAKE_TARGETS
        BUILD missing)

set(CMAKE_INSTALL_MESSAGE NEVER) # prevent logging installation messages

enable_testing()

add_subdirectory(src)
add_subdirectory(area)

install(DIRECTORY html DESTINATION .)
