add_library(doorman_lib STATIC
        IdAllocator.cpp IdAllocator.hpp
        TelnetProtocol.cpp TelnetProtocol.hpp
        Logger.cpp Logger.hpp
        Fd.cpp Fd.hpp
        Byte.hpp)
target_link_libraries(doorman_lib CONAN_PKG::fmt CONAN_PKG::ms-gsl CONAN_PKG::spdlog)

add_subdirectory(test)

add_executable(doorman doorman.c doorman.h)
install(TARGETS doorman RUNTIME DESTINATION bin)