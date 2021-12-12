configure_file(version.h.in version.h @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(common)
add_subdirectory(doorman)
add_subdirectory(chat)
add_subdirectory(test)
add_subdirectory(apps)

file(GLOB MudSources LIST_DIRECTORIES false CONFIGURE_DEPENDS *.h *.hpp *.cpp)

add_library(xania_lib STATIC ${MudSources})

target_link_libraries(xania_lib PRIVATE
        crypt
        chat
        CONAN_PKG::fmt
        CONAN_PKG::namedtype
        PUBLIC
        xania_common
        CONAN_PKG::date
        CONAN_PKG::range-v3
        CONAN_PKG::magic_enum
)
target_compile_options(xania_lib PUBLIC -fconcepts)

add_executable(xania main.cpp)
target_link_libraries(xania PRIVATE xania_lib)

install(TARGETS xania RUNTIME DESTINATION bin)
