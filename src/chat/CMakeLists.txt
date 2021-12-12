add_subdirectory(test)
add_library(chat KeywordResponses.cpp KeywordResponses.hpp Database.hpp Database.cpp chatmain.cpp Eliza.cpp Eliza.hpp chat_utils.cpp chat_utils.hpp)
target_link_libraries(
    chat
    PRIVATE
    CONAN_PKG::fmt
    CONAN_PKG::magic_enum
)
