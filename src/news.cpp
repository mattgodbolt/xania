/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  news.c:  the Xanian news system, based on USENET newsgroups          */
/*                                                                       */
/*************************************************************************/

/*
 * This news system first appeared in Xania mud - telnet://xania.org:9000
 * You are free to use it as you see fit provided this message is kept and
 * the authors are given credit.  The authors, in this case, are
 * TheMoog (matt@godbolt.org)
 * Faramir ()
 */

#include "news.h"
#include "Descriptor.hpp"
#include "buffer.h"
#include "flags.h"
#include "merc.h"
#include "string_utils.hpp"

#include <cstdio>
#include <cstdlib>
#include <sys/time.h>

void _do_news(CHAR_DATA *ch, const char *arg);

/* Globals */
THREAD *thread_free = nullptr;
ARTICLE *article_free = nullptr;
THREAD *thread_head = nullptr;

int cur_msg_id; // Current message ID - must be completely unique across whole Mud
static const char *news_name = "news"; // The name of the command being used - either 'news' or 'mail' to
                                       // allow players with speed-walkers access to the news commands

/* Create a new, blank thread, not chained onto the global linked list */
THREAD *new_thread() {
    THREAD *res;
    if (thread_free) {
        res = thread_free;
        thread_free = thread_free->next;
    } else {
        res = (THREAD *)alloc_perm(sizeof(THREAD));
    }
    res->next = nullptr;
    res->articles = nullptr;
    res->subject = nullptr;
    res->num_articles = 0;
    res->expiry = DEFAULT_EXPIRY;
    res->flags = 0;
    return res;
}

/* Remove a thread, does not unchain */
void free_thread(THREAD *thread) {
    if (thread->articles) { // If there are any articles in this thread then free them
        ARTICLE *article, *next;
        for (article = thread->articles; article; article = next) {
            next = article->next; /* as we are about to free article we take a copy of its next field */
            free_article(article);
        }
    }
    if (thread->subject)
        free_string(thread->subject); // Free the thread's subject
    thread->next = thread_free; // Chain the thread on to the list of free threads
    thread_free = thread;
}

/* Create a new, blank article - UPDATES cur_msg_id */
ARTICLE *new_article() {
    ARTICLE *res;
    if (article_free) {
        res = article_free;
        article_free = article_free->next;
    } else {
        res = (ARTICLE *)alloc_perm(sizeof(ARTICLE));
    }
    res->next = nullptr;
    res->author = nullptr;
    res->text = nullptr;
    res->msg_id = cur_msg_id++; // Give the message id to this article, and increment it
    return res;
}

/* Free an article - without removing from any linked lists it may be in */
void free_article(ARTICLE *article) {
    if (article->author)
        free_string(article->author); // Free the associated strings
    if (article->text)
        free_string(article->text);
    article->next = article_free; // Chain on to the linked list of free articles
    article_free = article;
}

/* Save the list of news */
void save_news() {
    FILE *fp;
    ARTICLE *art;
    THREAD *thread;

    fclose(fpReserve);
    if ((fp = fopen(NEWS_FILE, "w")) == nullptr) {
        fpReserve = fopen(NULL_FILE, "r");
        bug("Aaargh!  Couldn't write to note file"); // This should never happen.  I hope
        return;
    }

    fprintf(fp, "%d\n", cur_msg_id); // Write out the current message id

    for (thread = thread_head; thread; thread = thread->next) { // Save each thread and chain of articles
        if (IS_SET(thread->flags, THREAD_DELETED))
            continue;
        fprintf(fp, "#THREAD %d %d %s~\n", thread->flags, thread->expiry, thread->subject);
        for (art = thread->articles; art; art = art->next)
            fprintf(fp, "author %s~\ntime %d\nmsgid %d\n%s~\n", art->author, art->time_sent, art->msg_id, art->text);
    }

    if (thread_head == nullptr) // If we didn't save anything, then delete the otherwise
        unlink(NEWS_FILE); // empty file

    fclose(fp);

    fpReserve = fopen(NULL_FILE, "r");
}

