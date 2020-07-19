
//Christopher Busch
//(c) 1993 all rights reserved.
//eliza.cpp


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <search.h>

#include "eliza.hpp"





bool eliza::addname(char* n,int d)  //add one name and number
{
  if(numnames<maxnames) {
    for(int i=strlen(n)-1;i>=0;i--) if(n[i]=='_') n[i]=' ';
//    printf("found person '%s' at %d \n",n,d);
    return thenames[numnames++].set(n,d) != NULL;
  }
  return false;
}


int namecompare(const void* a,const void* b)
{ 
  return strcasecmp( 
                    ((eliza::nametype*)a)->name,
                    ((eliza::nametype*)b)->name); 
}

void eliza::sortnames()
{  
  qsort((char*)thenames,numnames,sizeof(eliza::nametype),namecompare);
}
int eliza::getname(char* n)
{
  nametype finder;
  if(finder.set(n,0) == NULL) return 0;
  nametype* nt=(nametype*)
    bsearch((char*)&finder,(char*)thenames,numnames,
            sizeof(nametype),namecompare);
  if(nt==NULL) return 0;
  return nt->dbase;
}


void eliza::reducespaces(char *m)
{
  int len=strlen(m),space= m[0]<' ',count=0;
  for(int i=0;i<len;i++)    {
    if(!space || m[i]>' ' )      {
      m[count]= (m[i]<' ' ? ' ' : m[i]);
      count++;
    }
    space= m[i]<' ';
  }
  if(count) m[count]='\0';
}

//Gets a word out of a string.
int fggetword(char*& input,char* outword,char& outother) //0 if done
{
  outword[0]=0;
  char *outword0=outword;
  int curchar;
  curchar=input[0];

  while( isalnum(curchar) || curchar=='_' )
    {
      *outword=curchar;
      outword++;
      input++;
      //                printf(",%c;",*input); //debug
      curchar=*input;
    }
  if(*input!= '\0') input++;
  *outword='\0';
  outother=curchar;
  if( curchar == 0 && outword0[0]==0 ) return 0;
  return 1;
}

//add more then one name to some number
bool eliza::addbunch(char* names,int dbase) 
{ 
  char aword[MAXSIZE+3];
  char otherch;
  char* theinput=names;
  while( fggetword(theinput,aword,otherch)) {
    if(addname(aword,dbase) == false ) return false;
  }
  return true;
}

//This tries more exhaustively to find a name out of the database
//for example if 'Bobby Jo' fails, it will try Bobby then Jo.
int eliza::getanyname(char* n)
{
  int dbase=getname(n);
  if(dbase==0) {
    char aword[MAXSIZE+3];
    char otherch;
    char* theinput=n;
    while( fggetword(theinput,aword,otherch)) {
      dbase=getname(aword);
      if(dbase!=0) return dbase;
    } 
  }
  return dbase;
}



char* substit(char *in)
{
  static const char pairs[][10]=
    {"am","are",
     "I","you",
     "mine","yours",
     "my","your",
     "me","you",
     "myself","yourself",
     //swapped order:
     "you","I",
     "yours","mine",
     "your","my",
     "yourself","myself",
     "",""};

  for(int i=0;pairs[i*2][0]!='\0';i++)
    {
      if(strcasecmp(in,pairs[i*2])==0)
        {
          return (char*)pairs[i*2+1];
        }
    }
  return in;
}       

#include <unistd.h>

void fixgrammer(char s[])
{
  char newsent[MAXSIZE+20];
  newsent[0]='\0';
  
  char aword[MAXSIZE+3];
  char* theinput;       
  theinput=s;
  char otherch;
  char ministr[]=" ";   


  while( fggetword(theinput,aword,otherch))
    {
#ifdef DEBUG
      printf(">%s'%s' : '%s' , %d\n",newsent,theinput,aword,otherch);
#endif

      ministr[0]=otherch;
      strcat(newsent,substit(aword));           
      strcat(newsent,ministr);

    }
  strcpy(s,newsent);

}


