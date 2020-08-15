#include "utils.hpp"
#include <cctype>
#include <cstring>

namespace chat {

// Gets a word out of a string.
int get_word(const char *&input, char *outword, char &outother) {
    outword[0] = 0;
    char *outword0 = outword;
    int curchar;
    curchar = input[0];

    while (isalnum(curchar) || curchar == '_') {
        *outword = curchar;
        outword++;
        input++;
        curchar = *input;
    }
    if (*input != '\0')
        input++;
    *outword = '\0';
    outother = curchar;
    if (curchar == 0 && outword0[0] == 0)
        return 0;
    return 1;
}

char *trim(char str[]) {
    int i, a, ln;
    for (ln = strlen(str); (ln > 0) && (str[ln - 1] <= ' '); ln--)
        ;
    str[ln] = 0;
    for (i = 0; (i < ln) && (str[i] <= ' '); i++)
        ;
    if (i > 0)
        for (ln -= i, a = 0; a <= ln; a++)
            str[a] = str[a + i];
    return str;
}

}