
//eliza.hpp
//chris busch (c) copyright 1995 all rights reserved

#ifndef ELIZA_HPP
#define ELIZA_HPP

#include <string.h>
#include "chatmain.hpp"
#include "allkeys.hpp"

//if set eliza will use more of an exhaustive search to find a name.
// if looking for 'Bobby Jo' this will look for 'Bobby Jo' then
// Bobby and finally Jo.
#define USE_EX_SEARCH	
		

class eliza
{
public:
  enum {maxdbases=100,maxnames=200,defaultdbase=0};
  enum { repsize=150}; //reply size

  struct nametype {
    char* name;
    int dbase;

    char* set(char* n,int d) { name=strdup(n); dbase=d; return name; }
    nametype() { name=nullptr; dbase=0; }
    ~nametype() { if(name) free(name); }
  };
  static char* trim(char str[]);
protected:
  int numdbases,numnames;
  allkeys thekeys[maxdbases];
  nametype thenames[maxnames];


  int doop(char op,int a,int b);
  int lowcase(char ch);
  int strpos(char *s,char *sub);
  int match(char s[],char m[],int& in,int&);

  bool addname(char*,int);  //add one name and number
  bool addbunch(char*, int); //add more then one name to some number
  void sortnames();
  int getname(char*);     //only tries once to find a name
  int getanyname(char*);  //more exhaustive search to find name

public:
  void reducespaces(char *);
  void addrest(char* replied, char* talker,
		       const char* rep, char* target,char* rest);
public:
    bool loaddata(const char *, char recurflag = 0);
    const char *processdbase(char *talker, char *message, char *target, int dbase);
    const char* process(char* talker,char* message,char* target)    {
#ifdef USE_EX_SEARCH	
    return processdbase(talker,message,target,getanyname(talker));
#else
    return processdbase(talker,message,target,getname(talker));
#endif
  }

  eliza() { numdbases=0; numnames=0; char de[] = "default"; addname(de,0); }

};

#endif 



