
//chatmain.C
//Chris Busch (c) 1993
//please see chat.doc

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "akey.hpp"
#include "allkeys.hpp"
#include "eliza.hpp"
#include "chatmain.hpp"

/////YOU MAY NOT change the next 2 lines.
const char eliza_title[]="chat by Christopher Busch  Copyright (c)1993";
const char eliza_version[]="version 1.0.0";



#ifdef UNIX
void randomize()
{
  srand((int)time(NULL ) );
}

int random(int x) 
{ 
  return rand() % x;
}

#endif //UNIX

eliza chatter;



extern "C" void startchat(char *filename)
{
  char buf[] = "";
  chatter.reducespaces(buf);
  chatter.loaddata(filename);
}

extern "C" const char* dochat(char* talker,char *msg,char* target)
{
  return chatter.process(talker,msg,target);
}





