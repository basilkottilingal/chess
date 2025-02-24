/**TODO:
misalignment of struct might cause problem
wile memcpy, memcmp
*/

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include <math.h>
#include <stdint.h>

/* Used to encode flags. 
.. Used for small unsigned int [0,256)
*/
typedef uint8_t Flag;

/* ---------------------------------------------------------
------------------------------------------------------------
  An "Array" is used to store dynamic data. You can create, 
  .. reallocate, shrink and free the "Array" ..
  .. without any memory mismanagement. 
  NOTE: realloc() function used inside array_append(),
  .. can slow the program, when you append large data ..
  .. multiple times. WHY? : Because realloc search for .. 
  .. continous chunk of memory that can fit the new ..
  .. array data which might be very large. In this case, ..
  .. impelement memory pooling of fixed sized blocks ..
  .. Refer to:
      http://www.boost.org/doc/libs/1_55_0/libs/pool/doc/html/boost_pool/pool/pooling.html
------------------------------------------------------------
--------------------------------------------------------- */
typedef struct {
  void * p;
  size_t max, len;
} Array;

Array * array_new()
{
  Array * a = (Array *)malloc (sizeof(Array));
  a->p = NULL;
  a->max = a->len = 0;
  return a;
}

void array_free (Array * a)
{
  free (a->p);
  free (a);
}

void array_append (Array * a, void * elem, size_t size)
{
  if (a->len + size >= a->max) {
    a->max += size >4096 ? size : 4096;
    a->p = realloc (a->p, a->max);
  }
  memcpy (((char *)a->p) + a->len, elem, size);
  a->len += size;
}

void array_shrink (Array * a)
{
  if(a->len < a->max) {
    a->p = realloc (a->p, a->len);
    a->max = a->len;
  }
}

/*
Error Handling
*/

Array GameErrorBuffer = {.p = NULL,.len = 0,.max = 0};

void GameError(char * err) {
  Array * b = &GameErrorBuffer;  
  Flag n = 0;
  char * c = err, end[2] = {'\n','\0'};
  /* Get the count of char in err*/
  while(n<UINT8_MAX-1 && *c++)
    n++;
  if(!n)
    return;
  if(b->len)
    b->len--;
  /* Copy error to buffer. Excluding the '\0' */
  array_append(b, err, n);
  array_append(b, end, 2);
}

void GameErrorFree() {
  Array * b = &GameErrorBuffer;  
  b->len = b->max = 0;
  if(b->p)
    free(b->p);
  b->p = NULL;
}

void GameErrorPrint(){
  char * err = (char *) GameErrorBuffer.p;  
  if(err) {
    fprintf(stderr, "\n=======Error======\n%s", err);
    fflush(stderr);
  }
  GameErrorFree(); 
}
