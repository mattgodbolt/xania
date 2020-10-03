add_executable(query query.cpp)
target_link_libraries(query xania_lib CONAN_PKG::lyra)
target_compile_definitions(query PRIVATE DEFAULT_AREA_PATH="${CMAKE_SOURCE_DIR}/area")
