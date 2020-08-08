//chris busch

#pragma once

#include "chatmain.hpp"

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
};