/* Load in the news */
void load_news() {
    FILE *fp;
    THREAD *last_thread = nullptr;

    cur_msg_id = 0; /* Failsafe */

    if ((fp = fopen(NEWS_FILE, "r")) == nullptr)
        return;

    cur_msg_id = fread_number(fp); // Read in the message id
    fread_to_eol(fp);

    for (;;) {
        int c;
        THREAD *thread;
        ARTICLE *last_article = nullptr;

        if (feof(fp)) {
            fclose(fp);
            return;
        }

        c = getc(fp);
        if (c != '#') {
            bug("Missing # in newsfile");
            return;
        }

        if (str_cmp("THREAD", fread_word(fp))) {
            bug("Expected #THREAD in newsfile");
            return;
        }

        thread = new_thread(); /* Create the new thread */
        thread->flags = fread_number(fp);
        thread->expiry = fread_number(fp);
        thread->subject = str_dup(fread_string(fp));
        thread->articles = nullptr; /* Paranoia */
        thread->num_articles = 0; // NB at this point the thread is not linked onto the
                                  // global linked list

        for (;;) {
            ARTICLE *article;
            int c;

            c = getc(fp);
            ungetc(c, fp);
            if (c == '#' || c == EOF)
                break;

            article = new_article();

            if (str_cmp("author", fread_word(fp))) {
                bug("Missing author field in article");
                return;
            }
            article->author = str_dup(fread_string(fp));

            if (str_cmp("time", fread_word(fp))) {
                bug("Missing time field in article");
                return;
            }
            article->time_sent = fread_number(fp);
            if (str_cmp("msgid", fread_word(fp))) {
                bug("Missing msgid field in article");
                return;
            }
            article->msg_id = fread_number(fp);

            article->text = str_dup(fread_string(fp));
            fread_to_eol(fp);

            if (article->time_sent < current_time - (thread->expiry * 24 * 60 * 60) && (thread->expiry != 0)) {
                free_article(article);
            } else {
                thread->num_articles++;
                if (thread->articles == nullptr)
                    thread->articles = article;
                if (last_article)
                    last_article->next = article;
                last_article = article;
            }
        }
        if (thread->num_articles == 0) { /* Blank thread */
            free_thread(thread);
        } else { /* Not a blank thread */
            if (thread_head == nullptr)
                thread_head = thread; // If this is the first thread, then point the head of
                                      // the linked list onto this one
            if (last_thread)
                last_thread->next = thread;
            last_thread = thread; // Chain onto the linked list
        }
    }
}

/* The user command 'news' */
void do_news(CHAR_DATA *ch, const char *argument) {
    news_name = "news"; // Set the news_name to be the same command as that the
                        // user used, as not to confuse them later on
    _do_news(ch, argument);
}

void do_mail(CHAR_DATA *ch, const char *argument) {
    news_name = "mail"; // Same as above
    _do_news(ch, argument);
}

/* ------------- The actual 'do' routines -------------- */

/* Find the next item of news to read */
void do_news_next(CHAR_DATA *ch, const char *argument) {
    (void)argument;
    char buf[MAX_STRING_LENGTH];
    if (ch->thread == nullptr) { // The user has already 'fallen off' the end of the
        // linked list - so check for any more unread threads
        ch->thread = thread_head; // Put user to top of linked list
        ch->article = (ch->thread) ? ch->thread->articles : nullptr; // Set up article appropriately
        move_to_next_unread(ch);
        if (ch->thread == nullptr) { // Still no news to read?
            snprintf(buf, sizeof(buf), "You have no more %s to read.\n\r", news_name);
            send_to_char(buf, ch);
            return;
        }
    }
    if (ch->article == nullptr) { // Has the user read all of the articles in this thread?
        move_to_next_unread(ch);
        if (ch->thread == nullptr) {
            send_to_char("No more articles or threads.\n\r", ch); // No more threads
            return;
        }
    }
    snprintf(buf, sizeof(buf), "%s[%3d/%-3d] - %s\n\r", (ch->article == ch->thread->articles) ? "|WNew thread: |w" : "",
             ch->articlenum, ch->thread->num_articles, ch->thread->subject);
    send_to_char(buf, ch);
    snprintf(buf, sizeof(buf), "From: %s\n\rDate: %s\r", ch->article->author, ctime((time_t *)&ch->article->time_sent));
    send_to_char(buf, ch);
    page_to_char(ch->article->text, ch);
    mark_as_read(ch, ch->article->msg_id);
    move_to_next_unread(ch);
}

/* Display a formatted list of the threads */
void do_news_list(CHAR_DATA *ch, const char *argument) {
    (void)argument;
    BUFFER *buffer = buffer_create();
    THREAD *t = thread_head;
    int numt = 0;

    send_to_char("|r[ |gThread# |r] |gOriginator      Subject                "
                 "            #messages|w\n\r",
                 ch);
    for (; t; t = t->next) {
        if (can_read_thread(ch, t)) {
            buffer_addline_fmt(buffer, "|r[   |w%3d   |r] |w%-15s %-35s %d/%d%s%s\n\r", ++numt, t->articles->author,
                               t->subject, num_unread(ch, t), t->num_articles,
                               (IS_SET(t->flags, THREAD_READ_ONLY)) ? " (r/o)" : "",
                               (IS_SET(t->flags, THREAD_DELETED)) ? " |R*del*" : "");
        }
    }
    buffer_send(buffer, ch); // This frees up the buffer as well
}

