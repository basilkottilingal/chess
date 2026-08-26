#define _POSIX_C_SOURCE 199309L

#include <time.h>
#include "../src/move.h"

struct timespec ts =
{
  .tv_sec = 0,
  .tv_nsec = 400000000
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

int main(){
  _Board * b =
  BoardSetFromFEN (NULL);
  //BoardSetFromFEN ("rnbqkbnr/pppppppp/8/8/P7/8/1PPPPPPP/RNBQKBNR b KQkq - 1 2");
  //BoardSetFromFEN ("8/P7/8/8/8/8/8/k6K w - - 0 1");
  //BoardSetFromFEN ("8/Q7/8/q7/8/8/8/k6K b - - 0 1");
  //BoardSetFromFEN ("k7/4np2/8/7n/8/8/PP6/R3K2R w KQ - 0 30");
  //BoardSetFromFEN ("k7/4np2/8/7n/8/8/PP5r/R3K2R w KQ - 0 30");
  //BoardSetFromFEN ("k7/4np2/8/7n/8/8/PP4r1/R3K2R w KQ - 0 30");
  //BoardSetFromFEN ("r1bqkbnr/pppp1ppp/2n5/4p3/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - 0 2");
  //BoardSetFromFEN ("rnbqkbnr/1pp1pppp/8/p2pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 3");
  //BoardSetFromFEN ("8/k7/8/K7/8/8/8/8 b - - 0 1");
  //BoardSetFromFEN ("k7/1Q6/K7/8/8/8/8/8 b - - 0 1");
  //BoardSetFromFEN ("rn1qk1n1/pBp1ppbr/8/4P1p1/3p3p/NP5Q/P1PP1PPP/1RB1K1NR w Kq - 1 2");
  //BoardSetFromFEN ( "rnB1k1n1/p1p1pp1r/4Q2b/1q2P1p1/7p/NP1p1N2/PBPP1PPP/1R1K3R w q - 2 6");
  //BoardSetFromFEN ("8/8/8/8/p3k3/4p3/5bK1/8 w - - 0 145");
  //BoardSetFromFEN ("r2q2n1/2p1p1kr/3p1p1b/p3B2p/p1PP1Pb1/4KR1P/RP2P3/nN3BN1 w - - 2 22");
  //BoardSetFromFEN ("3r1n2/8/1b2k3/6P1/2p3K1/1p6/4p1B1/8 b - - 2 120");
  //BoardSetFromFEN ("5n2/4r3/1b2k1P1/8/2p3K1/1p6/4p1B1/8 w - - 1 122");
  //BoardSetFromFEN ("B2r1n2/8/1b2k3/6P1/2p3K1/1p1n4/8/8 w - - 2 122");
  //BoardSetFromFEN ("8/k6P/8/K7/8/8/6p1/8 b - - 0 1");
  //BoardSetFromFEN ("8/r1k4P/R7/K7/8/8/6p1/8 w - - 0 1");
  //BoardSetFromFEN ("1qr5/3k2rp/n7/1K6/8/5p2/2qbr3/1n2n2b w - - 22 325");
  assert(b != NULL);

  WAIT_CLEAR ();
  BoardPrint (b);

  if ( BoardAllMoves (b) != GAME_CONTINUE )
  {
    BoardStatusPrint (b);
    return 0;
  }

  fprintf (stdout, "%d", b->totalMoves);
  _Move * move = MOVES_AT (b);
  for (uint8_t i = 0; i < b->totalMoves; ++i, ++move)
  {
    if (move->flags == MOVE_ILLEGAL)
      continue;

    WAIT_CLEAR ();
    BoardPrint (b);
    BoardMove (b, move);

    WAIT_CLEAR ();
    BoardPrint (b);
    BoardUnmove (move);
  }

  return 0;
}
