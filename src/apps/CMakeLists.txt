add_subdirectory(test)
add_executable(areas areas.cpp)
target_link_libraries(areas PRIVATE xania_lib CONAN_PKG::lyra)
target_compile_definitions(areas PRIVATE DEFAULT_AREA_PATH="${CMAKE_SOURCE_DIR}/area")

add_executable(map map.cpp)
target_link_libraries(map PRIVATE xania_lib CONAN_PKG::lyra)
target_compile_definitions(map PRIVATE DEFAULT_AREA_PATH="${CMAKE_SOURCE_DIR}/area")

add_library(pfu_lib STATIC pfu.cpp)
target_link_libraries(pfu_lib PRIVATE xania_common PUBLIC xania_lib CONAN_PKG::spdlog)

add_executable(pfu pfumain.cpp)
target_link_libraries(pfu PRIVATE pfu_lib xania_lib CONAN_PKG::lyra)

install(TARGETS pfu RUNTIME DESTINATION bin)