/* Send a formatted list of articles in the user's current thread */
void do_news_articles(CHAR_DATA *ch, const char *argument) {
    (void)argument;
    BUFFER *buffer = buffer_create();
    ARTICLE *a;
    int mesnum = 1;

    if (ch->thread == nullptr) {
        send_to_char("You have no thread currently selected.\n\r", ch);
        return;
    }
    a = ch->thread->articles;
    buffer_addline_fmt(buffer, "Articles for thread '|W%s|w':\n\r", ch->thread->subject);
    buffer_addline_fmt(buffer, "|r[|g Message# |r] |gAuthor       Date|w\n\r");
    for (; a; a = a->next) {
        buffer_addline_fmt(buffer, "|r[ |Y%s |w%3d    |r]|w %-13s%s\r", has_read_before(ch, a->msg_id) ? " " : "*",
                           mesnum++, a->author, ctime((time_t *)&a->time_sent));
    }
    buffer_addline_fmt(buffer, "|g= %d articles, %d unread.|w\n\r", ch->thread->num_articles,
                       num_unread(ch, ch->thread));
    buffer_send(buffer, ch);
}

/* Read a specific article within a thread */
void do_news_read(CHAR_DATA *ch, const char *argument) {
    char num[MAX_INPUT_LENGTH];
    int mesnum, current;
    ARTICLE *art;
    one_argument(argument, num);
    if (ch->thread == nullptr) {
        send_to_char("You have no thread currently selected.\n\r", ch);
        return;
    }
    if ((mesnum = atoi(num)) <= 0) { // was that a number?
        send_to_char("Usage: news read <num>\n\r", ch);
        return;
    }
    current = 1;
    for (art = ch->thread->articles; art && (mesnum != current++); art = art->next)
        ; // Step through
    if (art == nullptr) { // Didn't find the message
        snprintf(num, sizeof(num), "Couldn't find message number %d in current thread.\n\r", mesnum);
        send_to_char(num, ch);
        return;
    }
    ch->article = art; // Make char's article this one
    ch->articlenum = mesnum; // Make char's articlenumber this one
    do_news_next(ch, ""); // And display this message
}

/* Allow the user to select a thread */
void do_news_thread(CHAR_DATA *ch, const char *argument) {
    char num[MAX_INPUT_LENGTH];
    int threadnum, current;
    THREAD *t;
    one_argument(argument, num);
    if ((threadnum = atoi(num)) <= 0) { // Was that a number?
        send_to_char("Usage: news thread <num>\n\r", ch);
        return;
    }
    current = 1;
    for (t = thread_head; t; t = t->next) {
        if (!can_read_thread(ch, t))
            continue;
        if (threadnum == current)
            break;
        current++;
    }
    if (t == nullptr) {
        snprintf(num, sizeof(num), "Couldn't find thread %d.\n\r", threadnum);
        send_to_char(num, ch);
        return;
    }
    snprintf(num, sizeof(num), "Thread '|W%s|w' selected (#%d%s).\n\r", t->subject, threadnum,
             IS_SET(t->flags, THREAD_READ_ONLY) ? ",read only" : "");
    send_to_char(num, ch);
    ch->thread = t;
    ch->article = t->articles;
    ch->articlenum = 1;
}

/* Skip to next unread thread - leaving all messages in this thread unread */
void do_news_skip(CHAR_DATA *ch, const char *argument) {
    (void)argument;
    if (ch->thread) { // If the user isn't already at the end - move them on
        do {
            ch->thread = ch->thread->next;
        } while (ch->thread && !can_read_thread(ch, ch->thread));
    }
    if (ch->thread == nullptr) { // Either the user was at the end, or is now
        send_to_char("No more threads.\n\r", ch);
        return;
    }
    ch->article = ch->thread->articles;
    ch->articlenum = 1;
    do_news_next(ch, ""); // Display the message
}

/* Begin a new thread, given the subject */
void do_news_compose(CHAR_DATA *ch, const char *argument) {
    char buf[MAX_STRING_LENGTH];
    if (ch->newsbuffer) {
        snprintf(buf, sizeof(buf), "You are already composing a %s message!\n\r", news_name);
        send_to_char(buf, ch);
        return;
    }
    if (argument[0] == '\0') {
        snprintf(buf, sizeof(buf), "Usage: %s compose <subject>\n\r", news_name);
        send_to_char(buf, ch);
        return;
    }
    auto subject = smash_tilde(argument);
    ch->newsbuffer = buffer_create();
    ch->newssubject = str_dup(subject.c_str()); // TODO this hurts
    ch->newsfromname = str_dup(ch->name);
    snprintf(buf, sizeof(buf), "Composing new %s thread with subject '%s'\n\r", news_name, argument);
    send_to_char(buf, ch);
}

