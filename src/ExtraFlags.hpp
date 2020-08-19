#pragma once

/*
 * Extra flags
 */
#define MAX_EXTRA_FLAGS 64
#define EXTRA_WIZNET_ON 0
#define EXTRA_WIZNET_DEBUG 1
#define EXTRA_WIZNET_MORT 2
#define EXTRA_WIZNET_IMM 3
#define EXTRA_WIZNET_BUG 4
#define EXTRA_PERMIT 5
#define EXTRA_WIZNET_TICK 6

// Some bits no longer used here. Be aware there *could* be legacy player files where
// these bits are enabled so consider that if you decide to repurpose them. And
// maybe write some migration code to force it off on login if the new feature
// is sensitive to the bit.
#define EXTRA_UNUSED_1 9
#define EXTRA_UNUSED_2 10

#define EXTRA_INFO_MESSAGE 11

#define EXTRA_UNUSED_3 12

#define EXTRA_TIP_WIZARD 14
#define EXTRA_TIP_UNUSED_4 15
#define EXTRA_TIP_ADVANCED 16

extern const char *flagname_extra[];
