# Xania makefile
# Use:
# 	make
# on development systems
#	make RELEASE=true install
# on the final release

CC = gcc
CXX = g++
LINK = g++

ifdef RELEASE
  PROF = -O2
  DEBUG	= 
else
  PROF = 
  DEBUG	=  -g
endif

ifeq '$(HOSTTYPE)' 'arm'
  LIBS =
  C_DEFS =
else
  LIBS = -lcrypt -lnsl
  C_DEFS = -Dlibc6
endif

C_FLAGS = $(PROF) $(C_DEFS) -Wall $(DEBUG)
L_FLAGS = $(PROF) $(DEBUG)

OUT_NEW = xania-new
OUT_CURR = xania
OUT_OLD = xania-old


O_FILES = act_comm.o act_info.o act_move.o act_obj.o act_wiz.o ban.o buffer.o \
	challeng.o clan.o comm.o const.o db.o db2.o \
	fight.o flags.o handler.o healer.o info.o interp.o lookup.o magic.o \
	magic2.o mob_prog.o mob_commands.o news.o save.o skills.o \
	special.o tables.o update.o xania.o phil.o note.o trie.o channels.o \
	wiznet.o Wrapper.o

CHATOS  = chat/akey.o chat/allkeys.o chat/chatmain.o chat/eliza.o

all:	$(OUT_NEW) doorman

$(OUT_NEW): $(O_FILES) $(CHATOS)
	$(LINK) $(L_FLAGS) -o $(OUT_NEW) $(O_FILES) $(CHATOS) $(LIBS)
	@echo Run make install to set up the new binary
	@echo Use "mudmgr" to start the mud
install: all
	if (test -e $(OUT_OLD)) then gzip -f $(OUT_OLD); fi
	if (test -e $(OUT_CURR)) then mv $(OUT_CURR) $(OUT_OLD); fi
	if (test -e $(OUT_NEW)) then mv $(OUT_NEW) $(OUT_CURR); fi

doorman: doorman.o
	$(LINK) $(L_FLAGS) -o doorman doorman.o $(LIBS)

TAGS:   
	etags -dt *.c *.h

