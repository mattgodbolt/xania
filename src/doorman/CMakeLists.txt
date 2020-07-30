add_library(doorman_lib STATIC
        Doorman.cpp Doorman.hpp Channel.cpp Channel.hpp Xania.cpp Xania.hpp Fd.hpp Fd.cpp
        TelnetProtocol.cpp TelnetProtocol.hpp Logger.cpp Logger.hpp)
target_link_libraries(doorman_lib CONAN_PKG::fmt CONAN_PKG::ms-gsl CONAN_PKG::spdlog)

add_executable(doorman main.cpp)
target_link_libraries(doorman doorman_lib)

add_subdirectory(test)

install(TARGETS doorman RUNTIME DESTINATION bin)