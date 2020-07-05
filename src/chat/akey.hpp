
//akey.hpp
//chris busch

#ifndef AKEY_HPP
#define AKEY_HPP

#include "chatmain.hpp"

//#define REDUCEWATES //in muds you dont want this.

struct reply
{
	int wate;
	char *sent;
};



class akey
{
	protected:
		char *logic;
		int numreplys,totalwates;
		reply *replys;
	public:
		akey();
		int addlogic(char *);
		char *getlogic() {return logic;}
		int addreply(int,char *);
		reply& getreply(int num) {return replys[num];}
		const char* getrndreply();
		~akey();
};


#endif
