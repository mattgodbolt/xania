#pragma once

/*
 * Char file format metadata constants.
 * Some of these keywords are terse because Merc or Diku seems
 * to have used a convention of keywords no more than four chars long.
 * The longer ones came later.
 */
namespace charfilemeta {

// The *Section symbols are all used to delimit major sections of a file and are all
// prefixed with #.  The other symbols are not.
static inline constexpr auto SectionMobile = "MOBILE";
static inline constexpr auto SectionPlayer = "PLAYER";
static inline constexpr auto SectionObject = "O";
static inline constexpr auto SectionPet = "PET";
// For unknown historical reasons, this is a subtle difference to "End", which is only used between sections within a
// file.
static inline constexpr auto SectionEnd = "END";
static inline constexpr auto End = "End";
static inline constexpr auto Name = "Name";
static inline constexpr auto Version = "Vers";
static inline constexpr auto ShortDescription = "ShD";
static inline constexpr auto LongDescription = "LnD";
static inline constexpr auto Description = "Desc";
static inline constexpr auto Race = "Race";
static inline constexpr auto Sex = "Sex";
static inline constexpr auto Class = "Cla";
static inline constexpr auto Level = "Levl";
static inline constexpr auto Clan = "Clan";
static inline constexpr auto ClanLevel = "CLevel";
static inline constexpr auto ClanChanFlags = "CCFlags";
static inline constexpr auto Trust = "Tru";
static inline constexpr auto PlayedTime = "Plyd";
static inline constexpr auto LastNote = "Note";
static inline constexpr auto Scroll = "Scro";
static inline constexpr auto Room = "Room";
static inline constexpr auto HitManaMove = "HMV";
static inline constexpr auto HitManaMovePerm = "HMVP";
static inline constexpr auto Gold = "Gold";
static inline constexpr auto Experience = "Exp";
static inline constexpr auto ActFlags = "Act";
static inline constexpr auto AffectedBy = "AfBy";
static inline constexpr auto CommFlags = "Comm";
static inline constexpr auto InvisLevel = "Invi";
static inline constexpr auto Position = "Pos";
static inline constexpr auto Practice = "Prac";
static inline constexpr auto Train = "Trai";
static inline constexpr auto SavingThrow = "Save";
static inline constexpr auto Alignment = "Alig";
static inline constexpr auto HitRoll = "Hit";
static inline constexpr auto DamRoll = "Dam";
static inline constexpr auto ArmourClasses = "ACs";
static inline constexpr auto Wimpy = "Wimp";
static inline constexpr auto Attribs = "Attr";
static inline constexpr auto AttribModifiers = "AMod";
static inline constexpr auto Vnum = "Vnum";
static inline constexpr auto Password = "Pass";
static inline constexpr auto BamfIn = "Bin";
static inline constexpr auto BamfOut = "Bout";
static inline constexpr auto Title = "Titl";
static inline constexpr auto Afk = "Afk";
static inline constexpr auto Colour = "Colo";
static inline constexpr auto Prompt = "Prmt";
static inline constexpr auto CreationPoints = "Pnts";
static inline constexpr auto TrueSex = "TSex";
static inline constexpr auto LastLevelTime = "LLev";
static inline constexpr auto InfoMsg = "Info_message";
static inline constexpr auto LastLoginFrom = "LastLoginFrom";
static inline constexpr auto LastLoginAt = "LastLoginAt";
static inline constexpr auto Prefix = "Prefix";
static inline constexpr auto HourOffset = "HourOffset";
static inline constexpr auto MinOffset = "MinOffset";
static inline constexpr auto ExtraBits = "ExtraBits";
static inline constexpr auto Condition = "Cond";
static inline constexpr auto PronounPossessive = "Possessive";
static inline constexpr auto PronounSubjective = "Subjective";
static inline constexpr auto PronounObjective = "Objective";
static inline constexpr auto PronounReflexive = "Reflexive";
static inline constexpr auto Skill = "Sk";
static inline constexpr auto SkillGroup = "Gr";
static inline constexpr auto Affected = "AffD";
static inline constexpr auto Ridden = "Ridden";
static inline constexpr auto Cost = "Cost";
static inline constexpr auto Enchanted = "Enchanted";
static inline constexpr auto ExtraFlags = "ExtF";
static inline constexpr auto ExtraDescription = "ExDe";
static inline constexpr auto ItemType = "Ityp";
static inline constexpr auto Nest = "Nest";
static inline constexpr auto Spell = "Spell";
static inline constexpr auto Time = "Time";
static inline constexpr auto Val = "Val";
static inline constexpr auto WearFlags = "WeaF";
static inline constexpr auto WearLoc = "Wear";
static inline constexpr auto WearString = "WStr";
static inline constexpr auto Weight = "Wt";
static inline constexpr auto ObjectLevel = "Lev"; // different to mob "Levl"! :(

}