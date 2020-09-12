configure_file(version.h.in version.h @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(common)
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
        note.cpp
        note.h
        phil.cpp
        phil.h
        save.cpp save.hpp
        skills.cpp
        spec_concordius.cpp
        CorpseSummoner.hpp
        CorpseSummoner.cpp
        special.cpp
        tables.cpp
        tables.h
        update.cpp
        wiznet.cpp
        xania.cpp
        CommandSet.hpp
        Descriptor.cpp Descriptor.hpp
        string_utils.cpp string_utils.hpp string_utils_impl.hpp
        info.hpp
        TimeInfoData.cpp TimeInfoData.hpp
        WeatherData.cpp WeatherData.hpp
        WrappedFd.cpp WrappedFd.hpp
        nth.hpp nth.cpp
        DescriptorList.hpp DescriptorList.cpp
        DescriptorFilter.hpp
        Char.cpp Char.hpp
        Types.hpp
        Stats.hpp
        ExtraFlags.hpp
        PcData.hpp
        Constants.hpp
        handler.hpp
        ArgParser.cpp ArgParser.hpp
        Direction.cpp
        Format.hpp
        fight.hpp
        Pronouns.hpp
        AFFECT_DATA.cpp AFFECT_DATA.hpp
        AffectLocation.hpp
        AffectList.cpp AffectList.hpp
        Logging.cpp Logging.hpp
        AREA_DATA.cpp AREA_DATA.hpp
        Help.cpp Help.hpp
        Rng.cpp Rng.hpp
        MOB_INDEX_DATA.cpp MOB_INDEX_DATA.cpp
        Dice.cpp Dice.hpp)
target_link_libraries(xania_lib xania_common crypt chat CONAN_PKG::fmt CONAN_PKG::range-v3)
target_compile_options(xania_lib PUBLIC -fconcepts)

add_executable(xania main.cpp)
target_link_libraries(xania xania_lib)

install(TARGETS xania RUNTIME DESTINATION bin)