//enables $variable translation
void eliza::addrest(char* replied, char* talker,
		    const char* rep,char* target,char* rest)
{
  trim(rest);
  char* i;
  char* point   = replied;
  const char* str     = rep;
  char* toofar = replied+repsize-1;
  while ( *str != '\0')    {
    if ( *str != '$' )      {
      *point++ = *str++;
      if(point==toofar) { *point='\0'; return; }
      continue;
    }
    ++str;
    switch( *str )         /*add $Variable commands here*/
      {
      case 'n':         
        i=talker;
        break;
      case 't': 
        i=target;
        break;
      case 'r':
        i=rest;
        break;
      case '$': {
        static char str[] = "$";
        i = str;
        break;
      }
      case 0:
        continue;
      case 'A':  //dont change THIS - Chris Busch  //author
	i=(char*)eliza_title;
	break; 
      case 'V': //dont change THIS!   //Version
	i=(char*)eliza_version;
	break;
      case 'C': { // dont change this!  //Compile time
        static char buf[1024];
        snprintf(buf, sizeof(buf), "%s %s", __DATE__, __TIME__);
        i = buf;
        break;
      }
      default: {
        static char undef[] = "$UNDEFINED";
        i = undef;
        break;
      }
      }
    ++str;
    while ( ( *point = *i ) != '\0' ) {
      ++point;
      ++i;
      if(point==toofar) { *point='\0'; return; }
    }
  }
  *point++ = '\0';
}


//nothing can be NULL!!!
const char* eliza::processdbase(char* talker,char* message,char* target,int dbase)
{
  static char replied[repsize];
  const char *rep=NULL;

  if( dbase<0 || dbase>numdbases) return "dbase error in processnumber";

  if(strlen(message)>MAXSIZE) return "You talk too much.";

  strcpy(replied,"I dont really know much about that, say more.");

  reducespaces(message);
  trim(message);
  int overflow=10; //runtime check so we dont have circular cont jumps
  do{
    thekeys[dbase].reset();
    while(thekeys[dbase].curr()!=NULL)    {
      int i=0,rest=0;
      if(match(thekeys[dbase].curr()->key.getlogic(),message,i,rest))
        {
          rep=thekeys[dbase].curr()->key.getrndreply();
          fixgrammer(&message[rest]);
          addrest(replied,talker,rep,target,&message[rest]);
          reducespaces(replied);
          return replied;
        }
      thekeys[dbase].advance();
    }
    dbase=thekeys[dbase].contdbase;
    overflow--;
  }while(dbase>=0 && overflow>0);
  if(overflow <=0) {
    return "potential circular cont (@) jump in databases";
  }
  return replied;
}


bool eliza::loaddata(char file[],char recurflag)
{
  //NOTE!!!! numdbases is the count of dbases!!!
  if(numdbases>=maxdbases) return false;

  FILE *data;
  char str[MAXSIZE];
  if((data=fopen(file,"rt"))==NULL)
    {
#ifdef DEBUG
      puts("File error!");
#endif
      return false;
    }
  int linecount=0;

  while(fgets(str,MAXSIZE-1,data)!=NULL)
    {
      linecount++;
#ifdef DEBUG
      puts(str);
#endif
      trim(str);
      if(strlen(str)>0)
        {
          if(str[0]>='1' && str[0]<='9')            {
            thekeys[numdbases].curr()->key.addreply(str[0]-'0',&(str[1]));
#ifdef DEBUG
            //puts("reply found");
#endif
          } else switch(str[0])    {
          case '\0' :
            break;
          case '(':
            thekeys[numdbases].addkey();
            thekeys[numdbases].curr()->key.addlogic(str);
#ifdef DEBUG
            //puts("logic found");
#endif
            break;
          case '#':
#ifdef DEBUG
            puts("rem");
#endif
            break;
          case '"':
            fprintf(stderr,"%s\n",&str[1]);
            break;
          case '\'':
            fprintf(stdout,"%s\n",&str[1]);
            break;
          case '>': //add another database
            if(numdbases<maxdbases-1) numdbases++;
            else 
	      fputs("response database is full. numdbases>maxdbases",stderr);
            if(!addbunch(&str[1],numdbases) ) {
              puts("name database is full.");
            }
            break;
	  case '%': //include another file inline
	    trim(str+1);
	    if(!loaddata(str+1,1)) //recurflag set
	      printf("including inline '%s' failed at %d!\n",str+1,linecount);
	    break;
	  case '@': //add continue to another database jump
	    if(thekeys[numdbases].contdbase==allkeys::contnotset) {
	      trim(str+1);
	      for(char* i=str+1;*i!=0;i++) if(*i=='_') *i=' ';
	      sortnames();
	      int contd=getname(str+1);
	      thekeys[numdbases].contdbase=contd;
//	      printf("found cont link %s to %d\n",str,contd);
	    } else 
	      printf(" @ continue already used for database %d at %d\n"
		     ,numdbases,linecount);
	    break;
	  default:
            printf("extraneous line: '%s' at %d\n",str,linecount);
          }
        }
    }
  fclose(data);
  if(!recurflag) {
    numdbases++;
    sortnames();
  }
  return true;
}


