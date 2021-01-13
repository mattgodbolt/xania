#!/usr/bin/perl -i
# A script that will look for mobs within the MOBILS section of area files
# and improve each of the armour class ratings by 15, so that it doesn't
# need to be done at runtime in MobIndexData.

my $in_mobiles = 0;
my $vnum = 0;
my $found_damage_line = 0;

while (<>) {
    if (/#MOBILES/) {
        $in_mobiles = 1;
    } elsif ((/#[A-Z]{2,}/)) {  # ROOMS, OBJECTS, RESETS, HELPS etc.
        $in_mobiles = 0;
        $vnum = 0;
        $found_damage_line = 0;
    } elsif ($in_mobiles) {
        if (/^\s*#([\d]+)/) { # vnum
            if ($vnum) {
                # If the vnum hasn't been reset, one of the expected states was not met.
                if (!$found_damage_line) {
                    print STDERR "vnum $vnum, found no damage line, failed parse?\n";
                } else {
                    print STDERR "vnum $vnum, found damage line but didn't find armour line, failed parse?\n";
                }
                exit 1;
            }
            $vnum = $1;
        } elsif ($vnum != 0 && /^\d+\s+\-?\+?\d+\s+\d+d\d+\+\d+\s+\d+d\d+\+\d+/) {
            # The hit/mana line is distinctive enough line for us to know the next is the line we need.
            #                   level  damroll    hit            mana (rest ignored)
            $found_damage_line = 1;
        } elsif ($found_damage_line && /(-?\d+)\s+(-?\d+)\s+(-?\d+)\s+(-?\d+)\s*/) {
            my $pierce = $1 - 15;
            my $bash = $2 - 15;
            my $slash = $3 - 15;
            my $exotic = $4 - 15;
            print "$pierce $bash $slash $exotic\n";
            $vnum = 0;
            $found_damage_line = 0;
            next;
        }
    }
    print;

}