add_subdirectory(doorman)
add_subdirectory(chat)
add_subdirectory(test)
add_library(xania_lib STATIC
        act_comm.c
        act_info.c
        act_move.c
        act_obj.c
        act_wiz.c
        ban.c
        buffer.c
        buffer.h
        challeng.c
        challeng.h
        channels.c
        clan.c
        clan.h
        comm.c
        const.c
        db2.c
        db.c
        db.h
        fight.c
        flags.c
        flags.h
        handler.c
        healer.c
        info.c
        interp.c
        interp.h
        lookup.c
        lookup.h
        magic2.c
        magic.c
        magic.h
        merc.h
        mob_commands.c
        mob_prog.c
        news.c
        news.h
        note.c
        note.h
        phil.c
        phil.h
        save.c
        skills.c
        special.c
        tables.c
        tables.h
        trie.c
        trie.h
        update.c
        wiznet.c
        xania.c
        Wrapper.cpp
        )
target_link_libraries(xania_lib crypt chat)

add_executable(xania main.c)
target_link_libraries(xania xania_lib)

install(TARGETS xania RUNTIME DESTINATION bin)