/* Reply to an existing thread */
void do_news_reply(CHAR_DATA *ch, const char *argument) {
    (void)argument;
    char buf[MAX_STRING_LENGTH];
    if (ch->thread == nullptr) {
        send_to_char("You have no thread selected to reply to.\n\r", ch);
        return;
    }
    if (ch->newsbuffer) {
        snprintf(buf, sizeof(buf), "You are already composing a %s message.\n\r", news_name);
        send_to_char(buf, ch);
        return;
    }
    if (ch->thread == nullptr) {
        send_to_char("You have no thread currently selected.\n\r", ch);
        return;
    }
    if (!can_write_thread(ch, ch->thread)) {
        send_to_char("This thread is a read only thread.\n\r", ch);
        return;
    }
    ch->newsbuffer = buffer_create();
    ch->newsreply = ch->thread; // Set up the reply thread, as user may wish
                                // to read other threads while editing their message
    ch->newsfromname = str_dup(ch->name);
    snprintf(buf, sizeof(buf), "Replying to thread '%s'\n\r", ch->thread->subject);
    send_to_char(buf, ch);
}

/* Clear the currently-being-prepared message */
void do_news_clear(CHAR_DATA *ch, const char *argument) {
    (void)argument;
    if (ch->newsbuffer == nullptr) {
        char buf[MAX_STRING_LENGTH];
        snprintf(buf, sizeof(buf), "You aren't composing a %s message.\n\r", news_name);
        send_to_char(buf, ch);
        return;
    }
    buffer_destroy(ch->newsbuffer); // Free all the memory that may have been alloced
    ch->newsbuffer = nullptr;
    if (ch->newssubject) {
        free_string(ch->newssubject);
        ch->newssubject = nullptr;
    }
    if (ch->newsfromname) {
        free_string(ch->newsfromname);
        ch->newsfromname = nullptr;
    }
    send_to_char("Message cleared.\n\r", ch);
}

/* Actually post an article and thread into the linked list of articles/and or create a new thread */
void do_news_post(CHAR_DATA *ch, const char *argument) {
    (void)argument;
    if (ch->newsbuffer == nullptr) { // Is the user actually composing a message?
        char buf[MAX_STRING_LENGTH];
        snprintf(buf, sizeof(buf), "You aren't composing a %s message.\n\r", news_name);
        send_to_char(buf, ch);
        return;
    }
    if (!buffer_string(ch->newsbuffer)) {
        send_to_char("No body of article to post!\n\r", ch);
        return;
    }
    if (ch->newssubject) { // If subject is non-nullptr then it must be a new thread we are creating
        THREAD *add = new_thread();
        THREAD *t = thread_head;
        ARTICLE *art;
        if (t == nullptr) { // If there are no threads at all - then this is the first one
            thread_head = add;
            add->next = nullptr;
        } else {
            for (; t->next; t = t->next)
                ; // Skip to one-before end of linked list
            t->next = add; // Chain onto list here
            add->next = nullptr;
        }
        add->subject = str_dup(ch->newssubject); // Dup the subject for this thread's subject
        art = new_article(); // Create a new article
        add->articles = art; // Chain the article onto the thread's l_list
        add->num_articles = 1;
        art->author = str_dup(ch->newsfromname); // Dupe the author's name
        art->time_sent = current_time;
        art->next = nullptr; // Only one article - so no ->next
        art->text = str_dup(buffer_string(ch->newsbuffer)); // Dup the body of the article
        buffer_destroy(ch->newsbuffer); // Remove the ch's news buffer
        ch->newsbuffer = nullptr; // Make nullptr so ch can write a new message
        free_string(ch->newssubject);
        ch->newssubject = nullptr; // Release more stuff
        free_string(ch->newsfromname);
        ch->newsfromname = nullptr;
        ch->thread = add; // Place ch's thread and art to point at the new message
        ch->article = art;
        ch->articlenum = 1;
        send_to_char("New thread created successfully.\n\r", ch); // Hooray!
    } else { // This must be a reply as newssubject==nullptr
        ARTICLE *art;
        ARTICLE *add = new_article(); // Generate the new article
        for (art = ch->newsreply->articles; art->next; art = art->next)
            ; // Find last art (ch->newsreply is nonnull)
        art->next = add; // Thread on article
        add->next = nullptr; // No more articles after this one
        add->time_sent = current_time;
        add->text = str_dup(buffer_string(ch->newsbuffer)); // Dup the body of the art
        add->author = str_dup(ch->newsfromname);
        ch->thread = ch->newsreply;
        ch->article = add; // Point char at the message they've just created
        ch->articlenum = ++ch->newsreply->num_articles; // Inc num of arts in this thread, and point char to this one
        buffer_destroy(ch->newsbuffer); // Free user's newsbuffer
        ch->newsbuffer = nullptr;
        free_string(ch->newsfromname);
        ch->newsfromname = nullptr;
        send_to_char("Thread replied to ok.\n\r", ch);
    }
    save_news(); // Commit the changes to the news database
}

