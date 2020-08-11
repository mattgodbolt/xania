#pragma once

#include "Descriptor.hpp"
#include "doorman/doorman_protocol.h"

bool SendPacket(Packet *p, const void *extra);

extern Descriptor *descriptor_list;
