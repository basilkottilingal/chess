#define _POSIX_C_SOURCE 199309L

#include <time.h>
#define _CHESS_DEBUG_
#include "../src/tree.h"


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
  //NULL;
  "r1bk1r2/pppp1Qpp/4p2q/4P1N1/7P/2NP2P1/PPP5/2KR1Bn1 w - - 1 32";
  //"r1bk3r/pppp1Qpp/4p2q/4P1N1/7P/2NP2P1/PPP5/2KR1Bn1 b - - 2 32";
  //"rnb1kbnr/pppp1ppp/5q2/4p3/4P2P/6P1/PPPP1P2/RNBQKBNR b KQkq e3 0 5";
  //"rnb1kb1r/pp2pp1p/2pp2p1/q1n5/2Q2P2/3P3N/PPPB2PP/RN2KB1R b KQkq - 3 15";
  //"rnbqkbnr/pppp1ppp/4p3/8/6Q1/4P3/PPPP1PPP/RNB1KBNR b KQkq - 1 3";
  //"r1b3Q1/ppppk2N/2nnpp2/3q4/8/2P5/PP2BPPP/RNB2RK1 b - - 1 38";
  //"3R4/ppk2ppp/8/1bp2p2/8/P1r5/1K1N4/3R4 b - - 1 67";
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

  _Board * b = BoardStack;
  assert (BoardRoot (fen));
  unsigned depthmax = 3u;
  _Move * best = BoardProbe (depthmax);

  BoardPrint (b);
  for(int d = depthmax; d; d--)
  {
    BoardRoll (b++, & bestMoves [depthmax][d]);
    BoardPrint (b);

    if (!GAME_CONTINUES (b->status))
      break;
  }

  BoardStatusPrint (b);

  return 0;
}
