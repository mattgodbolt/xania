add_executable(doorman doorman.cpp doorman.h Channel.cpp Channel.hpp doorman.hpp Xania.cpp Xania.hpp)
target_link_libraries(doorman CONAN_PKG::fmt)
install(TARGETS doorman RUNTIME DESTINATION bin)