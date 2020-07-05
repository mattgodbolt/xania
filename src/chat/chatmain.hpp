
//chatmain.hpp
//chris busch

#ifndef CHATMAIN_HPP
#define CHATMAIN_HPP

//set to DEBUG for debugging
//set to TEST for testing
//set to CHECKMEM for checking memory

#define DEBUGoff
#define TESToff			
#define CHECKMEM



//dont define UNIX if using msdos.
#define UNIX

//maximum size of an input string.
#define MAXSIZE 100

//enum bool {false,true};
#define BOOL int
#define false 0
#define true 1
#ifndef NULL
	#define NULL 0
#endif

#ifdef UNIX
	void randomize();
	int random(int);
#endif


extern const char eliza_title[];
extern const char eliza_version[];



#endif //CHATMAIN_HPP









