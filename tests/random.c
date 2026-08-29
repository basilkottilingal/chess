#define _POSIX_C_SOURCE 199309L

#include <time.h>
#include "../src/move.h"

struct timespec ts =
{
  .tv_sec = 0,
  .tv_nsec = 600000000
};
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
  _Board * b =
  //BoardSetFromFEN (NULL);
  //BoardSetFromFEN ("8/P7/8/8/8/8/8/k6K w - - 0 1");
  //BoardSetFromFEN ("8/Q7/8/q7/8/8/8/k6K b - - 0 1");
  //BoardSetFromFEN ("k7/4np2/8/7n/8/8/PP6/R3K2R w KQ - 0 30");
  //BoardSetFromFEN ("r1bqkbnr/pppp1ppp/2n5/4p3/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - 0 2");
  //BoardSetFromFEN ("rnbqkbnr/1pp1pppp/8/p2pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 3");
  //BoardSetFromFEN ("8/k7/8/K7/8/8/8/8 b - - 0 1");
  //BoardSetFromFEN ("k7/1Q6/K7/8/8/8/8/8 b - - 0 1");
  //BoardSetFromFEN ("rn1qk1n1/pBp1ppbr/8/4P1p1/3p3p/NP5Q/P1PP1PPP/1RB1K1NR w Kq - 1 2");
  //BoardSetFromFEN ( "rnB1k1n1/p1p1pp1r/4Q2b/1q2P1p1/7p/NP1p1N2/PBPP1PPP/1R1K3R w q - 2 6");
  //BoardSetFromFEN ("8/8/8/8/p3k3/4p3/5bK1/8 w - - 0 145");
  BoardSetFromFEN ("r2q2n1/2p1p1kr/3p1p1b/p3B2p/p1PP1Pb1/4KR1P/RP2P3/nN3BN1 w - - 2 22");
  //BoardSetFromFEN ("3r1n2/8/1b2k3/6P1/2p3K1/1p6/4p1B1/8 b - - 2 120");
  //BoardSetFromFEN ("5n2/4r3/1b2k1P1/8/2p3K1/1p6/4p1B1/8 w - - 1 122");
  //BoardSetFromFEN ("B2r1n2/8/1b2k3/6P1/2p3K1/1p1n4/8/8 w - - 2 122");
  //BoardSetFromFEN ("8/k6P/8/K7/8/8/6p1/8 b - - 0 1");
  //BoardSetFromFEN ("8/r1k4P/R7/K7/8/8/6p1/8 w - - 0 1");
  assert(b != NULL);

  /* starting board */
  WAIT_CLEAR ();
  BoardPrint (b);

  uint8_t loc = 0;

  /*
  .. Doesn't use real randomness inorder to maintain reproducability
  */
  int randomvec [] = {
    731, 42, 918, 305, 667, 184, 559, 823,
    96, 477, 250, 704, 13, 891, 365, 628,
    542, 77, 936, 411, 159, 782, 298, 645,
    870, 224, 503, 61, 719, 347, 982, 136,
    455, 809, 273, 590, 34, 764, 617, 201,
    948, 386, 729, 112, 573, 845, 68, 439,
    314, 957, 166, 681, 520, 93, 756, 281,
    607, 49, 894, 332, 718, 155, 463, 829,
    237, 671, 14, 544, 902, 388, 126, 753,
    496, 805, 263, 578, 41, 937, 350, 692,
    183, 821, 469, 108, 635, 274, 716, 57,
    880, 321, 601, 147, 964, 403, 759, 216,
    528, 73, 847, 291, 663, 135, 914, 356,
    780, 24, 509, 872, 187, 643, 315, 951,
    430, 99, 688, 246, 825, 572, 39, 934,
    163, 710, 284, 556, 118, 866, 377, 624
  };

  while ( BoardAllMoves (b) == GAME_CONTINUE )
  {
    _Move * moves = MOVES_AT (b);
    loc = (loc + randomvec [ b->totalMoves ]) % b->totalMoves;
    while (moves [loc].flags == MOVE_ILLEGAL)
      loc = ++loc % b->totalMoves; 

    //BoardNextLevel (&b, moves + loc);
    _Move * m = moves + loc;
    BoardMove (b, m);
    if (m->flags & (MOVE_CAPTURE | MOVE_ENP_CAPTURE))
      npieces--;
    fullclock++;
    color = !color;
    b [0] = b [1];
    BoardAllMoves (b);

    //WAIT_CLEAR ();
    BoardPrint (b);
  }

  /* game over */
  BoardStatusPrint (b);

  char fen [100];
  BoardFEN (b, fen);
  printf ("\n\n %s\n", fen);

  return 0;
}
