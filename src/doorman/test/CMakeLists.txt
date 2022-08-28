file(GLOB SOURCE_FILES CONFIGURE_DEPENDS *.cpp *.hpp)
add_executable(doorman_test ${SOURCE_FILES})
target_link_libraries(doorman_test PRIVATE doorman_lib Catch2::Catch2WithMain trompeloeil::trompeloeil)
add_test(NAME doorman_test COMMAND $<TARGET_FILE:doorman_test>)
