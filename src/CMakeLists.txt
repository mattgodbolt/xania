configure_file(version.h.in generated_sources/version.h @ONLY)

add_library(version INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/generated_sources/version.h)
target_include_directories(version INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/generated_sources/)

add_subdirectory(common)
add_subdirectory(doorman)
add_subdirectory(test)
add_subdirectory(apps)

file(GLOB MudSources LIST_DIRECTORIES false CONFIGURE_DEPENDS *.h *.hpp *.cpp)

list(REMOVE_ITEM MudSources ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

add_library(xania_lib STATIC ${MudSources})

target_link_libraries(xania_lib PRIVATE
        crypt
        fmt::fmt
        PUBLIC
        Microsoft.GSL::GSL
        xania_common
        date::date
        range-v3::range-v3
        spdlog::spdlog
        magic_enum::magic_enum
)
target_compile_options(xania_lib PUBLIC)

add_executable(xania main.cpp)
target_link_libraries(xania PRIVATE xania_lib version)

install(TARGETS xania RUNTIME DESTINATION bin)
