add_subdirectory(test)
add_library(chat KeywordResponses.cpp KeywordResponses.hpp Database.hpp Database.cpp chatmain.cpp Eliza.cpp Eliza.hpp chat_utils.cpp chat_utils.hpp)
target_link_libraries(chat CONAN_PKG::fmt)
