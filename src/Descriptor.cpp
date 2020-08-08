#include "Descriptor.hpp"

#include "comm.hpp"
#include "string_utils.hpp"

#include <fmt/format.h>
using namespace fmt::literals;

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

Descriptor::Descriptor(uint32_t descriptor) : login_time_(ctime(&current_time)), descriptor(descriptor) {}

Descriptor::~Descriptor() { /*    free_string(dclose->showstr_head); */ }

std::optional<std::string> Descriptor::pop_raw() {
    if (pending_commands_.empty())
        return std::nullopt;
    auto result = std::move(pending_commands_.front());
    pending_commands_.pop_front();
    return result;
}

std::optional<std::string> Descriptor::pop_incomm() {
    auto maybe_next = pop_raw();
    if (!maybe_next)
        return {};

    // Handle any backspace characters, newlines, or non-printing characters.
    auto next_line = sanitise_input(*maybe_next);

    if (next_line.size() > MAX_INPUT_LENGTH - 1) {
        write_direct("Line too long.\n\r");
        return {};
    }

    // Do '!' substitution.
    if (next_line[0] == '!')
        return last_command_;
    else {
        last_command_ = next_line;
        return next_line;
    }
}

bool Descriptor::write_direct(std::string_view text) const {
    Packet p;
    p.type = PACKET_MESSAGE;
    p.channel = descriptor;

    while (!text.empty()) {
        p.nExtra = std::min<uint32_t>(text.length(), PACKET_MAX_PAYLOAD_SIZE);
        if (!SendPacket(&p, text.data()))
            return false;
        text = text.substr(p.nExtra);
    }
    return true;
}

namespace {

// TODO duplicated with doorman! we should share implementation
unsigned long djb2_hash(std::string_view str) {
    unsigned long hash = 5381;
    for (auto c : str)
        hash = hash * 33 + c;
    return hash;
}

// Returns the hostname, masked for privacy and with a hashcode of the full hostname. This can be used by admins to spot
// users coming from the same IP.
std::string get_masked_hostname(std::string_view hostname) {
    return "{}*** [#{}]"_format(hostname.substr(0, 6), djb2_hash(hostname));
}

}

void Descriptor::raw_full_hostname(std::string_view raw_full_hostname) {
    raw_host_ = raw_full_hostname;
    masked_host_ = get_masked_hostname(raw_host_);
}

bool Descriptor::flush_output() noexcept {
    if (outbuf_.empty())
        return true;

    if (snoop_by && character) {
        snoop_by->write(character->name);
        snoop_by->write("> ");
        snoop_by->write(outbuf_);
    }

    auto result = write_direct(outbuf_);
    outbuf_.clear();
    return result;
}

void Descriptor::write(std::string_view message) noexcept {
    // Initial \n\r if needed.
    if (outbuf_.empty() && !fcommand)
        outbuf_ += "\n\r";

    if (outbuf_.size() + message.size() > MaxOutputBufSize) {
        bug("Buffer overflow. Closing.");
        outbuf_.clear(); // Prevent a possible loop where close_socket() might write some last few things to the socket.
        close_socket(this); // NB a bit hairy, amounts to a `delete this;` The only thing safe to do is return.
        return;
    }

    outbuf_ += message;
}
