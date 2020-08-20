add_library(chat KeywordResponses.cpp KeywordResponses.hpp Database.hpp Database.cpp chatmain.cpp Eliza.cpp Eliza.hpp)
target_link_libraries(chat CONAN_PKG::fmt)
