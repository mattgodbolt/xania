add_library(doorman_lib STATIC
        Fd.cpp Fd.hpp Byte.hpp)
target_link_libraries(doorman_lib CONAN_PKG::fmt CONAN_PKG::ms-gsl)

add_executable(doorman doorman.c doorman.h)
install(TARGETS doorman RUNTIME DESTINATION bin)