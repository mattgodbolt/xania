add_library(xania_common STATIC
        Configuration.hpp Configuration.cpp
        Fd.cpp Fd.hpp
        doorman_protocol.h
        mask_hostname.hpp mask_hostname.cpp
        Time.hpp
        BitOps.hpp
        StandardBits.hpp)
target_link_libraries(xania_common CONAN_PKG::fmt CONAN_PKG::ms-gsl CONAN_PKG::spdlog CONAN_PKG::date)
target_include_directories(xania_common PUBLIC ..)

add_subdirectory(test)
