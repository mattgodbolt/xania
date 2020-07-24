add_executable(doorman doorman.cpp doorman.h)
target_link_libraries(doorman CONAN_PKG::fmt)
install(TARGETS doorman RUNTIME DESTINATION bin)