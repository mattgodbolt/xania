#pragma once

#include "Descriptor.hpp"
#include "doorman/doorman_protocol.h"

struct CHAR_DATA;

void close_socket(Descriptor *dclose);
void send_to_char(std::string_view txt, CHAR_DATA *ch);
void page_to_char(const char *txt, CHAR_DATA *ch);
void act(const char *format, CHAR_DATA *ch, const void *arg1, const void *arg2, int type);
void act(const char *format, CHAR_DATA *ch, const void *arg1, const void *arg2, int type, int min_pos);

bool SendPacket(Packet *p, const void *extra);

extern Descriptor *descriptor_list;
