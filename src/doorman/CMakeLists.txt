add_executable(doorman doorman.cpp doorman.h Channel.cpp Channel.hpp doorman.hpp Xania.cpp Xania.hpp Fd.hpp Fd.cpp)
target_link_libraries(doorman CONAN_PKG::fmt CONAN_PKG::ms-gsl)
install(TARGETS doorman RUNTIME DESTINATION bin)