/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  news.h:  the Xanian news system, based on USENET newsgroups          */
/*                                                                       */
/*************************************************************************/

#pragma once

/* Structures for news - mostly self-explanatory */

// TODO extract and make this not terrible. taken from merc.h
typedef struct char_data CHAR_DATA;

extern int cur_msg_id;

#define MES_HASH 32

typedef struct _MES_ID {
    struct _MES_ID *next;
    int id;
} MES_ID;

typedef struct _ARTICLE {
    struct _ARTICLE *next;
    char *author;
    char *text;
    int time_sent;
    int msg_id;
} ARTICLE;

typedef struct _THREAD {
    struct _THREAD *next;
    struct _ARTICLE *articles;
    char *subject;
    int num_articles;
    int flags;
    int expiry;
} THREAD;

/* Defines for news */
#define NEWS_FILE "news.txt"

#define DEFAULT_EXPIRY 7 /* That is, one week */

/* Thread flags */
/* note: uses the flagging sys with permissions. If you start changing
   the number of levels in the mud, don't forget to tweak things below
   like 100imponly! :)
   */

#define THREAD_READ_ONLY 1
#define THREAD_IMMORTAL_ONLY 2
#define THREAD_CLAN_ONLY 4
#define THREAD_DELETED 8
#define THREAD_IMPLEMENTOR_ONLY 16
#define THREAD_FLAGS "readonly immortalonly clanonly 98deleted 100imponly"

/* Globals */
extern THREAD *thread_head;
extern THREAD *thread_free;
extern ARTICLE *article_free;

/* Function prototypes */
THREAD *new_thread();
void free_thread(THREAD *thread);
ARTICLE *new_article();
void free_article(ARTICLE *article);
void read_news();
void save_news();
bool has_read_before(CHAR_DATA *ch, int mes_id);
void mark_as_read(CHAR_DATA *ch, int mes_id);
void mark_as_unread(CHAR_DATA *ch, int mes_id);
bool article_exists(int mes_id);
void news_info(CHAR_DATA *ch);
void move_to_next_unread(CHAR_DATA *ch);
int num_unread(CHAR_DATA *ch, THREAD *t);
bool can_read_thread(CHAR_DATA *ch, THREAD *t);
bool can_write_thread(CHAR_DATA *ch, THREAD *t);