backup:
	tar -zcvf ../area.tgz ../area/*.are ../area/*.lst ../area/chat.data 
	tar -zcvf ../src.tgz ../src/*.c ../src/*.h ../src/chat/* ../src/Makefile
	@echo Backup made ok

clean:
	rm -f *.o
	rm -f chat/*.o
	rm -f xania xania-new doorman

chat/akey.o: chat/akey.C
	$(CC) -c $(C_FLAGS) -o chat/akey.o chat/akey.C
chat/allkeys.o: chat/allkeys.C
	$(CC) -c $(C_FLAGS) -o chat/allkeys.o chat/allkeys.C
chat/chatmain.o: chat/chatmain.C
	$(CC) -c $(C_FLAGS) -o chat/chatmain.o chat/chatmain.C
chat/eliza.o: chat/eliza.C
	$(CC) -c $(C_FLAGS) -o chat/eliza.o chat/eliza.C

.c.o:
	$(CC) -c $(C_FLAGS) $<
.cpp.o:
	$(CXX) -c $(C_FLAGS) $<

# Now dependencies are generated here by makedepend(1)
depend:
	makedepend -- $(C_FLAGS) -- *.c chat/*.C


# DO NOT DELETE

act_comm.o: /usr/include/stdio.h /usr/include/features.h /usr/include/libio.h
act_comm.o: /usr/include/_G_config.h /usr/include/wchar.h
act_comm.o: /usr/include/string.h /usr/include/xlocale.h
act_comm.o: /usr/include/stdlib.h /usr/include/alloca.h /usr/include/time.h
act_comm.o: merc.h clan.h chat/chatlink.h
act_info.o: /usr/include/stdio.h /usr/include/features.h /usr/include/libio.h
act_info.o: /usr/include/_G_config.h /usr/include/wchar.h
act_info.o: /usr/include/string.h /usr/include/xlocale.h
act_info.o: /usr/include/stdlib.h /usr/include/alloca.h /usr/include/ctype.h
act_info.o: /usr/include/endian.h /usr/include/time.h merc.h clan.h
act_info.o: chat/chatlink.h buffer.h db.h
act_move.o: /usr/include/stdio.h /usr/include/features.h /usr/include/libio.h
act_move.o: /usr/include/_G_config.h /usr/include/wchar.h
act_move.o: /usr/include/string.h /usr/include/xlocale.h merc.h clan.h
act_move.o: chat/chatlink.h challeng.h
act_obj.o: /usr/include/stdio.h /usr/include/features.h /usr/include/libio.h
act_obj.o: /usr/include/_G_config.h /usr/include/wchar.h
act_obj.o: /usr/include/string.h /usr/include/xlocale.h /usr/include/stdlib.h
act_obj.o: /usr/include/alloca.h merc.h clan.h chat/chatlink.h buffer.h
act_wiz.o: /usr/include/stdio.h /usr/include/features.h /usr/include/libio.h
act_wiz.o: /usr/include/_G_config.h /usr/include/wchar.h
act_wiz.o: /usr/include/string.h /usr/include/xlocale.h /usr/include/stdlib.h
act_wiz.o: /usr/include/alloca.h /usr/include/ctype.h /usr/include/endian.h
act_wiz.o: merc.h clan.h chat/chatlink.h db.h magic.h buffer.h flags.h
ban.o: /usr/include/stdio.h /usr/include/features.h /usr/include/libio.h
ban.o: /usr/include/_G_config.h /usr/include/wchar.h /usr/include/string.h
ban.o: /usr/include/xlocale.h /usr/include/stdlib.h /usr/include/alloca.h
ban.o: merc.h clan.h chat/chatlink.h buffer.h
buffer.o: /usr/include/stdlib.h /usr/include/features.h /usr/include/alloca.h
buffer.o: /usr/include/stdio.h /usr/include/libio.h /usr/include/_G_config.h
buffer.o: /usr/include/wchar.h /usr/include/string.h /usr/include/xlocale.h
buffer.o: merc.h clan.h chat/chatlink.h buffer.h
challeng.o: /usr/include/stdio.h /usr/include/features.h /usr/include/libio.h
challeng.o: /usr/include/_G_config.h /usr/include/wchar.h
challeng.o: /usr/include/string.h /usr/include/xlocale.h /usr/include/time.h
challeng.o: merc.h clan.h chat/chatlink.h interp.h challeng.h
channels.o: /usr/include/stdio.h /usr/include/features.h /usr/include/libio.h
channels.o: /usr/include/_G_config.h /usr/include/wchar.h
channels.o: /usr/include/string.h /usr/include/xlocale.h
channels.o: /usr/include/stdlib.h /usr/include/alloca.h /usr/include/ctype.h
channels.o: /usr/include/endian.h /usr/include/time.h merc.h clan.h
channels.o: chat/chatlink.h
clan.o: /usr/include/stdlib.h /usr/include/features.h /usr/include/alloca.h
clan.o: /usr/include/stdio.h /usr/include/libio.h /usr/include/_G_config.h
clan.o: /usr/include/wchar.h merc.h clan.h chat/chatlink.h
comm.o: /usr/include/ctype.h /usr/include/features.h /usr/include/endian.h
comm.o: /usr/include/xlocale.h /usr/include/errno.h /usr/include/stdio.h
comm.o: /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h
comm.o: /usr/include/string.h /usr/include/stdlib.h /usr/include/alloca.h
comm.o: /usr/include/time.h merc.h clan.h chat/chatlink.h note.h news.h
comm.o: challeng.h doorman.h
const.o: /usr/include/stdio.h /usr/include/features.h /usr/include/libio.h
const.o: /usr/include/_G_config.h /usr/include/wchar.h /usr/include/time.h
const.o: /usr/include/xlocale.h merc.h clan.h chat/chatlink.h magic.h
db.o: /usr/include/stdio.h /usr/include/features.h /usr/include/libio.h
db.o: /usr/include/_G_config.h /usr/include/wchar.h /usr/include/string.h
db.o: /usr/include/xlocale.h /usr/include/stdlib.h /usr/include/alloca.h
db.o: /usr/include/ctype.h /usr/include/endian.h /usr/include/time.h merc.h
db.o: clan.h chat/chatlink.h db.h buffer.h note.h interp.h
db2.o: /usr/include/stdio.h /usr/include/features.h /usr/include/libio.h
db2.o: /usr/include/_G_config.h /usr/include/wchar.h /usr/include/string.h
db2.o: /usr/include/xlocale.h /usr/include/stdlib.h /usr/include/alloca.h
db2.o: /usr/include/ctype.h /usr/include/endian.h /usr/include/time.h merc.h
db2.o: clan.h chat/chatlink.h db.h lookup.h
dbextras.o: /usr/include/stdio.h /usr/include/features.h /usr/include/libio.h
dbextras.o: /usr/include/_G_config.h /usr/include/wchar.h
dbextras.o: /usr/include/string.h /usr/include/xlocale.h
dbextras.o: /usr/include/stdlib.h /usr/include/alloca.h /usr/include/ctype.h
dbextras.o: /usr/include/endian.h /usr/include/time.h merc.h clan.h
dbextras.o: chat/chatlink.h db.h
doorman.o: /usr/include/stdio.h /usr/include/features.h /usr/include/libio.h
doorman.o: /usr/include/_G_config.h /usr/include/wchar.h
doorman.o: /usr/include/string.h /usr/include/xlocale.h /usr/include/stdlib.h
doorman.o: /usr/include/alloca.h /usr/include/errno.h /usr/include/fcntl.h
doorman.o: /usr/include/time.h /usr/include/netdb.h /usr/include/netinet/in.h
doorman.o: /usr/include/stdint.h /usr/include/endian.h
doorman.o: /usr/include/rpc/netdb.h /usr/include/unistd.h
doorman.o: /usr/include/getopt.h /usr/include/ctype.h
doorman.o: /usr/include/arpa/inet.h /usr/include/arpa/telnet.h
doorman.o: /usr/include/signal.h merc.h clan.h chat/chatlink.h doorman.h
fight.o: /usr/include/stdio.h /usr/include/features.h /usr/include/libio.h
fight.o: /usr/include/_G_config.h /usr/include/wchar.h /usr/include/string.h
fight.o: /usr/include/xlocale.h /usr/include/time.h /usr/include/ctype.h
fight.o: /usr/include/endian.h merc.h clan.h chat/chatlink.h challeng.h
flags.o: /usr/include/ctype.h /usr/include/features.h /usr/include/endian.h
flags.o: /usr/include/xlocale.h /usr/include/errno.h /usr/include/stdio.h
flags.o: /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h
flags.o: /usr/include/string.h /usr/include/stdlib.h /usr/include/alloca.h
flags.o: /usr/include/time.h merc.h clan.h chat/chatlink.h flags.h buffer.h
handler.o: /usr/include/ctype.h /usr/include/features.h /usr/include/endian.h
handler.o: /usr/include/xlocale.h /usr/include/stdio.h /usr/include/libio.h
handler.o: /usr/include/_G_config.h /usr/include/wchar.h
handler.o: /usr/include/string.h /usr/include/time.h merc.h clan.h
handler.o: chat/chatlink.h buffer.h
healer.o: /usr/include/stdio.h /usr/include/features.h /usr/include/libio.h
healer.o: /usr/include/_G_config.h /usr/include/wchar.h /usr/include/string.h
healer.o: /usr/include/xlocale.h /usr/include/stdlib.h /usr/include/alloca.h
healer.o: merc.h clan.h chat/chatlink.h magic.h
info.o: /usr/include/stdio.h /usr/include/features.h /usr/include/libio.h
info.o: /usr/include/_G_config.h /usr/include/wchar.h /usr/include/string.h
info.o: /usr/include/xlocale.h /usr/include/stdlib.h /usr/include/alloca.h
info.o: /usr/include/ctype.h /usr/include/endian.h /usr/include/time.h
info.o: /usr/include/dirent.h merc.h clan.h chat/chatlink.h buffer.h
interp.o: /usr/include/ctype.h /usr/include/features.h /usr/include/endian.h
interp.o: /usr/include/xlocale.h /usr/include/stdio.h /usr/include/libio.h
interp.o: /usr/include/_G_config.h /usr/include/wchar.h /usr/include/stdlib.h
interp.o: /usr/include/alloca.h /usr/include/string.h /usr/include/time.h
interp.o: merc.h clan.h chat/chatlink.h interp.h note.h trie.h
lookup.o: /usr/include/stdio.h /usr/include/features.h /usr/include/libio.h
lookup.o: /usr/include/_G_config.h /usr/include/wchar.h /usr/include/time.h
lookup.o: /usr/include/xlocale.h /usr/include/string.h /usr/include/stdlib.h
lookup.o: /usr/include/alloca.h /usr/include/ctype.h /usr/include/endian.h
lookup.o: merc.h clan.h chat/chatlink.h tables.h
magic.o: /usr/include/stdio.h /usr/include/features.h /usr/include/libio.h
magic.o: /usr/include/_G_config.h /usr/include/wchar.h /usr/include/stdlib.h
magic.o: /usr/include/alloca.h /usr/include/string.h /usr/include/xlocale.h
magic.o: /usr/include/time.h merc.h clan.h chat/chatlink.h magic.h challeng.h
magic2.o: /usr/include/stdio.h /usr/include/features.h /usr/include/libio.h
magic2.o: /usr/include/_G_config.h /usr/include/wchar.h /usr/include/stdlib.h
magic2.o: /usr/include/alloca.h /usr/include/string.h /usr/include/xlocale.h
magic2.o: /usr/include/time.h merc.h clan.h chat/chatlink.h magic.h
magic2.o: challeng.h
mob_commands.o: /usr/include/stdio.h /usr/include/features.h
mob_commands.o: /usr/include/libio.h /usr/include/_G_config.h
mob_commands.o: /usr/include/wchar.h /usr/include/string.h
mob_commands.o: /usr/include/xlocale.h /usr/include/stdlib.h
mob_commands.o: /usr/include/alloca.h merc.h clan.h chat/chatlink.h
mob_prog.o: /usr/include/stdio.h /usr/include/features.h /usr/include/libio.h
mob_prog.o: /usr/include/_G_config.h /usr/include/wchar.h
mob_prog.o: /usr/include/string.h /usr/include/xlocale.h
mob_prog.o: /usr/include/stdlib.h /usr/include/alloca.h /usr/include/ctype.h
mob_prog.o: /usr/include/endian.h merc.h clan.h chat/chatlink.h
news.o: /usr/include/stdio.h /usr/include/features.h /usr/include/libio.h
news.o: /usr/include/_G_config.h /usr/include/wchar.h /usr/include/stdlib.h
news.o: /usr/include/alloca.h /usr/include/string.h /usr/include/xlocale.h
news.o: merc.h clan.h chat/chatlink.h buffer.h news.h flags.h
note.o: /usr/include/stdio.h /usr/include/features.h /usr/include/libio.h
note.o: /usr/include/_G_config.h /usr/include/wchar.h /usr/include/string.h
note.o: /usr/include/xlocale.h /usr/include/stdlib.h /usr/include/alloca.h
note.o: /usr/include/ctype.h /usr/include/endian.h /usr/include/time.h merc.h
note.o: clan.h chat/chatlink.h note.h trie.h buffer.h
olc_ctrl.o: /usr/include/stdio.h /usr/include/features.h /usr/include/libio.h
olc_ctrl.o: /usr/include/_G_config.h /usr/include/wchar.h
olc_ctrl.o: /usr/include/string.h /usr/include/xlocale.h
olc_ctrl.o: /usr/include/stdlib.h /usr/include/alloca.h /usr/include/ctype.h
olc_ctrl.o: /usr/include/endian.h merc.h clan.h chat/chatlink.h db.h buffer.h
olc_ctrl.o: olc_ctrl.h
olc_obj.o: /usr/include/stdio.h /usr/include/features.h /usr/include/libio.h
olc_obj.o: /usr/include/_G_config.h /usr/include/wchar.h
olc_obj.o: /usr/include/string.h /usr/include/xlocale.h /usr/include/stdlib.h
olc_obj.o: /usr/include/alloca.h merc.h clan.h chat/chatlink.h db.h flags.h
olc_obj.o: buffer.h olc_ctrl.h olc_obj.h
olc_room.o: /usr/include/stdio.h /usr/include/features.h /usr/include/libio.h
olc_room.o: /usr/include/_G_config.h /usr/include/wchar.h
olc_room.o: /usr/include/string.h /usr/include/xlocale.h
olc_room.o: /usr/include/stdlib.h /usr/include/alloca.h merc.h clan.h
olc_room.o: chat/chatlink.h db.h flags.h buffer.h olc_ctrl.h olc_room.h
phil.o: /usr/include/stdio.h /usr/include/features.h /usr/include/libio.h
phil.o: /usr/include/_G_config.h /usr/include/wchar.h /usr/include/stdlib.h
phil.o: /usr/include/alloca.h /usr/include/string.h /usr/include/xlocale.h
phil.o: /usr/include/time.h merc.h clan.h chat/chatlink.h magic.h interp.h
prog.o: /usr/include/ctype.h /usr/include/features.h /usr/include/endian.h
prog.o: /usr/include/xlocale.h /usr/include/stdio.h /usr/include/libio.h
prog.o: /usr/include/_G_config.h /usr/include/wchar.h /usr/include/stdlib.h
prog.o: /usr/include/alloca.h /usr/include/string.h /usr/include/time.h
prog.o: merc.h clan.h chat/chatlink.h magic.h interp.h prog.h
save.o: /usr/include/ctype.h /usr/include/features.h /usr/include/endian.h
save.o: /usr/include/xlocale.h /usr/include/stdio.h /usr/include/libio.h
save.o: /usr/include/_G_config.h /usr/include/wchar.h /usr/include/string.h
save.o: /usr/include/time.h merc.h clan.h chat/chatlink.h news.h
skills.o: /usr/include/stdio.h /usr/include/features.h /usr/include/libio.h
skills.o: /usr/include/_G_config.h /usr/include/wchar.h /usr/include/string.h
skills.o: /usr/include/xlocale.h /usr/include/stdlib.h /usr/include/alloca.h
skills.o: merc.h clan.h chat/chatlink.h magic.h
special.o: /usr/include/stdio.h /usr/include/features.h /usr/include/libio.h
special.o: /usr/include/_G_config.h /usr/include/wchar.h
special.o: /usr/include/stdlib.h /usr/include/alloca.h /usr/include/string.h
special.o: /usr/include/xlocale.h /usr/include/time.h merc.h clan.h
special.o: chat/chatlink.h magic.h phil.h
tables.o: /usr/include/stdio.h /usr/include/features.h /usr/include/libio.h
tables.o: /usr/include/_G_config.h /usr/include/wchar.h /usr/include/string.h
tables.o: /usr/include/xlocale.h /usr/include/time.h merc.h clan.h
tables.o: chat/chatlink.h tables.h
trie.o: /usr/include/stdlib.h /usr/include/features.h /usr/include/alloca.h
trie.o: /usr/include/stdio.h /usr/include/libio.h /usr/include/_G_config.h
trie.o: /usr/include/wchar.h /usr/include/string.h /usr/include/xlocale.h
trie.o: /usr/include/ctype.h /usr/include/endian.h trie.h
trietest.o: /usr/include/stdio.h /usr/include/features.h /usr/include/libio.h
trietest.o: /usr/include/_G_config.h /usr/include/wchar.h trie.h
update.o: /usr/include/stdio.h /usr/include/features.h /usr/include/libio.h
update.o: /usr/include/_G_config.h /usr/include/wchar.h /usr/include/string.h
update.o: /usr/include/xlocale.h /usr/include/time.h merc.h clan.h
update.o: chat/chatlink.h prog.h
wiznet.o: /usr/include/ctype.h /usr/include/features.h /usr/include/endian.h
wiznet.o: /usr/include/xlocale.h /usr/include/stdio.h /usr/include/libio.h
wiznet.o: /usr/include/_G_config.h /usr/include/wchar.h /usr/include/stdlib.h
wiznet.o: /usr/include/alloca.h /usr/include/string.h /usr/include/time.h
wiznet.o: merc.h clan.h chat/chatlink.h trie.h
xania.o: /usr/include/ctype.h /usr/include/features.h /usr/include/endian.h
xania.o: /usr/include/xlocale.h /usr/include/stdio.h /usr/include/libio.h
xania.o: /usr/include/_G_config.h /usr/include/wchar.h /usr/include/stdlib.h
xania.o: /usr/include/alloca.h /usr/include/string.h /usr/include/time.h
xania.o: merc.h clan.h chat/chatlink.h db.h interp.h magic.h buffer.h
chat/akey.o: /usr/include/stdlib.h /usr/include/features.h
chat/akey.o: /usr/include/alloca.h /usr/include/stdio.h /usr/include/libio.h
chat/akey.o: /usr/include/_G_config.h /usr/include/wchar.h
chat/akey.o: /usr/include/string.h /usr/include/xlocale.h chat/akey.hpp
chat/akey.o: chat/chatmain.hpp
chat/allkeys.o: chat/allkeys.hpp chat/chatmain.hpp chat/akey.hpp
chat/allkeys.o: /usr/include/stdlib.h /usr/include/features.h
chat/allkeys.o: /usr/include/alloca.h /usr/include/stdio.h
chat/allkeys.o: /usr/include/libio.h /usr/include/_G_config.h
chat/allkeys.o: /usr/include/wchar.h
chat/chatmain.o: /usr/include/stdlib.h /usr/include/features.h
chat/chatmain.o: /usr/include/alloca.h /usr/include/string.h
chat/chatmain.o: /usr/include/xlocale.h /usr/include/stdio.h
chat/chatmain.o: /usr/include/libio.h /usr/include/_G_config.h
chat/chatmain.o: /usr/include/wchar.h /usr/include/time.h chat/akey.hpp
chat/chatmain.o: chat/chatmain.hpp chat/allkeys.hpp chat/eliza.hpp
chat/chatrun.o: /usr/include/stdio.h /usr/include/features.h
chat/chatrun.o: /usr/include/libio.h /usr/include/_G_config.h
chat/chatrun.o: /usr/include/wchar.h chat/chatmain.hpp chat/eliza.hpp
chat/chatrun.o: /usr/include/string.h /usr/include/xlocale.h chat/allkeys.hpp
chat/chatrun.o: chat/akey.hpp /usr/include/stdlib.h /usr/include/alloca.h
chat/eliza.o: /usr/include/stdlib.h /usr/include/features.h
chat/eliza.o: /usr/include/alloca.h /usr/include/stdio.h /usr/include/libio.h
chat/eliza.o: /usr/include/_G_config.h /usr/include/wchar.h
chat/eliza.o: /usr/include/string.h /usr/include/xlocale.h
chat/eliza.o: /usr/include/ctype.h /usr/include/endian.h
chat/eliza.o: /usr/include/search.h chat/eliza.hpp chat/chatmain.hpp
chat/eliza.o: chat/allkeys.hpp chat/akey.hpp /usr/include/unistd.h
chat/eliza.o: /usr/include/getopt.h
