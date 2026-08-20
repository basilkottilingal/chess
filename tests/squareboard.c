#include "../src/board.h"

int main()
{
  _Board * b = BoardSetFromFEN ("");
  BoardPrint (b);
  for (int i=0; i<8; ++i)
  {
    printf ("\n");
    for (int j=0; j<8; ++j)
    {
      printf (" %s", RANKFILE [8*i+j]);
    }
  }

  printf ("\n");
  for (int i=0; i<8; ++i)
  {
    printf ("\n");
    for (int j=0; j<8; ++j)
    {
      printf (" %c", ASCII [PIECES [BOARD [i+START][j+START]]]);
    }
  }
  printf ("\n");
  for (int i=0; i<8; ++i)
  {
    printf ("\n");
    for (int j=0; j<8; ++j)
    {
      printf (" %2d", BOARD [i+START][j+START]);
    }
  }
  return 0;
}
