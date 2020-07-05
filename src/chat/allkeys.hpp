
//allkeys.hpp


#ifndef ALLKEYS_HPP
#define ALLKEYS_HPP

#include "chatmain.hpp"
#include "akey.hpp"

#include <stdlib.h> //for NULL


struct akeynode
{
  akey key;
  akeynode *next;
};



class allkeys
{
protected:
  akeynode *first,*current,*top;
  int numkeys;
public:
  enum {contnotset=-1};
  int contdbase; //signifies which datbase to use if this one fell thru
  allkeys() { current=first=top=NULL; numkeys=0; contdbase=contnotset;}
  akeynode* curr() {return current;}
  akeynode* addkey();
  akeynode* reset();
  akeynode* advance();
  ~allkeys();
};

#endif
