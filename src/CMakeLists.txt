configure_file(version.h.in generated_sources/version.h @ONLY)

add_library(version INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/generated_sources/version.h)
target_include_directories(version INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/generated_sources/)

add_subdirectory(common)
add_subdirectory(doorman)
add_subdirectory(chat)
add_subdirectory(test)
add_subdirectory(apps)

file(GLOB MudSources LIST_DIRECTORIES false CONFIGURE_DEPENDS *.h *.hpp *.cpp)

list(REMOVE_ITEM MudSources ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

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
target_link_libraries(xania PRIVATE xania_lib version)

install(TARGETS xania RUNTIME DESTINATION bin)
