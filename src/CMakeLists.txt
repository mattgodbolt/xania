configure_file(version.h.in version.h @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(common)
add_subdirectory(doorman)
add_subdirectory(chat)
add_subdirectory(test)
add_subdirectory(apps)

add_library(xania_lib STATIC
        act_comm.hpp act_comm.cpp
        act_info.cpp
        act_move.hpp act_move.cpp
        act_obj.cpp
        act_wiz.hpp act_wiz.cpp
        ban.cpp
        buffer.cpp
        buffer.h
        challenge.cpp
        challenge.hpp
        channels.cpp
        clan.cpp
        clan.h
        comm.cpp
        Attacks.hpp Attacks.cpp
        Classes.hpp Classes.cpp
        Materials.hpp Materials.cpp
        Races.hpp Races.cpp
        Titles.hpp Titles.cpp
        Socials.hpp Socials.cpp
        ExtraDescription.hpp
        ObjectIndex.hpp
        Object.hpp
        Exit.hpp
        Room.hpp
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
        Note.cpp
        Note.hpp
        phil.cpp
        phil.h
        ride.hpp ride.cpp
        save.cpp save.hpp
        SkillNumbers.hpp
        SkillTables.hpp SkillTables.cpp
        skills.cpp
        spec_concordius.cpp
        CorpseSummoner.hpp
        CorpseSummoner.cpp
        special.cpp
        tables.cpp
        tables.h
        update.hpp update.cpp
        wiznet.cpp
        entity_balance.cpp
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
        Stats.cpp Stats.hpp
        ExtraFlags.hpp ExtraFlags.cpp
        PcNutrition.hpp PcNutrition.cpp
        PcData.hpp
        Constants.hpp
        handler.hpp
        ArgParser.cpp ArgParser.hpp
        Direction.cpp
        Format.hpp
        fight.hpp
        Pronouns.hpp
        Pronouns.cpp
        AFFECT_DATA.cpp AFFECT_DATA.hpp
        AffectLocation.hpp
        AffectList.cpp AffectList.hpp
        Logging.cpp Logging.hpp
        AREA_DATA.cpp AREA_DATA.hpp
        Help.cpp Help.hpp
        Rng.cpp Rng.hpp
        MobIndexData.cpp MobIndexData.cpp
        ResetData.hpp
        Dice.cpp Dice.hpp
        SectorType.cpp SectorType.hpp
        PerEnum.hpp
        GenericList.hpp
        Sex.hpp Sex.cpp
        Tip.cpp Tip.hpp
        Position.hpp Position.cpp
        InjuredPart.hpp InjuredPart.cpp
        DamageMessages.hpp DamageMessages.cpp)
target_link_libraries(
        xania_lib
        xania_common
        crypt
        chat
        CONAN_PKG::fmt
        CONAN_PKG::range-v3
        CONAN_PKG::magic_enum
        CONAN_PKG::namedtype
)
target_compile_options(xania_lib PUBLIC -fconcepts)

add_executable(xania main.cpp)
target_link_libraries(xania xania_lib)

install(TARGETS xania RUNTIME DESTINATION bin)
