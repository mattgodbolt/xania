include_directories(..)
file(GLOB SOURCE_FILES CONFIGURE_DEPENDS *.cpp *.hpp)
add_executable(common_test ${SOURCE_FILES})
target_link_libraries(common_test xania_common CONAN_PKG::catch2 CONAN_PKG::trompeloeil)
target_compile_definitions(common_test PUBLIC CATCH_CONFIG_ENABLE_ALL_STRINGMAKERS TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/test_data")
add_test(NAME common_test COMMAND $<TARGET_FILE:common_test>)
