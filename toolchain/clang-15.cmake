set(CMAKE_C_COMPILER clang CACHE FILEPATH "MUD C compiler")
set(CMAKE_CXX_COMPILER clang++ CACHE FILEPATH "MUD C++ compiler")
# CMAKE_CXX_FLAGS is _not_ setting "-stdlib=libc++" currently, it leads to memory leaks/ASAN troubles
