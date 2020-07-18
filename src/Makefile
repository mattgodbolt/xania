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
	etags -D *.[ch] chat/*.hpp chat/*.C doorman/*.[ch]

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
