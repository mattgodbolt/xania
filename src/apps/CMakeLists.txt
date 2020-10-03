add_executable(areas areas.cpp)
target_link_libraries(areas xania_lib CONAN_PKG::lyra)
target_compile_definitions(areas PRIVATE DEFAULT_AREA_PATH="${CMAKE_SOURCE_DIR}/area")

add_executable(map map.cpp)
target_link_libraries(map xania_lib CONAN_PKG::lyra)
target_compile_definitions(map PRIVATE DEFAULT_AREA_PATH="${CMAKE_SOURCE_DIR}/area")
