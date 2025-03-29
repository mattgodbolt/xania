cmake_minimum_required(VERSION 3.28)
include(${CMAKE_SOURCE_DIR}/conan/conan_provider.cmake)

# Conan version to be used
set(CONDA_VERSION 25.1.1-2)

# Conda is used to have a minimal environment where to install conan
set(CONDA_INSTALLER_SHA256 4766d85b5f7d235ce250e998ebb5a8a8210cbd4f2b0fea4d2177b3ed9ea87884)
set(REQUIRED_CONAN_VERSION 2.14.0)


find_program(CONAN_CMD conan)

message("Looking for a conan version above ${REQUIRED_CONAN_VERSION}...")

if (NOT CONAN_CMD)
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
    file(DOWNLOAD https://repo.anaconda.com/miniconda/Miniconda3-py312_${CONDA_VERSION}-Linux-x86_64.sh ${CMAKE_BINARY_DIR}/conda-${CONDA_VERSION}-installer.sh
            EXPECTED_HASH SHA256=${CONDA_INSTALLER_SHA256})
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

endif()

# Per-dependency settings overrides.
# The default static lib shipped by catch2 is compiled with std 14 and this is binary incompatible
# with some of the features our tests use, so force recompilation.
list(APPEND CONAN_SETTINGS "catch2:compiler.cppstd=20")

