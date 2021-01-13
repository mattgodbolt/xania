#!/usr/bin/perl -i
# A script that will look for rooms within the ROOMS section of area files
# and strip the leading number from the line right after the description.
# This field was never used. We can probably adapt this script to do other
# tactical area file massagery in future.

my $in_rooms = 0;
my $in_room = 0;
my $room_tildes = 0;

while (<>) {
    if (/#ROOMS/) {
        $in_rooms = 1;
    } elsif ((/#[A-Z]{2,}/)) {  # OBJECTS, MOBILES, RESETS, HELPS etc.
        $in_rooms = 0;
    }
    if ($in_rooms) {
        if (/#[\d]+/) { # room vnum
            $in_room = 1;
            $room_tildes = 0;
        } elsif ($in_room && /\~/) {
            $room_tildes++;
        } elsif ($room_tildes == 2 && /^([\d]+)\s+(\S+)\s+(\S+)\s*$/) {
            print "$2 $3\n";
            $in_room = 0;
            next;
        }
    }
    print;

}