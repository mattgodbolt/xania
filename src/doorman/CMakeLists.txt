add_library(doorman_lib STATIC
        Channel.cpp Channel.hpp
        Doorman.cpp Doorman.hpp
        IdAllocator.cpp IdAllocator.hpp
        Logger.cpp Logger.hpp
        TelnetProtocol.cpp TelnetProtocol.hpp
        Xania.cpp Xania.hpp)
target_link_libraries(doorman_lib
                       PUBLIC
                         xania_lib
                         spdlog::spdlog
                       PRIVATE
                         fmt::fmt
                         Microsoft.GSL::GSL
                         bfg::lyra)

add_executable(doorman main.cpp)
target_link_libraries(doorman PRIVATE doorman_lib version bfg::lyra)

add_subdirectory(test)

install(TARGETS doorman RUNTIME DESTINATION bin)
