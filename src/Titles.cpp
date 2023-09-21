/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2023 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "Titles.hpp"
#include "Char.hpp"
#include "Class.hpp"
#include "Constants.hpp"

#include <algorithm>
#include <array>

namespace Titles {

using namespace std::literals;

struct Pair {
    constexpr Pair(std::string_view male, std::string_view female) : male_(male), female_(female) {}
    [[nodiscard]] std::string_view title(const Char &ch) const { return ch.sex.is_male() ? male_ : female_; }
    std::string_view male_;
    std::string_view female_;
};

constexpr std::array<std::array<Pair, MAX_LEVEL + 1>, MAX_CLASS> AllTitles = {{
    {
        {
            {"Man"sv, "Woman"sv},
            {"Apprentice of Magic"sv, "Apprentice of Magic"sv},
            {"Apprentice of Magic"sv, "Apprentice of Magic"sv},
            {"Apprentice of Magic"sv, "Apprentice of Magic"sv},
            {"Spell Student"sv, "Spell Student"sv},
            {"Spell Student"sv, "Spell Student"sv},

            {"Spell Student"sv, "Spell Student"sv},
            {"Scholar of Magic"sv, "Scholar of Magic"sv},
            {"Scholar of Magic"sv, "Scholar of Magic"sv},
            {"Scholar of Magic"sv, "Scholar of Magic"sv},
            {"Delver in Spells"sv, "Delveress in Spells"sv},

            {"Delver in Spells"sv, "Delveress in Spells"sv},
            {"Delver in Spells"sv, "Delveress in Spells"sv},
            {"Medium of Magic"sv, "Medium of Magic"sv},
            {"Medium of Magic"sv, "Medium of Magic"sv},
            {"Medium of Magic"sv, "Medium of Magic"sv},

            {"Scribe of Magic"sv, "Scribess of Magic"sv},
            {"Scribe of Magic"sv, "Scribess of Magic"sv},
            {"Scribe of Magic"sv, "Scribess of Magic"sv},
            {"Seer"sv, "Seeress"sv},
            {"Seer"sv, "Seeress"sv},

            {"Seer"sv, "Seeress"sv},
            {"Sage"sv, "Sage"sv},
            {"Sage"sv, "Sage"sv},
            {"Sage"sv, "Sage"sv},
            {"Illusionist"sv, "Illusionist"sv},

            {"Illusionist"sv, "Illusionist"sv},
            {"Illusionist"sv, "Illusionist"sv},
            {"Abjurer"sv, "Abjuress"sv},
            {"Abjurer"sv, "Abjuress"sv},
            {"Abjurer"sv, "Abjuress"sv},

            {"Invoker"sv, "Invoker"sv},
            {"Invoker"sv, "Invoker"sv},
            {"Invoker"sv, "Invoker"sv},
            {"Enchanter"sv, "Enchantress"sv},
            {"Enchanter"sv, "Enchantress"sv},

            {"Enchanter"sv, "Enchantress"sv},
            {"Conjurer"sv, "Conjuress"sv},
            {"Conjurer"sv, "Conjuress"sv},
            {"Conjurer"sv, "Conjuress"sv},
            {"Magician"sv, "Witch"sv},

            {"Magician"sv, "Witch"sv},
            {"Magician"sv, "Witch"sv},
            {"Creator"sv, "Creator"sv},
            {"Creator"sv, "Creator"sv},
            {"Creator"sv, "Creator"sv},

            {"Savant"sv, "Savant"sv},
            {"Savant"sv, "Savant"sv},
            {"Savant"sv, "Savant"sv},
            {"Magus"sv, "Craftess"sv},
            {"Magus"sv, "Craftess"sv},

            {"Magus"sv, "Craftess"sv},
            {"Wizard"sv, "Wizard"sv},
            {"Wizard"sv, "Wizard"sv},
            {"Wizard"sv, "Wizard"sv},
            {"Warlock"sv, "War Witch"sv},

            {"Warlock"sv, "War Witch"sv},
            {"Warlock"sv, "War Witch"sv},
            {"Sorcerer"sv, "Sorceress"sv},
            {"Sorcerer"sv, "Sorceress"sv},
            {"Sorcerer"sv, "Sorceress"sv},

            {"Elder Sorcerer"sv, "Elder Sorceress"sv},
            {"Elder Sorcerer"sv, "Elder Sorceress"sv},
            {"Elder Sorcerer"sv, "Elder Sorceress"sv},
            {"Grand Sorcerer"sv, "Grand Sorceress"sv},
            {"Grand Sorcerer"sv, "Grand Sorceress"sv},

            {"Great Sorcerer"sv, "Great Sorceress"sv},
            {"Great Sorcerer"sv, "Great Sorceress"sv},
            {"Great Sorcerer"sv, "Great Sorceress"sv},
            {"Golem Maker"sv, "Golem Maker"sv},
            {"Golem Maker"sv, "Golem Maker"sv},

            {"Greater Golem Maker"sv, "Greater Golem Maker"sv},
            {"Greater Golem Maker"sv, "Greater Golem Maker"sv},
            {"Maker of Stones"sv, "Maker of Stones"sv},
            {"Maker of Stones"sv, "Maker of Stones"sv},
            {"Maker of Potions"sv, "Maker of Potions"sv},

            {"Maker of Potions"sv, "Maker of Potions"sv},
            {"Maker of Scrolls"sv, "Maker of Scrolls"sv},
            {"Maker of Scrolls"sv, "Maker of Scrolls"sv},
            {"Maker of Wands"sv, "Maker of Wands"sv},
            {"Maker of Wands"sv, "Maker of Wands"sv},

            {"Maker of Staves"sv, "Maker of Staves"sv},
            {"Maker of Staves"sv, "Maker of Staves"sv},
            {"Demon Summoner"sv, "Demon Summoner"sv},
            {"Demon Summoner"sv, "Demon Summoner"sv},
            {"Greater Demon Summoner"sv, "Greater Demon Summoner"sv},

            {"Greater Demon Summoner"sv, "Greater Demon Summoner"sv},
            {"Dragon Charmer"sv, "Dragon Charmer"sv},
            {"Dragon Charmer"sv, "Dragon Charmer"sv},
            {"Greater Dragon Charmer"sv, "Greater Dragon Charmer"sv},
            {"Master of all Magic"sv, "Master of all Magic"sv},

            {"Master Mage"sv, "Master Mage"sv},
            {"Avatar of Magic"sv, "Avatar of Magic"sv},
            {"Angel of Magic"sv, "Angel of Magic"sv},
            {"Demigod of Magic"sv, "Demigoddess of Magic"sv},
            {"Immortal of Magic"sv, "Immortal of Magic"sv},

            {"God of Magic"sv, "Goddess of Magic"sv},
            {"Deity of Magic"sv, "Deity of Magic"sv},
            {"Supremity of Magic"sv, "Supremity of Magic"sv},
            {"Creator"sv, "Creator"sv},
            {"Implementor"sv, "Implementress"sv},
        },
    },
    {
        {
            {"Man"sv, "Woman"sv},
            {"Believer"sv, "Believer"sv},
            {"Believer"sv, "Believer"sv},
            {"Believer"sv, "Believer"sv},
            {"Attendant"sv, "Attendant"sv},
            {"Attendant"sv, "Attendant"sv},

            {"Attendant"sv, "Attendant"sv},
            {"Acolyte"sv, "Acolyte"sv},
            {"Acolyte"sv, "Acolyte"sv},
            {"Acolyte"sv, "Acolyte"sv},
            {"Novice"sv, "Novice"sv},

            {"Novice"sv, "Novice"sv},
            {"Novice"sv, "Novice"sv},
            {"Missionary"sv, "Missionary"sv},
            {"Missionary"sv, "Missionary"sv},
            {"Missionary"sv, "Missionary"sv},

            {"Adept"sv, "Adept"sv},
            {"Adept"sv, "Adept"sv},
            {"Adept"sv, "Adept"sv},
            {"Deacon"sv, "Deaconess"sv},
            {"Deacon"sv, "Deaconess"sv},

            {"Deacon"sv, "Deaconess"sv},
            {"Vicar"sv, "Vicaress"sv},
            {"Vicar"sv, "Vicaress"sv},
            {"Vicar"sv, "Vicaress"sv},
            {"Priest"sv, "Priestess"sv},

            {"Priest"sv, "Priestess"sv},
            {"Priest"sv, "Priestess"sv},
            {"Priest"sv, "Priestess"sv},
            {"Minister"sv, "Lady Minister"sv},
            {"Minister"sv, "Lady Minister"sv},

            {"Minister"sv, "Lady Minister"sv},
            {"Canon"sv, "Canon"sv},
            {"Canon"sv, "Canon"sv},
            {"Canon"sv, "Canon"sv},
            {"Levite"sv, "Levitess"sv},

            {"Levite"sv, "Levitess"sv},
            {"Levite"sv, "Levitess"sv},
            {"Curate"sv, "Curess"sv},
            {"Curate"sv, "Curess"sv},
            {"Curate"sv, "Curess"sv},

            {"Monk"sv, "Nun"sv},
            {"Monk"sv, "Nun"sv},
            {"Monk"sv, "Nun"sv},
            {"Healer"sv, "Healess"sv},
            {"Healer"sv, "Healess"sv},

            {"Healer"sv, "Healess"sv},
            {"Chaplain"sv, "Chaplain"sv},
            {"Chaplain"sv, "Chaplain"sv},
            {"Chaplain"sv, "Chaplain"sv},
            {"Expositor"sv, "Expositress"sv},

            {"Expositor"sv, "Expositress"sv},
            {"Expositor"sv, "Expositress"sv},
            {"Bishop"sv, "Bishop"sv},
            {"Bishop"sv, "Bishop"sv},
            {"Bishop"sv, "Bishop"sv},

            {"Arch Bishop"sv, "Arch Lady of the Church"sv},
            {"Arch Bishop"sv, "Arch Lady of the Church"sv},
            {"Arch Bishop"sv, "Arch Lady of the Church"sv},
            {"Patriarch"sv, "Matriarch"sv},
            {"Patriarch"sv, "Matriarch"sv},

            {"Patriarch"sv, "Matriarch"sv},
            {"Elder Patriarch"sv, "Elder Matriarch"sv},
            {"Elder Patriarch"sv, "Elder Matriarch"sv},
            {"Elder Patriarch"sv, "Elder Matriarch"sv},
            {"Grand Patriarch"sv, "Grand Matriarch"sv},

            {"Grand Patriarch"sv, "Grand Matriarch"sv},
            {"Great Patriarch"sv, "Great Matriarch"sv},
            {"Great Patriarch"sv, "Great Matriarch"sv},
            {"Great Patriarch"sv, "Great Matriarch"sv},
            {"Demon Killer"sv, "Demon Killer"sv},

            {"Demon Killer"sv, "Demon Killer"sv},
            {"Demon Killer"sv, "Demon Killer"sv},
            {"Greater Demon Killer"sv, "Greater Demon Killer"sv},
            {"Greater Demon Killer"sv, "Greater Demon Killer"sv},
            {"Greater Demon Killer"sv, "Greater Demon Killer"sv},

            {"Cardinal of the Sea"sv, "Cardinal of the Sea"sv},
            {"Cardinal of the Sea"sv, "Cardinal of the Sea"sv},
            {"Cardinal of the Earth"sv, "Cardinal of the Earth"sv},
            {"Cardinal of the Earth"sv, "Cardinal of the Earth"sv},
            {"Cardinal of the Air"sv, "Cardinal of the Air"sv},

            {"Cardinal of the Air"sv, "Cardinal of the Air"sv},
            {"Cardinal of the Ether"sv, "Cardinal of the Ether"sv},
            {"Cardinal of the Ether"sv, "Cardinal of the Ether"sv},
            {"Cardinal of the Heavens"sv, "Cardinal of the Heavens"sv},
            {"Cardinal of the Heavens"sv, "Cardinal of the Heavens"sv},

            {"Avatar of an Immortal"sv, "Avatar of an Immortal"sv},
            {"Avatar of a Deity"sv, "Avatar of a Deity"sv},
            {"Avatar of a Supremity"sv, "Avatar of a Supremity"sv},
            {"Avatar of an Implementor"sv, "Avatar of an Implementor"sv},
            {"Master of all Divinity"sv, "Mistress of all Divinity"sv},

            {"Holy Hero"sv, "Holy Heroine"sv},
            {"Holy Avatar"sv, "Holy Avatar"sv},
            {"Angel"sv, "Angel"sv},
            {"Demigod"sv, "Demigoddess"sv},
            {"Immortal"sv, "Immortal"sv},

            {"God"sv, "Goddess"sv},
            {"Deity"sv, "Deity"sv},
            {"Supreme Master"sv, "Supreme Mistress"sv},
            {"Creator"sv, "Creator"sv},
            {"Implementor"sv, "Implementress"sv},
        },
    },
    {
        {
            {"Man"sv, "Woman"sv},
            {"Swordpupil"sv, "Swordpupil"sv},
            {"Swordpupil"sv, "Swordpupil"sv},
            {"Swordpupil"sv, "Swordpupil"sv},
            {"Recruit"sv, "Recruit"sv},
            {"Recruit"sv, "Recruit"sv},

            {"Recruit"sv, "Recruit"sv},
            {"Recruit"sv, "Recruit"sv},
            {"Sentry"sv, "Sentress"sv},
            {"Sentry"sv, "Sentress"sv},
            {"Sentry"sv, "Sentress"sv},

            {"Fighter"sv, "Fighter"sv},
            {"Fighter"sv, "Fighter"sv},
            {"Fighter"sv, "Fighter"sv},
            {"Soldier"sv, "Soldier"sv},
            {"Soldier"sv, "Soldier"sv},

            {"Soldier"sv, "Soldier"sv},
            {"Warrior"sv, "Warrior"sv},
            {"Warrior"sv, "Warrior"sv},
            {"Warrior"sv, "Warrior"sv},
            {"Veteran"sv, "Veteran"sv},

            {"Veteran"sv, "Veteran"sv},
            {"Veteran"sv, "Veteran"sv},
            {"Swordsman"sv, "Swordswoman"sv},
            {"Swordsman"sv, "Swordswoman"sv},
            {"Swordsman"sv, "Swordswoman"sv},

            {"Fencer"sv, "Fenceress"sv},
            {"Fencer"sv, "Fenceress"sv},
            {"Fencer"sv, "Fenceress"sv},
            {"Combatant"sv, "Combatess"sv},
            {"Combatant"sv, "Combatess"sv},

            {"Combatant"sv, "Combatess"sv},
            {"Hero"sv, "Heroine"sv},
            {"Hero"sv, "Heroine"sv},
            {"Hero"sv, "Heroine"sv},
            {"Myrmidon"sv, "Myrmidon"sv},

            {"Myrmidon"sv, "Myrmidon"sv},
            {"Swashbuckler"sv, "Swashbuckleress"sv},
            {"Swashbuckler"sv, "Swashbuckleress"sv},
            {"Swashbuckler"sv, "Swashbuckleress"sv},
            {"Mercenary"sv, "Mercenaress"sv},

            {"Mercenary"sv, "Mercenaress"sv},
            {"Mercenary"sv, "Mercenaress"sv},
            {"Swordmaster"sv, "Swordmistress"sv},
            {"Swordmaster"sv, "Swordmistress"sv},
            {"Swordmaster"sv, "Swordmistress"sv},

            {"Lieutenant"sv, "Lieutenant"sv},
            {"Lieutenant"sv, "Lieutenant"sv},
            {"Lieutenant"sv, "Lieutenant"sv},
            {"Champion"sv, "Lady Champion"sv},
            {"Champion"sv, "Lady Champion"sv},

            {"Champion"sv, "Lady Champion"sv},
            {"Dragoon"sv, "Lady Dragoon"sv},
            {"Dragoon"sv, "Lady Dragoon"sv},
            {"Dragoon"sv, "Lady Dragoon"sv},
            {"Cavalier"sv, "Lady Cavalier"sv},

            {"Cavalier"sv, "Lady Cavalier"sv},
            {"Cavalier"sv, "Lady Cavalier"sv},
            {"Knight"sv, "Lady Knight"sv},
            {"Knight"sv, "Lady Knight"sv},
            {"Knight"sv, "Lady Knight"sv},

            {"Grand Knight"sv, "Grand Knight"sv},
            {"Grand Knight"sv, "Grand Knight"sv},
            {"Grand Knight"sv, "Grand Knight"sv},
            {"Master Knight"sv, "Master Knight"sv},
            {"Master Knight"sv, "Master Knight"sv},

            {"Paladin"sv, "Paladin"sv},
            {"Paladin"sv, "Paladin"sv},
            {"Grand Paladin"sv, "Grand Paladin"sv},
            {"Grand Paladin"sv, "Grand Paladin"sv},
            {"Demon Slayer"sv, "Demon Slayer"sv},

            {"Demon Slayer"sv, "Demon Slayer"sv},
            {"Greater Demon Slayer"sv, "Greater Demon Slayer"sv},
            {"Greater Demon Slayer"sv, "Greater Demon Slayer"sv},
            {"Dragon Slayer"sv, "Dragon Slayer"sv},
            {"Dragon Slayer"sv, "Dragon Slayer"sv},

            {"Greater Dragon Slayer"sv, "Greater Dragon Slayer"sv},
            {"Greater Dragon Slayer"sv, "Greater Dragon Slayer"sv},
            {"Underlord"sv, "Underlord"sv},
            {"Underlord"sv, "Underlord"sv},
            {"Overlord"sv, "Overlord"sv},

            {"Baron of Thunder"sv, "Baroness of Thunder"sv},
            {"Baron of Thunder"sv, "Baroness of Thunder"sv},
            {"Baron of Storms"sv, "Baroness of Storms"sv},
            {"Baron of Storms"sv, "Baroness of Storms"sv},
            {"Baron of Lightning"sv, "Baroness of Lightning"sv},

            {"Baron of Tornadoes"sv, "Baroness of Tornadoes"sv},
            {"Baron of Hurricanes"sv, "Baroness of Hurricanes"sv},
            {"Baron of Meteors"sv, "Baroness of Meteors"sv},
            {"Baron of the Earth"sv, "Baroness of the Earth"sv},
            {"Master Warrior"sv, "Master Warrior"sv},

            {"Knight Hero"sv, "Knight Heroine"sv},
            {"Avatar of War"sv, "Avatar of War"sv},
            {"Angel of War"sv, "Angel of War"sv},
            {"Demigod of War"sv, "Demigoddess of War"sv},
            {"Immortal Warlord"sv, "Immortal Warlord"sv},

            {"God of War"sv, "God of War"sv},
            {"Deity of War"sv, "Deity of War"sv},
            {"Supreme Master of War"sv, "Supreme Mistress of War"sv},
            {"Creator"sv, "Creator"sv},
            {"Implementor"sv, "Implementress"sv},
        },
    },
    {
        {{"Man"sv, "Woman"sv},
         {"Axeman"sv, "Axewoman"sv},
         {"Axeman"sv, "Axewoman"sv},
         {"Axeman"sv, "Axewoman"sv},
         {"Rogue"sv, "Rogue"sv},
         {"Rogue"sv, "Rogue"sv},

         {"Rogue"sv, "Rogue"sv},
         {"Nomad"sv, "Nomad "sv},
         {"Nomad"sv, "Nomad "sv},
         {"Nomad"sv, "Nomad "sv},
         {"Strongman"sv, "Strongwoman"sv},

         {"Strongman"sv, "Strongwoman"sv},
         {"Strongman"sv, "Strongwoman"sv},
         {"Bold"sv, "Bold"sv},
         {"Bold"sv, "Bold"sv},
         {"Bold"sv, "Bold"sv},

         {"Renegade"sv, "Renegade"sv},
         {"Renegade"sv, "Renegade"sv},
         {"Renegade"sv, "Renegade"sv},
         {"Cut-throat"sv, "Cut-throat"sv},
         {"Cut-throat"sv, "Cut-throat"sv},

         {"Mighty"sv, "Mighty"sv},
         {"Mighty"sv, "Mighty"sv},
         {"Mighty"sv, "Mighty"sv},
         {"Warrior"sv, "Warrior"sv},
         {"Warrior"sv, "Warrior"sv},

         {"Warrior"sv, "Warrior"sv},
         {"Executioner"sv, "Executioner"sv},
         {"Executioner"sv, "Executioner"sv},
         {"Executioner"sv, "Executioner"sv},
         {"Weapons Crafter"sv, "Weapons Crafter"sv},

         {"Weapons Crafter"sv, "Weapons Crafter"sv},
         {"Weapons Crafter"sv, "Weapons Crafter"sv},
         {"Weapons Master"sv, "Weapons Mistress"sv},
         {"Weapons Master"sv, "Weapons Mistress"sv},
         {"Weapons Master"sv, "Weapons Mistress"sv},

         {"Highwayman"sv, "Highwaywoman"sv},
         {"Highwayman"sv, "Highwaywoman"sv},
         {"Highwayman"sv, "Highwaywoman"sv},
         {"Fearless"sv, "Fearless"sv},
         {"Fearless"sv, "Fearless"sv},

         {"Fearless"sv, "Fearless"sv},
         {"Barbarian"sv, "Barbarian"sv},
         {"Barbarian"sv, "Barbarian"sv},
         {"Barbarian"sv, "Barbarian"sv},
         {"Kinsman"sv, "Kinswoman"sv},

         {"Kinsman"sv, "Kinswoman"sv},
         {"Kinsman"sv, "Kinswoman"sv},
         {"Provider"sv, "Provider"sv},
         {"Provider"sv, "Provider"sv},
         {"Provider"sv, "Provider"sv},

         {"Judicier"sv, "Judicier"sv},
         {"Judicier"sv, "Judicier"sv},
         {"Judicier"sv, "Judicier"sv},
         {"Slayer"sv, "Slayer"sv},
         {"Slayer"sv, "Slayer"sv},

         {"Slayer"sv, "Slayer"sv},
         {"Giant Slayer"sv, "Giant Slayer"sv},
         {"Giant Slayer"sv, "Giant Slayer"sv},
         {"Daemon Slayer"sv, "Daemon Slayer"sv},
         {"Daemon Slayer"sv, "Daemon Slayer"sv},

         {"Wanderer of the Plains"sv, "Wanderer of the Plains"sv},
         {"Wanderer of the Plains"sv, "Wanderer of the Plains"sv},
         {"Wanderer of the Plains"sv, "Wanderer of the Plains"sv},
         {"Wanderer of the Plains"sv, "Wanderer of the Plains"sv},
         {"Wanderer of the Plains"sv, "Wanderer of the Plains"sv},

         {"Baron of the Plains"sv, "Baroness of the Plains"sv},
         {"Baron of the Plains"sv, "Baroness of the Plains"sv},
         {"Baron of the Plains"sv, "Baroness of the Plains"sv},
         {"Lord of the Plains"sv, "Lady of the Plains"sv},
         {"Lord of the Plains"sv, "Lady of the Plains"sv},

         {"Blade Master"sv, "Blade Mistress"sv},
         {"Blade Master"sv, "Blade Mistress"sv},
         {"Infamous"sv, "Infamous"sv},
         {"Infamous"sv, "Infamous"sv},
         {"Infamous"sv, "Infamous"sv},

         {"Master of Vision"sv, "Mistress of Vision"sv},
         {"Master of Vision"sv, "Mistress of Vision"sv},
         {"Master of Lore"sv, "Mistress of Lore"sv},
         {"Master of Lore"sv, "Mistress of Lore"sv},
         {"Master of the Hunt"sv, "Mistress of the Hunt"sv},

         {"Master of the Hunt"sv, "Mistress of the Hunt"sv},
         {"Guardian of an Immortal"sv, "Guardian of an Immortal"sv},
         {"Guardian of an Implementor"sv, "Guardian of an Implementor"sv},
         {"Guardian of an Implementor"sv, "Guardian of an Implementor"sv},
         {"Master of the Wild"sv, "Mistress of the Wild"sv},

         {"King of the Animals"sv, "Queen of the Animals"sv},
         {"Guardian of Justice"sv, "Guardian of Justice"sv},
         {"Protector of the Planet"sv, "Protector of the Planet"sv},
         {"Legend of the Realm"sv, "Legend of the Realm"sv},
         {"Barbarian King"sv, "Barbarian Queen"sv},

         {"Hero of Barbarity"sv, "Heroine of Barbarity"sv},
         {"Avatar of Death"sv, "Avatar of Death"sv},
         {"Angel of Death"sv, "Angel of Death"sv},
         {"Demigod"sv, "Demigoddess"sv},
         {"Immortal"sv, "Immortal"sv},

         {"God of Barbarity"sv, "Goddess of Barbarity"sv},
         {"Deity"sv, "Deity"sv},
         {"Supreme Master"sv, "Supreme Mistress"sv},
         {"Creator"sv, "Creator"sv},
         {"Implementor"sv, "Implementress"sv}},
    },
}};

std::string_view default_title(const Char &ch) {
    const auto class_idx = std::clamp(ch.pcdata->class_type->id, 0_s, static_cast<sh_int>(MAX_CLASS));
    const auto clamped_level = std::clamp(ch.level, 1_s, static_cast<sh_int>(MAX_LEVEL));
    return AllTitles[class_idx][clamped_level].title(ch);
}

}
