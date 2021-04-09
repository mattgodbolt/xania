#!/usr/bin/env python3
import fileinput
import sys
import os
import re
import enum

# A script that rewrites the Equip/Give resets of every area and replaces the "arg2" slot 
# with a percentage based "drop rate". The mud will test this rate whenever it is resetting the mob.
# Some mobs always use a fixed rate of 100%:
#    < Level 20
#    shop keepers
#    trainers/guildmasters
# Some objects always use a fixed rate too so that we don't screw with certain game mechanics e.g. keys always drop.

# These get populated in the first pass.
# Then in the second pass only the #RESETS section is examined, and every Equip/Give reset ends up being
# temporarily buffered within each Mobile, before those resets are flushed and rewritten.
mobs = {}
objects = {}
shops = []

class Parser:
    resets_section_line = re.compile("^#RESETS")
    mobiles_section_line = re.compile("^#MOBILES")
    objects_section_line = re.compile("^#OBJECTS")
    shops_section_line = re.compile("^#SHOPS")
    other_section_line = re.compile("^#[A-Z]{2,}")
    vnum_line = re.compile("^#(\d{1,}\s*$)")
    object_type_line = re.compile("^([a-zA-Z_]+)\s+[a-zA-Z0-9]+\s[a-zA-Z0-9]+\s*$")
    shop_line = re.compile("^\s*(\d+)\s+\d+\s+\d+\s+\d+\s+\d+\s+\d+\s+\d+\s+\d+\s+\d+\s+\d+")
    shops_section_end_line = re.compile("^\s*0")
    equip_give_line = re.compile("^\s*([EG]\s+\d+\s+)(\d+)\s+-?\d+(.*$)")
    mob_reset_line = re.compile("^\s*M\s+\d+\s+(\d+)\s+.*")
    other_reset_line = re.compile("^\s*[OPDRS].*")
    mob_act_line = re.compile("\s*([a-zA-Z0-9]+)\s+[a-zA-Z0-9]+\s+\-?\d+\s+\d+\s*$")
    mob_level_line = re.compile("^(\d+)\s+\d+\s+\d+d\d+\+.*")

    class Section(enum.Enum):
        Mobiles = 0
        Objects = 1
        Resets  = 2
        Shops   = 3
        Other   = 4

    def __init__(self):
        self.section = Parser.Section.Other
        self.tilde_depth = 0
        self.current_vnum = None
    def start_section(self, section):
        self.section = section
        self.tilde_depth = 0
        self.current_vnum = None

class EquipGiveReset:
    def __init__(self, before_arg2, object_vnum, after_arg2):
        self.before_arg2 = before_arg2
        self.object_vnum = object_vnum
        self.after_arg2 = after_arg2
        self.drop_rate = 100
    def __repr__(self):
        return "<%s %s before_arg2:%s object_vnum:%s after_arg2:%s>" % (
            self.__class__.__name__, id(self), self.before_arg2, self.object_vnum, self.after_arg2)

class Mobile:
    healer_trainer_act_bits = re.compile("[abJK]")
    def __init__(self, vnum):
        self.vnum = vnum
        self.level = 0
        self.act_flags = None
        self.equip_give_resets = []
    
    # Can this mob have resets with dynamic drop rates?
    def has_dyn_drop_rates(self):
        return not (self.level < 20 
            or self.vnum in shops
            or self.healer_trainer_act_bits.search(self.act_flags) is not None)
    
    # Print out the adjusted Equip/Give resets cached for this mob and clear the cache
    # because the RESETS section enables same mob vnum can be instantiated more than once,
    # with different reset entries.
    def output_resets(self):
        if not self.equip_give_resets:
            return
        self.fill_reset_drop_rates()
        for reset in self.equip_give_resets: 
            print("{}{} {:>3}{}".format(reset.before_arg2, reset.object_vnum, reset.drop_rate, reset.after_arg2, end = ""))
        self.equip_give_resets = []

    # If the mob can't have dynamic drop rates all of its EquipGiveResets use the default of 100.
    def fill_reset_drop_rates(self):
        if self.has_dyn_drop_rates():
            eligible_obj_count = self.count_objs_with_dyn_drop_rate()
            if eligible_obj_count > 0:
                mob_dyn_drop_rate = self.get_mob_dyn_drop_rate(eligible_obj_count)
                for reset in self.equip_give_resets:
                    reset.drop_rate = self.select_reset_drop_rate(reset, mob_dyn_drop_rate)

    # Calculate the dynamic drop rate the mob should used based on how many objects eligible 
    # for dynamic drop rates it is potentially going to be given, and its level.
    # Higher level mobs will on average have slightly lower drop rates.
    def get_mob_dyn_drop_rate(self, eligible_obj_count):
        base =  (90 if self.level <= 50
                else 80 if self.level <= 75
                else 70) 
        return int(min(base, (base / eligible_obj_count) + (
            18 if self.level <= 50 else
            14 if self.level <= 75 else 10)))

    # Choose the drop rate to use for a specific Equip/Give reset. Some objects are ineligible like keys.
    def select_reset_drop_rate(self, reset, mob_dyn_drop_rate):
        object = objects[reset.object_vnum]
        return mob_dyn_drop_rate if object.has_dyn_drop_rate() else 100

    # How many objects is the mob due to be Equipped/Given that are eligible for dynamic drop rates?
    def count_objs_with_dyn_drop_rate(self):
        return len(
            [object for object in 
                [objects[reset.object_vnum] for reset in self.equip_give_resets] 
                if object.has_dyn_drop_rate()])


