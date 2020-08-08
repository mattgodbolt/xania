#pragma once

#include "chatmain.hpp"
#include "akey.hpp"

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
  allkeys() { current=first=top=nullptr; numkeys=0; contdbase=contnotset;}
  akeynode* curr() {return current;}
  akeynode* addkey();
  akeynode* reset();
  akeynode* advance();
  ~allkeys();
};
