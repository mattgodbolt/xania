cmake_minimum_required(VERSION 3.19)
project(xania)

set(BUILD_VERSION "(local build)" CACHE STRING "Build version (build number only)")
set(BUILD_FULL_VERSION "(local build)" CACHE STRING "Full string of build version (with git info)")
set(BUILD_COMMIT_URL "https://this.is.a.local.build/1234" CACHE STRING "Full string of git URL to this build")

set(CONDA_VERSION 4.10.3)
set(CONDA_INSTALLER_MD5 8c69f65a4ae27fb41df0fe552b4a8a3b)

set(REQUIRED_CONAN_VERSION 1.43.1)

include(scripts/conan.cmake)

find_program(CONAN_CMD conan)

if (CONAN_CMD)
   conan_check(VERSION)
endif()

if (NOT CONAN_CMD OR NOT "${_CONAN_VERSION_OUTPUT}" STREQUAL "Conan version ${REQUIRED_CONAN_VERSION}")

message("Updating Conan with Conda")

set(TOOLS_DIR ${CMAKE_BINARY_DIR}/.tools/)
set(CONDA_ROOT ${TOOLS_DIR}/conda-${CONDA_VERSION})

if (EXISTS conda-${CONDA_VERSION}-installer.sh)
    file(MD5 conda-${CONDA_VERSION}-installer.sh LOCAL_CONDA_INSTALLER_MD5)
endif()

if (NOT "${LOCAL_CONDA_INSTALLER_MD5}" STREQUAL "${CONDA_INSTALLER_MD5}")
    message("Downloading conda installer...")
    file(DOWNLOAD https://repo.anaconda.com/miniconda/Miniconda3-py39_${CONDA_VERSION}-Linux-x86_64.sh conda-${CONDA_VERSION}-installer.sh
            EXPECTED_MD5 ${CONDA_INSTALLER_MD5})
    message("Conda dowloaded")
endif()

execute_process(
        COMMAND chmod +x ${CMAKE_BINARY_DIR}/conda-${CONDA_VERSION}-installer.sh
        COMMAND ${CMAKE_BINARY_DIR}/conda-${CONDA_VERSION}-installer.sh -u -b -p ${CONDA_ROOT}
        COMMAND_ERROR_IS_FATAL ANY)
	
message("Installing conan locally...")

execute_process(
        COMMAND ${CONDA_ROOT}/bin/pip install conan==${REQUIRED_CONAN_VERSION}
        COMMAND_ERROR_IS_FATAL ANY)

set(CMAKE_PROGRAM_PATH ${CONDA_ROOT}/bin ${CMAKE_PROGRAM_PATH})
endif()

set(CMAKE_CXX_STANDARD 20)
add_compile_options(-Werror -Wall -Wextra)
# Regardless of the CMake settings, I always want us to have debug information available.
add_compile_options(-g)

# -static-libasan is required to prevent doorman+xania from crashing on startup (with some error about LD_PRELOAD)
# when you build with 'make'.
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=leak -fsanitize=address -fsanitize=undefined -static-libasan")

include(scripts/conan.cmake)

# See Makefile - CONAN_VERSION 

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