char* eliza::trim(char str[])
{
  int i,a,ln;
  for(ln=strlen(str);(ln>0) && (str[ln-1]<=' ');ln--);
  str[ln]=0;
  for(i=0;(i<ln) && (str[i]<=' ');i++);
  if(i>0)
    for(ln-=i,a=0;a<=ln;a++)
      str[a]=str[a+i];
  return str;
}




int eliza::doop(char op,int a,int b)
{
  switch(op)
    {
    case '\0':
    case '&': return a && b;
            case '|': return a || b;
            case '~': return a && !b;
            default:
#ifdef DEBUG
              printf("Errored Op! %c\n",op);
#endif
              return 0;
            }
}

int eliza::lowcase(char ch)
{
  if(ch>='A' && ch<='Z') return ch+32;
  return ch;
}

int eliza::strpos(char *s,char *sub)
{
  int i,a,lns=strlen(s),lnsub=strlen(sub),run,space=1;
  if(sub[0]=='=') {// = exact equality operator    
    return strcasecmp(&sub[1],s) ? 0 : lns;
  }
  if(sub[0]=='^'){  // match start operator    
    return strncasecmp(&sub[1],s,lnsub-1) ? 0 : lns;
  }
  if(lnsub>lns) return 0;

  run=lns-lnsub;
  for(i=0;i<=run;i++)
    {
      if(space)
        {
          for(a=0;a<lnsub && (lowcase(s[i+a])==sub[a]);a++);
#ifdef DEBUG
          //printf("hey %d r:%d",a,i+a);
#endif
          if(a==lnsub) return (i+a);
        }
      space=s[i]==' ';
    }

  return 0;
}

#define encounterop                  \
trim(ph);                    \
if(strlen(ph))               \
{                            \
        a=strpos(m,ph);      \
        if(a>0) rest=a;      \
        res=doop(op,res,a);  \
}                            \
len=ph[0]=0;                 



int eliza::match(char s[],char m[],int& in,int& rest)
{
  int l=strlen(s),res=1,op='\0',a;
  char ph[MAXSIZE];
  int len=0;
  ph[0]=0;
#ifdef DEBUG
  //puts(s);
  //puts(m);
#endif
  while(s[in]!='(') in++;
  in++;
  for(;in<l;in++)
    {
      switch(s[in])
        {
        case '(':
#ifdef DEBUG
          //printf("in %d into...\n",in);
#endif
          a=match(s,m,in,rest);
          if(op=='\0') res=a;
          else
            res=doop(op,res,a);
          break;
        case '&':
          encounterop
            op='&';
          break;
        case '|':
          encounterop
            op='|';
          break;
        case '~':
          encounterop
            op='~';
          break;
        case ')':
#ifdef DEBUG
          //printf("p:'%s'\n",ph);
#endif
          trim(ph);
          if(strlen(ph))
            {
              a=strpos(m,ph);
              if(a>0) rest=a;
              return doop(op,res,a);
            } else return res;

          break;
        default:
          ph[len++]=s[in];
          ph[len]='\0';
        }

    }
  return res;
}

