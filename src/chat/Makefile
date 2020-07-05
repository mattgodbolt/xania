#makefile for chat by Chris Busch

#compiler options:
CC=g++
FLAGS= -ansi  
#LIB= -ldl
LIB =
OUTPUT=chat
OBJS=chatmain.o akey.o allkeys.o eliza.o chatrun.o

$(OUTPUT): $(OBJS)
	$(CC) $(FLAGS) $(OBJS) -o $(OUTPUT) $(LIB)


chatmain.o: chatmain.hpp chatmain.C eliza.hpp
	$(CC) $(FLAGS) -c chatmain.C

#dont link this object into your mud!
chatrun.o: chatmain.hpp chatrun.C
	$(CC) $(FLAGS) -c chatrun.C

akey.o: chatmain.hpp akey.hpp akey.C
	$(CC) $(FLAGS) -c akey.C

allkeys.o:chatmain.hpp allkeys.hpp allkeys.C
	$(CC) $(FLAGS) -c allkeys.C

eliza.o: chatmain.hpp eliza.C eliza.hpp
	$(CC) $(FLAGS) -c eliza.C





