cmake_minimum_required(VERSION 3.19)

# This CMakeList.txt is a wrapper around the main CMakeList.txt in the xania root
# directory. It aims to setup a full conan environment to ease the project
# build. Any conan-setup related thing should happen here in order to keep the
# top level CMakeList.txt conan agnostic. This eases maintenance as it separate
# concerns about setup and build itself, and will ease future packaging with
# standard distro packagers (rpm/deb/...).
#
project(xania-conan)

# Conan version to be used
set(CONDA_VERSION 4.11.0)

# Conda is used to have a minimal environment where to install conan
set(CONDA_INSTALLER_MD5 4e2f31e0b2598634c80daa12e4981647)
set(REQUIRED_CONAN_VERSION 1.47.0)

include(conan.cmake)

find_program(CONAN_CMD conan)

message("Looking for a conan version above ${REQUIRED_CONAN_VERSION}...")

if (CONAN_CMD)
   # Sadly, the conan macros make the whole cake fail when the version
   # does not match. This makes it difficult to deal with prior local install
   # of conan.
   # Here we trick the macro by giving a lower version that will always be good
   # (for instance 0.0.0) and then get back the parsed version in the
   # CMAKE_MATCH_1 variable
   conan_check(VERSION 0.0.0)
endif()

if (NOT CONAN_CMD OR "${CMAKE_MATCH_1}" VERSION_LESS "${REQUIRED_CONAN_VERSION}")
unset(CONAN_CMD CACHE)

message("Conan was not found or is too old; updating with Conda...")

set(TOOLS_DIR ${CMAKE_BINARY_DIR}/.tools/)
set(CONDA_ROOT ${TOOLS_DIR}/conda-${CONDA_VERSION})

if (EXISTS ${CMAKE_BINARY_DIR}/conda-${CONDA_VERSION}-installer.sh)
    file(MD5 ${CMAKE_BINARY_DIR}/conda-${CONDA_VERSION}-installer.sh LOCAL_CONDA_INSTALLER_MD5)
else()
    set(LOCAL_CONDA_INSTALLER_MD5 "none")
endif()

if (NOT "${LOCAL_CONDA_INSTALLER_MD5}" STREQUAL "${CONDA_INSTALLER_MD5}")
    message("Downloading conda installer...")
    file(DOWNLOAD https://repo.anaconda.com/miniconda/Miniconda3-py39_${CONDA_VERSION}-Linux-x86_64.sh ${CMAKE_BINARY_DIR}/conda-${CONDA_VERSION}-installer.sh
            EXPECTED_MD5 ${CONDA_INSTALLER_MD5})
    message("Conda dowloaded")
endif()

# Careful commands need to be separated as cmake builds a pipeline of commands
# (thus all processes are started at the same time). Here chmod NEEDS to happen
# before invoking the installer.
execute_process(
        COMMAND_ECHO STDOUT
        COMMAND chmod +x ${CMAKE_BINARY_DIR}/conda-${CONDA_VERSION}-installer.sh
        COMMAND_ERROR_IS_FATAL ANY)

execute_process(
        COMMAND_ECHO STDOUT
        COMMAND ${CMAKE_BINARY_DIR}/conda-${CONDA_VERSION}-installer.sh -u -b -p ${CONDA_ROOT}
        COMMAND_ERROR_IS_FATAL ANY)

message("Installing conan locally...")

execute_process(
        COMMAND_ECHO STDOUT
        COMMAND ${CONDA_ROOT}/bin/pip install conan==${REQUIRED_CONAN_VERSION}
        COMMAND_ERROR_IS_FATAL ANY)

# Force conan detection not using environment (thus using only conda path)
set(CMAKE_PROGRAM_PATH ${CONDA_ROOT}/bin ${CMAKE_PROGRAM_PATH})
find_program(CONAN_CMD conan REQUIRED
        NO_CMAKE_ENVIRONMENT_PATH
        NO_SYSTEM_ENVIRONMENT_PATH
        NO_CMAKE_SYSTEM_PATH
        NO_CMAKE_FIND_ROOT_PATH)

# paranoid check of conan version
conan_check(VERSION ${REQUIRED_CONAN_VERSION} REQUIRED)

endif()

# See Makefile - CONAN_VERSION

conan_add_remote(NAME conancenter
        INDEX 0
        URL https://center.conan.io)

conan_cmake_autodetect(CONAN_SETTINGS)

conan_cmake_install(PATH_OR_REFERENCE ${CMAKE_SOURCE_DIR}
        SETTINGS ${CONAN_SETTINGS}
        BUILD missing)

include(${CMAKE_BINARY_DIR}/conan_paths.cmake)

enable_testing()

# Now the environment is setup correctly, call the top level CMakeList.txt
add_subdirectory(${PROJECT_SOURCE_DIR}/.. ${CMAKE_BINARY_DIR}/xania)