/* Add a line to the message being edited */
void do_news_plus(CHAR_DATA *ch, const char *argument) {
    if (ch->newsbuffer == nullptr) { // Are they composing a message?
        char buf[MAX_STRING_LENGTH];
        snprintf(buf, sizeof(buf), "You aren't composing a %s message.\n\r", news_name);
        send_to_char(buf, ch);
        return;
    }
    auto line = smash_tilde(argument); // Kill those tildes
    buffer_addline(ch->newsbuffer, line.c_str()); // Add the line...
    buffer_addline(ch->newsbuffer, "\n\r"); // ... and a \n\r
    send_to_char("Ok.\n\r", ch);
}

/* Remove a line from the message */
void do_news_minus(CHAR_DATA *ch, const char *argument) {
    (void)argument;
    if (ch->newsbuffer == nullptr) {
        char buf[MAX_STRING_LENGTH];
        snprintf(buf, sizeof(buf), "You aren't composing a %s message.\n\r", news_name);
        send_to_char(buf, ch);
        return;
    }
    buffer_removeline(ch->newsbuffer); // Thank God I wrote the buffer routines :)
    send_to_char("Ok.\n\r", ch);
}

/* Show the text being edited */
void do_news_show(CHAR_DATA *ch, const char *argument) {
    (void)argument;
    if (ch->newsbuffer == nullptr) {
        char buf[MAX_STRING_LENGTH];
        snprintf(buf, sizeof(buf), "You aren't composing a %s message.\n\r", news_name);
        send_to_char(buf, ch);
        return;
    }
    if (buffer_string(ch->newsbuffer)) {
        page_to_char(buffer_string(ch->newsbuffer), ch);
    }
}

/* Mark all articles as read */
void do_news_catchup(CHAR_DATA *ch, const char *argument) {
    (void)argument;
    ARTICLE *a;
    if (ch->thread == nullptr) {
        send_to_char("You have no thread selected.\n\r", ch);
        return;
    }
    for (a = ch->thread->articles; a; a = a->next)
        mark_as_read(ch, a->msg_id);
    send_to_char("Thread marked as read.\n\r", ch);
    move_to_next_unread(ch);
}

/* Mark all articles as unread */
void do_news_uncatchup(CHAR_DATA *ch, const char *argument) {
    (void)argument;
    ARTICLE *a;
    if (ch->thread == nullptr) {
        send_to_char("You have no thread selected.\n\r", ch);
        return;
    }
    for (a = ch->thread->articles; a; a = a->next)
        mark_as_unread(ch, a->msg_id);
    send_to_char("Thread marked as unread.\n\r", ch);
    move_to_next_unread(ch);
}

/* Delete an article within a thread
 * Be careful to make sure nobody has hanging pointer refs */
void do_news_delete(CHAR_DATA *ch, const char *argument) {
    int artnum;
    ARTICLE *art;
    Descriptor *d;
    THREAD *chthread;
    int n;
    if (ch->thread == nullptr) { // Check to see if char has a thread sleceted
        send_to_char("You have no thread currently selected.\n\r", ch);
        return;
    }
    if ((artnum = atoi(argument)) <= 0) { // We need a non-negative numeric
        char buf[MAX_STRING_LENGTH];
        snprintf(buf, sizeof(buf), "Usage : %s delete <article number>\n\r", news_name);
        send_to_char(buf, ch);
        return;
    }
    for (art = ch->thread->articles, n = 1; art && n != artnum; art = art->next, n++)
        ; // Find the article
    if (art == nullptr) { // Did we find the article?
        send_to_char("Article not found.\n\r", ch);
        return;
    }
    if ((get_trust(ch) < SUPREME) && str_cmp(art->author, ch->name)) { // Does the ch have enough privs
                                                                       // to delete this article?
        send_to_char("You cannot delete that message.\n\r", ch);
        return;
    }

    chthread = ch->thread; // Take a copy of this as it may change after the next bit...
    /* Check to see if anyone is 'reading' this message currently, if so move them on to the next */
    for (d = descriptor_list; d; d = d->next) {
        CHAR_DATA *person;
        if (d->is_playing()) {
            person = d->original ? d->original : d->character; // Find the corresponding person assoc'd with d
            if ((person != nullptr) && // Do they exist?
                (person->article == art)) // Are they looking at this article?
                move_to_next_unread(person); // If so - move them on
        }
    }

    if (art == chthread->articles) { // Was this article the top article?
        chthread->articles = art->next;
    } else { // Unchain from inside the l_list
        ARTICLE *a;
        for (a = chthread->articles; a->next; a = a->next) {
            if (a->next == art)
                break;
        }
        if (a->next == nullptr) {
            bug("Article not found in news delete");
        } else {
            a->next = a->next->next;
        }
    }
    if (--chthread->num_articles == 0) { // Have we just deleted the last message in the thread?
        THREAD *t = thread_head;
        /* Check to see if anyone is 'reading' this thread */
        for (d = descriptor_list; d; d = d->next) {
            CHAR_DATA *person;
            if (d->is_playing()) {
                person = d->original ? d->original : d->character; // Find the corresponding person assoc'd with d
                if ((person != nullptr) && // Do they exist?
                    (person->thread == chthread)) { // Are they looking at this thread?
                    person->article = nullptr;
                    move_to_next_unread(person); // If so - move them on to next thread
                }
            }
        }
        if (t == chthread) {
            thread_head = t->next;
        } else {
            for (; t->next; t = t->next) {
                if (t->next == chthread) {
                    t->next = t->next->next;
                    break;
                }
            }
        }
        free_thread(chthread);
    }

    free_article(art);
    save_news();
    send_to_char("Article deleted.\n\r", ch);
}

