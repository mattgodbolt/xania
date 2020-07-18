cmake_minimum_required(VERSION 3.10)
project(xania)

set(CMAKE_CXX_STANDARD 14)

add_compile_options(-Werror -Wall -Wextra)

add_subdirectory(src)

install(DIRECTORY area DESTINATION .)
install(DIRECTORY html DESTINATION .)
