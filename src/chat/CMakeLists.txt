add_subdirectory(test)
add_library(chat KeywordResponses.cpp KeywordResponses.hpp Database.hpp Database.cpp chatmain.cpp Eliza.cpp Eliza.hpp chat_utils.cpp chat_utils.hpp)

# FIXME here including .. is preventing clear isolation of APIs: One may use
# this path to pull some other header... This should be reworked so that public
# headers are clearly isolated from any other so that one lib does not pull
# accidently any other lib header.
target_include_directories(chat PUBLIC ..)

target_link_libraries(
    chat
    PRIVATE
    fmt::fmt
    magic_enum::magic_enum
)
