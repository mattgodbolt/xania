#include "DescriptorData.hpp"

// Up to 5 characters
const char *short_name_of(DescriptorState state) {
    switch (state) {
    case DescriptorState::Playing: return "Play";
    case DescriptorState::GetName: return "Name";
    case DescriptorState::GetOldPassword: return "OldPw";
    case DescriptorState::ConfirmNewName: return "CnfNm";
    case DescriptorState::GetNewPassword: return "NewPw";
    case DescriptorState::ConfirmNewPassword: return "CnfNp";
    case DescriptorState::GetNewRace: return "GetNR";
    case DescriptorState::GetNewSex: return "GetNS";
    case DescriptorState::GetNewClass: return "GetNA";
    case DescriptorState::GetAlignment: return "GetAl";
    case DescriptorState::DefaultChoice: return "DefCh";
    case DescriptorState::GenGroups: return "GenGr";
    case DescriptorState::ReadIMotd: return "Imotd";
    case DescriptorState::ReadMotd: return "Motd";
    case DescriptorState::BreakConnect: return "BrkCn";
    case DescriptorState::GetAnsi: return "Ansi";
    case DescriptorState::CircumventPassword: return "CPass";
    case DescriptorState::Disconnecting: return "Disc";
    case DescriptorState::DisconnectingNp: return "DiscN";
    }
    return "<UNK>";
}

const char *name_of(DescriptorState state) {
    switch (state) {
    case DescriptorState::Playing: return "Playing";
    case DescriptorState::GetName: return "GetName";
    case DescriptorState::GetOldPassword: return "GetOldPassword";
    case DescriptorState::ConfirmNewName: return "ConfirmNewName";
    case DescriptorState::GetNewPassword: return "GetNewPassword";
    case DescriptorState::ConfirmNewPassword: return "ConfirmNewPassword";
    case DescriptorState::GetNewRace: return "GetNewRace";
    case DescriptorState::GetNewSex: return "GetNewSex";
    case DescriptorState::GetNewClass: return "GetNewClass";
    case DescriptorState::GetAlignment: return "GetAlignment";
    case DescriptorState::DefaultChoice: return "DefaultChoice";
    case DescriptorState::GenGroups: return "GenGroups";
    case DescriptorState::ReadIMotd: return "ReadIMotd";
    case DescriptorState::ReadMotd: return "ReadMotd";
    case DescriptorState::BreakConnect: return "BreakConnect";
    case DescriptorState::GetAnsi: return "GetAnsi";
    case DescriptorState::CircumventPassword: return "CircumventPassword";
    case DescriptorState::Disconnecting: return "Disconnecting";
    case DescriptorState::DisconnectingNp: return "DisconnectingNp";
    }
    return "Unknown";
}