/* Set flags on a news thread */
void do_news_flags(CHAR_DATA *ch, const char *argument) {
    int flags;
    if (ch->thread == nullptr) {
        send_to_char("You have no thread currently selected.\n\r", ch);
        return;
    }
    send_to_char("Flags set for this thread are:\n\r", ch);
    flags = (int)flag_set(THREAD_FLAGS, argument, ch->thread->flags, ch); // Long Live Flags (tm)
    if (flags != ch->thread->flags) { // Have the flags changed?
        ch->thread->flags = flags; // If so - update...
        save_news(); // ....and commit to the news database
    }
}

/* Masquerade as another person */
void do_news_from(CHAR_DATA *ch, const char *argument) {
    if (argument[0] == '\0') {
        char buf[MAX_STRING_LENGTH];
        snprintf(buf, sizeof(buf), "Usage: %s from <pseudonym>.\n\r", news_name);
        send_to_char(buf, ch);
        return;
    }
    if (ch->newsbuffer == nullptr) {
        send_to_char("You are not writing a message.\n\r", ch);
        return;
    }
    free_string(ch->newsfromname);
    ch->newsfromname = str_dup(argument);
    send_to_char("Ok.\n\r", ch);
}

/* Set the article expiry of the thread */
void do_news_expiry(CHAR_DATA *ch, const char *argument) {
    char buf[MAX_STRING_LENGTH];
    if (ch->thread == nullptr) {
        send_to_char("You have no thread currently selected.\n\r", ch);
        return;
    }
    if (argument[0] == '\0') { // Is this an enquiry?
        if (ch->thread->expiry) {
            snprintf(buf, sizeof(buf), "The thread '%s' has an expiry time of %d day%s.\n\r", ch->thread->subject,
                     ch->thread->expiry, (ch->thread->expiry == 1) ? "" : "s");
        } else {
            snprintf(buf, sizeof(buf), "The thread '%s' does not expire.\n\r", ch->thread->subject);
        }
        send_to_char(buf, ch);
        return;
    }
    if ((ch->thread->expiry = atoi(argument)) == 0) {
        snprintf(buf, sizeof(buf), "The thread '%s' now does not expire.\n\r", ch->thread->subject);
    } else {
        snprintf(buf, sizeof(buf), "The thread '%s' now expires after %d day%s.\n\r", ch->thread->subject,
                 ch->thread->expiry, (ch->thread->expiry == 1) ? "" : "s");
    }
    save_news();
    send_to_char(buf, ch);
}