class Object:
    object_types_fixed_drop_rate = re.compile("key|money|boat|fountain|pc_corpse|npc_corpse|container", re.IGNORECASE)

    def __init__(self):
        self.object_type = None

    # Can this object have a dynamic drop rate rather than 100%?
    def has_dyn_drop_rate(self):
        return not Object.object_types_fixed_drop_rate.search(self.object_type)

    def __repr__(self):
        return "<%s %s %s>" % (self.__class__.__name__, id(self), self.object_type)

# Make two passes over the area files. The first is to load up a minimal amount of mobile
# and object data so that when analyzing the resets in the second pass. The second pass copies
# out each area line by line, rewriting the Equip/Give resets. It's done in two passes because
# resets can reference mobiles and objects by vnum that are defined in other files.
def rewrite_areas():
    files = [f for f in os.listdir(".") if os.path.isfile(f) 
        and f.endswith(".are")
        and f != "version.are"] # this isn't a real area
    for filename in files:
        load_database(filename)
    for filename in files:
        rewrite_area(filename)

def load_database(filename):
    with fileinput.FileInput(filename) as f:
        parser = Parser()
        for line in f:
            if Parser.mobiles_section_line.search(line):
                parser.start_section(Parser.Section.Mobiles)
            elif Parser.objects_section_line.search(line):
                parser.start_section(Parser.Section.Objects)
            elif Parser.shops_section_line.search(line):
                parser.start_section(Parser.Section.Shops)
            elif Parser.other_section_line.search(line):
                parser.start_section(Parser.Section.Other)

            if parser.section == Parser.Section.Mobiles:
                if matches := Parser.vnum_line.match(line):
                    parser.current_vnum = int(matches.group(1))
                    parser.tilde_depth = 0
                    mobs[parser.current_vnum] = Mobile(parser.current_vnum)
                elif parser.current_vnum is not None:
                    if '~' in line:
                        parser.tilde_depth += 1
                    elif parser.tilde_depth == 5:
                        matches = Parser.mob_act_line.fullmatch(line)
                        assert matches is not None,  "bad mob act line #{}, {}".format(parser.current_vnum, line) 
                        mobs[parser.current_vnum].act_flags = matches.group(1)
                        parser.tilde_depth = 0
                    elif matches := Parser.mob_level_line.match(line):
                        mobs[parser.current_vnum].level = int(matches.group(1))

            elif parser.section == Parser.Section.Objects:
                if matches := Parser.vnum_line.match(line):
                    parser.current_vnum = int(matches.group(1))
                    objects[parser.current_vnum] = Object()
                elif matches := Parser.object_type_line.match(line):
                    objects[parser.current_vnum].object_type = matches.group(1)

            elif parser.section == Parser.Section.Shops:
                if Parser.shops_section_end_line.search(line):
                    parser.start_section(Parser.Section.Other)
                    shops.sort()
                elif matches := Parser.shop_line.match(line):
                    shops.append(int(matches.group(1)))


def rewrite_area(filename):
    with fileinput.FileInput(filename, inplace = True, backup = ".bak") as f:
        parser = Parser()
        for line in f:
            if Parser.resets_section_line.search(line):
                parser.start_section(Parser.Section.Resets)
            elif Parser.other_section_line.search(line):
                parser.start_section(Parser.Section.Other)

            if parser.section == Parser.Section.Resets:
                # A new mobile reset, unbuffer the last mob's resets if we have one
                # Important: the same mobile vnum can appear more than once in the resets section
                # but its E/G resets can be different for each one.
                if matches := Parser.mob_reset_line.match(line):
                    if parser.current_vnum is not None:
                        mobs[parser.current_vnum].output_resets()
                    parser.current_vnum = int(matches.group(1))
                # An (E)quip or (G)ive line, a mobile reset context must exist
                elif matches := Parser.equip_give_line.match(line):
                    assert parser.current_vnum is not None, "Expected current_vnum before E/G reset: " + line
                    equip_give_reset = EquipGiveReset(matches.group(1), int(matches.group(2)), matches.group(3))
                    mobs[parser.current_vnum].equip_give_resets.append(equip_give_reset)
                    continue
                # Any other kind of reset line including the final 'S' that ends the RESETS section,
                # unbuffer the last mob's resets if we have one
                elif matches := Parser.other_reset_line.match(line):
                    if parser.current_vnum is not None:
                        mobs[parser.current_vnum].output_resets()
                        parser.current_vnum = None

            print(line, end = "")
    os.unlink(filename + ".bak")

if __name__ == "__main__":
    rewrite_areas()
    
