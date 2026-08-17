#define _POSIX_C_SOURCE 199309L

#include <time.h>
#include "../src/tree.h"
#include <inttypes.h>


#define WAIT_CLEAR() do \
  { \
    nanosleep (&ts, NULL);\
    printf ("\033[2J");\
    printf ("\033[1;1H");\
  } while (0)

//Run this test script using
//$ gcc -Winline -o del test-move.c -lm&& ./del

/*Sample FEN's for verifying
1) Fool's Mate (Black Checkmates White)
r1bqkbnr/pppp1ppp/2n5/4p3/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - 0 2
2) Promotable white pawn
8/P7/8/8/8/8/8/k6K w - - 0 1
3) En passante 
rnbqkbnr/ppp1pppp/8/3pP3/8/8/PPPP1PPP/RNBQKBNR b KQkq d6 0 2
rnbqkbnr/1pp1pppp/8/p2pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 3
4) To see if filtering invalid moves works fine
8/Q7/8/q7/8/8/8/k6K b - - 0 1
5)Castling available for 'w'
k7/4np2/8/7n/8/8/PP6/R3K2R w KQ - 0 30
6)Game already over
8/k7/8/K7/8/8/8/8 b - - 0 1
7)Bug
rn1qk1n1/pBp1ppbr/8/4P1p1/3p3p/NP5Q/P1PP1PPP/1RB1K1NR w Kq - 1 2
rn2k1n1/pBp1pp1r/4Q2b/1q2P1p1/7p/NP1p1N2/PBPP1PPP/1R1K3R w q - 2 6
*/

int main()
{
  const char * fen =
  NULL;
  //"8/P7/8/8/8/8/8/k6K w - - 0 1";
  //"8/Q7/8/q7/8/8/8/k6K b - - 0 1";
  //"k7/4np2/8/7n/8/8/PP6/R3K2R w KQ - 0 30";
  //"r1bqkbnr/pppp1ppp/2n5/4p3/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - 0 2";
  //"rnbqkbnr/1pp1pppp/8/p2pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 3";
  //"8/k7/8/K7/8/8/8/8 b - - 0 1";
  //"k7/1Q6/K7/8/8/8/8/8 b - - 0 1";
  //"rn1qk1n1/pBp1ppbr/8/4P1p1/3p3p/NP5Q/P1PP1PPP/1RB1K1NR w Kq - 1 2";
  //"8/8/8/8/p3k3/4p3/5bK1/8 w - - 0 145";
  //"r2q2n1/2p1p1kr/3p1p1b/p3B2p/p1PP1Pb1/4KR1P/RP2P3/nN3BN1 w - - 2 22";
  //"3r1n2/8/1b2k3/6P1/2p3K1/1p6/4p1B1/8 b - - 2 120";
  //"5n2/4r3/1b2k1P1/8/2p3K1/1p6/4p1B1/8 w - - 1 122";
  //"B2r1n2/8/1b2k3/6P1/2p3K1/1p1n4/8/8 w - - 2 122";
  //"8/k6P/8/K7/8/8/6p1/8 b - - 0 1";
  //"8/r1k4P/R7/K7/8/8/6p1/8 w - - 0 1";

  /* nodes at ech level. NOTE : only for default FEN */
  static const long leaves [] = {
  	20,
  	400,
  	8902,
  	197281,
  	4865609,
  	119060324,
  	3195901860,
  	84998978956
  };

  _Board * b = BoardRoot (fen);
  #define DEPTHMAX 5
  #define LEVEL(d) (DEPTHMAX - d)
  int depth = DEPTHMAX;
  int traversed [DEPTHMAX] = {0};

  
  clock_t start, end;
  start = clock();

  assert (depth);
  /* depth first search */
  do {
    do
    {
      if (!BoardNextLevel (&b))
        break;

      /* skip */
      _Entry * e = HashLoc (b->zobrist);
      if (e->hash == b->zobrist /*&& e->depth > REQD */) {
        //printf("{%" PRId64 "}\n", b->zobrist);
        break;
      }

      traversed [LEVEL(depth)] ++;
    } while (--depth);

    /* new entry to TT or update deth */
    _Entry * e = HashLoc (b->zobrist);
    e->depth = depth;
    e->hash = b->zobrist;

    if (++depth > DEPTHMAX)
      break;

    BoardPrevLevel (&b);
  } while (1);

  end = clock();
  double time_used = ((double) (end - start)) / CLOCKS_PER_SEC;

  int nodes = 1; /* root node */
  fprintf (stdout, "\ntraversed at each level");
  for (int i=0; i<DEPTHMAX; ++i) {
    fprintf (stdout, "\n  %10d  (compare with %10ld)", traversed [i], leaves [i]);
    nodes += traversed [i];
  }
  fprintf (stdout, "\ntime elapsed %g. nodes traversed %d\n", time_used, nodes);

  BoardStatusPrint (b);
  char lastfen [100];
  BoardFEN (b, lastfen);
  printf ("\n\n %s\n", lastfen);

  return 0;
}