/* The actual news or mail command itself - basically a big dispatcher */
void _do_news(CHAR_DATA *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];

    if (IS_NPC(ch))
        return; /* switched IMMs can go jump */

    argument = one_argument(argument, arg);

    if (arg[0] == '\0' || !str_prefix(arg, "next")) {
        do_news_next(ch, argument);
        return;
    }

    if (!str_prefix(arg, "list")) {
        do_news_list(ch, argument);
        return;
    }

    if (!str_prefix(arg, "articles")) {
        do_news_articles(ch, argument);
        return;
    }

    if (!str_prefix(arg, "read")) {
        do_news_read(ch, argument);
        return;
    }

    if (!str_prefix(arg, "thread")) {
        do_news_thread(ch, argument);
        return;
    }

    if (!str_prefix(arg, "skip")) {
        do_news_skip(ch, argument);
        return;
    }

    if (!str_prefix(arg, "compose")) {
        do_news_compose(ch, argument);
        return;
    }

    if (!str_prefix(arg, "reply")) {
        do_news_reply(ch, argument);
        return;
    }

    if (!str_prefix(arg, "clear")) {
        do_news_clear(ch, argument);
        return;
    }

    if (!str_prefix(arg, "post")) {
        do_news_post(ch, argument);
        return;
    }

    if (arg[0] == '+') {
        do_news_plus(ch, argument);
        return;
    }

    if (arg[0] == '-') {
        do_news_minus(ch, argument);
        return;
    }

    if (!str_prefix(arg, "show")) {
        do_news_show(ch, argument);
        return;
    }

    if (!str_prefix(arg, "catchup")) {
        do_news_catchup(ch, argument);
        return;
    }

    if (!str_prefix(arg, "uncatchup")) {
        do_news_uncatchup(ch, argument);
        return;
    }

    if (!str_prefix(arg, "delete")) {
        do_news_delete(ch, argument);
        return;
    }

    if (!str_prefix(arg, "flags") && (IS_IMMORTAL(ch))) {
        do_news_flags(ch, argument);
        return;
    }

    if (!str_prefix(arg, "expiry") && (get_trust(ch) >= SUPREME)) {
        do_news_expiry(ch, argument);
        return;
    }

    if (!str_prefix(arg, "from") && get_trust(ch) >= SUPREME) {
        do_news_from(ch, argument);
        return;
    }

    {
        BUFFER *buffer = buffer_create();
        buffer_addline_fmt(buffer,
                           "Usage: %s next              - read next article\n\r"
                           "       %s read <num>        - read article num in current thread\n\r"
                           "       %s list              - list all threads\n\r"
                           "       %s thread <num>      - select thread num\n\r"
                           "       %s articles          - list articles in current thread\n\r"
                           "       %s delete <num>      - delete article, if you wrote it\n\r"
                           "       %s skip              - skip to next thread\n\r"
                           "       %s catchup           - mark this thread as read\n\r"
                           "       %s uncatchup         - mark this thread as unread\n\r"
                           "       %s compose <subject> - create new thread\n\r"
                           "       %s reply             - reply to current thread\n\r"
                           "       %s + <line>          - add a line to your current message\n\r"
                           "       %s -                 - remove a line from your current message\n\r"
                           "       %s show              - show the news message you are composing\n\r"
                           "       %s post              - finish writing a news message\n\r"
                           "       %s clear             - abort writing a news message\n\r",
                           news_name, news_name, news_name, news_name, news_name, news_name, news_name, news_name,
                           news_name, news_name, news_name, news_name, news_name, news_name, news_name, news_name);
        if (IS_IMMORTAL(ch))
            buffer_addline_fmt(buffer, "       %s flags <flags>     - set/clear flags on current thread\n\r",
                               news_name);
        if (get_trust(ch) >= SUPREME) {
            buffer_addline_fmt(buffer, "       %s expiry <num>      - set expiry time to <num> days, or 'never'\n\r",
                               news_name);
            buffer_addline_fmt(buffer,
                               "       %s from <string>     - change the 'from' field of a message you are writing\n\r",
                               news_name);
        }

        buffer_send(buffer, ch);
    }
}

bool has_read_before(CHAR_DATA *ch, int mes_id) {
    MES_ID *m;
    if (IS_NPC(ch))
        return true;
    m = ch->mes_hash[mes_id % MES_HASH];
    for (; m; m = m->next)
        if (m->id == mes_id)
            return true;
    return false;
}

MES_ID *free_mes = nullptr;

void mark_as_read(CHAR_DATA *ch, int mes_id) {
    MES_ID *m;
    if (has_read_before(ch, mes_id))
        return;
    if (free_mes) {
        m = free_mes;
        free_mes = free_mes->next;
    } else {
        m = (MES_ID *)alloc_perm(sizeof(MES_ID));
    }
    m->id = mes_id;
    m->next = ch->mes_hash[mes_id % MES_HASH];
    ch->mes_hash[mes_id % MES_HASH] = m;
}

void mark_as_unread(CHAR_DATA *ch, int mes_id) {
    MES_ID *m;
    if (!has_read_before(ch, mes_id))
        return;
    m = ch->mes_hash[mes_id % MES_HASH];
    if (m->id == mes_id) {
        /* Special case */
        ch->mes_hash[mes_id % MES_HASH] = ch->mes_hash[mes_id % MES_HASH]->next;
        m->next = free_mes;
        free_mes = m;
    } else {
        for (; (m->next->id != mes_id) && m->next; m = m->next)
            ;
        if (m->next) {
            m->next = m->next->next;
            m = m->next;
            m->next = free_mes;
            free_mes = m;
        }
    }
}

