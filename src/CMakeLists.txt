configure_file(version.h.in version.h @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(doorman)
add_subdirectory(chat)
add_subdirectory(test)
add_library(xania_lib STATIC
        act_comm.cpp
        act_info.cpp
        act_move.cpp
        act_obj.cpp
        act_wiz.cpp
        ban.cpp
        buffer.cpp
        buffer.h
        challeng.cpp
        challeng.h
        channels.cpp
        clan.cpp
        clan.h
        comm.cpp
        const.cpp
        db2.cpp
        db.cpp
        db.h
        fight.cpp
        flags.cpp
        flags.h
        handler.cpp
        healer.cpp
        info.cpp
        interp.cpp
        interp.h
        lookup.cpp
        lookup.h
        magic2.cpp
        magic.cpp
        magic.h
        merc.h
        mob_commands.cpp
        mob_prog.cpp
        news.cpp
        news.h
        note.cpp
        note.h
        phil.cpp
        phil.h
        save.cpp
        skills.cpp
        special.cpp
        tables.cpp
        tables.h
        trie.cpp
        trie.h
        update.cpp
        wiznet.cpp
        xania.cpp
        DescriptorData.cpp DescriptorData.hpp
        Wrapper.cpp
        string_utils.cpp string_utils.hpp)
target_link_libraries(xania_lib crypt chat)

add_executable(xania main.cpp)
target_link_libraries(xania xania_lib)

install(TARGETS xania RUNTIME DESTINATION bin)
