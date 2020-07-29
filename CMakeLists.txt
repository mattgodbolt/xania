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

set(CMAKE_CXX_STANDARD 17)
add_compile_options(-Werror -Wall -Wextra)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address -fsanitize=undefined")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address -fsanitize=undefined")

include(scripts/conan.cmake)

conan_check(VERSION 1.27.1 REQUIRED)

conan_add_remote(NAME bincrafters INDEX 1
        URL https://api.bintray.com/conan/bincrafters/public-conan)
conan_cmake_run(CONANFILE conanfile.txt
        BASIC_SETUP CMAKE_TARGETS
        BUILD missing)

enable_testing()

add_subdirectory(src)

install(DIRECTORY area DESTINATION .)
install(DIRECTORY html DESTINATION .)
