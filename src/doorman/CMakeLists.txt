add_library(doorman_lib STATIC
        Byte.hpp
        Channel.cpp Channel.hpp
        Doorman.cpp Doorman.hpp
        Fd.hpp Fd.cpp
        IdAllocator.cpp IdAllocator.hpp
        Logger.cpp Logger.hpp
        TelnetProtocol.cpp TelnetProtocol.hpp
        Xania.cpp Xania.hpp SeasocksToLogger.hpp)
target_link_libraries(doorman_lib CONAN_PKG::fmt CONAN_PKG::ms-gsl CONAN_PKG::spdlog CONAN_PKG::seasocks)

add_executable(doorman main.cpp)
target_link_libraries(doorman doorman_lib)

add_subdirectory(test)

install(TARGETS doorman RUNTIME DESTINATION bin)