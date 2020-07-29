add_library(doorman_lib STATIC
        Doorman.cpp Doorman.hpp Channel.cpp Channel.hpp Xania.cpp Xania.hpp Fd.hpp Fd.cpp Misc.cpp TelnetLineParser.cpp TelnetLineParser.hpp)
target_link_libraries(doorman_lib CONAN_PKG::fmt CONAN_PKG::ms-gsl)

add_executable(doorman main.cpp)
target_link_libraries(doorman doorman_lib)

install(TARGETS doorman RUNTIME DESTINATION bin)