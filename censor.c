/*
 * $Log: censor.c,v $
 * Revision 1.1.1.1  1998/11/01 17:24:08  siegl
 * COW 3.0 initial revision
 * */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>

#define MAX_CURSE_LEN 10
#define REPLACE_STR_LEN 20
#define NUM_CURSES 8
#define NOT_FOUND -1

typedef struct
  {
    char    badWord[MAX_CURSE_LEN];
    int     beginningOnly;
    int     skip[128];
  }
profanity;

/* if beginningOnly is true censor only looks for the curse at the beginning
 * of words.  Currently only set for "twat" so "saltwater", etc aren't caught.
 * Grepped for the other strings in /usr/dict/words and came up with a blank,
 * so beginningOnly is left off for them. */

static profanity curseWords[NUM_CURSES] =
{
  {"damn", 0},
  {"fuck", 0},
  {"bastard", 0},
  {"shit", 0},
  {"bitch", 0},
  {"twat", 1},
  {"cunt", 0},
  {"dammit", 0}
};

static char replacementChars[REPLACE_STR_LEN + 1] = "@#$%^&*%#$%@#$*&@%$%";

/* + 1 for trailing null */

void    initSkipArray(char *word, int *skip)
{
  int     i, wordLen = strlen(word);

  for (i = 0; i < 128; i++)
    skip[i] = wordLen;
  for (i = 0; i < wordLen; i++)
    {
      skip[word[i]] = wordLen - i - 1;
      if (isascii(toupper(word[i])))
	skip[toupper(word[i])] = skip[word[i]];
    }
}

void    initCensoring()
{
  int     i;

  for (i = 0; i < NUM_CURSES; i++)
    initSkipArray(curseWords[i].badWord, curseWords[i].skip);
}

static int search(char *word, char *text, int *skip)
{
  int     i, j, wordLen = strlen(word), textLen = strlen(text);

  for (i = j = wordLen - 1; j >= 0; i--, j--)
    while (tolower(text[i]) != word[j])
      {
	int     t = skip[text[i]];

	i += (wordLen - j > t) ? wordLen - j : t;
	if (i >= textLen)
	  return NOT_FOUND;
	j = wordLen - 1;
      }
  return i + 1;
}

char   *censor(char *text)
{
  int     i, j, t = strlen(text) - 1;
  char   *str = text;

  for (i = 0; i < NUM_CURSES; i++)
    {
      text = str;
      while ((j = search(curseWords[i].badWord, text, curseWords[i].skip))
	     != NOT_FOUND)
	{
	  int     k, l, wordBegin, wordEnd;

	  for (wordBegin = j; wordBegin > 0; wordBegin--)
	    if (!isalpha(text[wordBegin - 1]))
	      break;
	  for (wordEnd = j + strlen(curseWords[i].badWord) - 1; wordEnd < t; wordEnd++)
	    if (!isalpha(text[wordEnd + 1]))
	      break;
	  if (!curseWords[i].beginningOnly || wordBegin == j)
	    for (k = wordBegin, l = 0; k <= wordEnd; k++)
	      {
		text[k] = replacementChars[l];
		if (++l >= REPLACE_STR_LEN)
		  l = 0;			 /* make sure we don't go *
						  * past bound of *
						  * replacementChars */
	      }
	  text += wordEnd;			 /* so we don't go into *
						  * infinite loop if *
						  * beginningOnly */
	}
    }
  return str;
}

#undef MAX_CURSE_LEN
#undef REPLACE_STR_LEN
#undef NUM_CURSES
#undef NOT_FOUND