bool article_exists(int mes_id) {
    THREAD *t;
    ARTICLE *a;
    for (t = thread_head; t; t = t->next)
        for (a = t->articles; a; a = a->next)
            if (a->msg_id == mes_id)
                return true;
    return false;
}

/* Give the user some info on logging in */
void news_info(CHAR_DATA *ch) {
    THREAD *t;
    ARTICLE *a;
    int numthreads = 0, unreadthreads = 0, unreadarticles = 0, numarticles = 0;
    bool flag;
    for (t = thread_head; t; t = t->next) {
        if (!can_read_thread(ch, t))
            continue;
        numthreads++;
        flag = false;
        for (a = t->articles; a; a = a->next) {
            numarticles++;
            if (!has_read_before(ch, a->msg_id)) {
                unreadarticles++;
                flag = true;
            }
        }
        if (flag)
            unreadthreads++;
    }
    if (unreadthreads || unreadarticles) {
        char buf[MAX_STRING_LENGTH];
        snprintf(buf, sizeof(buf),
                 "You have %d unread article%s in %d thread%s.\n\rThe news base has %d article%s in %d thread%s.\n\r",
                 unreadarticles, (unreadarticles == 1) ? "" : "s", unreadthreads, (unreadthreads == 1) ? "" : "s",
                 numarticles, (numarticles == 1) ? "" : "s", numthreads, (numthreads == 1) ? "" : "s");
        send_to_char(buf, ch);
    }
}

void move_to_next_unread(CHAR_DATA *ch) {
    ARTICLE *a, *start_a;
    THREAD *t;
    bool flag;
    if (IS_NPC(ch))
        return;
    t = ch->thread;
    if (t == nullptr)
        return;
    start_a = ch->article;
    if (start_a == nullptr) { // End of this thread?
        t = t->next; // move on
    }
    if (t == nullptr) {
        ch->thread = nullptr;
        return;
    }

    for (; t; t = t->next) {
        if (!can_read_thread(ch, t)) { // Can we read this thread at all?
            start_a = nullptr;
            ch->articlenum = 1; // If not - reset start_a and skip to next thread
            continue;
        }
        for (a = (start_a) ? (start_a) : t->articles; a; a = a->next) {
            if (!has_read_before(ch, a->msg_id)) {
                ARTICLE *b = t->articles;
                ch->articlenum = 1;
                ch->thread = t;
                ch->article = a;
                for (; b && b != a; b = b->next)
                    ch->articlenum++;
                return;
            }
        }
        start_a = nullptr;
        ch->articlenum = 1;
    }
    /* Try again from the beginning, unless we really have finished all messages */
    ch->thread = nullptr;
    flag = false;
    for (t = thread_head; t; t = t->next)
        if (num_unread(ch, t) != 0) {
            flag = true;
            break;
        }
    if (flag) {
        ch->thread = thread_head;
        ch->article = ch->thread->articles;
        move_to_next_unread(ch); /* Loop back as we know there's something there */
    }
}

int num_unread(CHAR_DATA *ch, THREAD *t) {
    int num = 0;
    ARTICLE *a = t->articles;
    if (!can_read_thread(ch, t))
        return 0;
    for (; a; a = a->next)
        if (!has_read_before(ch, a->msg_id))
            num++;
    return num;
}

bool can_read_thread(CHAR_DATA *ch, THREAD *t) {
    if (ch == nullptr) {
        bug("nullptr ch in can_read_thread");
        return false;
    }
    if (IS_NPC(ch))
        return false;
    if (IS_SET(t->flags, THREAD_DELETED) && (get_trust(ch) < SUPREME))
        return false;
    if (IS_SET(t->flags, THREAD_IMMORTAL_ONLY) && !IS_IMMORTAL(ch))
        return false;
    if (IS_SET(t->flags, THREAD_IMPLEMENTOR_ONLY) && (get_trust(ch) < IMPLEMENTOR))
        return false;
    if (IS_SET(t->flags, THREAD_CLAN_ONLY) && !IS_IMMORTAL(ch)
        && !(ch->pcdata->pcclan && (is_name(t->subject, ch->pcdata->pcclan->clan->name))))
        return false;
    return true;
}

bool can_write_thread(CHAR_DATA *ch, THREAD *t) {
    if (IS_NPC(ch))
        return false;
    if (IS_SET(t->flags, THREAD_DELETED))
        return false;
    if (IS_SET(t->flags, THREAD_READ_ONLY) && !IS_IMMORTAL(ch))
        return false;
    return true;
}
