#include "../src/board.h"


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
  const char * fens [] = 
  {
    "8/P7/8/8/8/8/8/k6K w - - 0 1",
    "8/Q7/8/q7/8/8/8/k6K b - - 0 1",
    "k7/4np2/8/7n/8/8/PP6/R3K2R w KQ - 0 30",
    "r1bqkbnr/pppp1ppp/2n5/4p3/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - 0 2",
    "rnbqkbnr/1pp1pppp/8/p2pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 3",
    "8/k7/8/K7/8/8/8/8 b - - 0 1",
    "k7/1Q6/K7/8/8/8/8/8 b - - 0 1",
    "rn1qk1n1/pBp1ppbr/8/4P1p1/3p3p/NP5Q/P1PP1PPP/1RB1K1NR w Kq - 1 2",
    "rnB1k1n1/p1p1pp1r/4Q2b/1q2P1p1/7p/NP1p1N2/PBPP1PPP/1R1K3R w q - 2 6",
    /* wrong FEN */
    "p7/1Q6/K7/8/8/8/8/8 b - - 0 1", //no king
    "8/Q7/8/w7/8/8/8/k6K b - - 0 1", //unknown pice
    "k7/3np2/8/7n/8/8/PP6/R3K2R w KQ - 0 30", // < 8 pieces in a row
    "k7/4np2/8/7n/8/8/PP6/1R2K2R w KQ - 0 30", // Rook moved & castle shouldn't be available
    "r1bqkbnr/pppp1ppp/2n5/4p3/4P3/8/PPPP1PPP/RNBQKBNR b KqQk - 0 2", //wrong castle order
    "k7/4np2/8/8n/8/8/PP6/R3K2R w KQ - 0 30"  // > 8 pieces in a row
  };
  char fen [FEN_MAXSIZE];

  _Board board, * b = & board;
  for (int i=0; i<sizeof (fens) / sizeof (fens [1]); ++i)
  {
    fprintf(stdout, "%s", fens [i]);
    if (BoardSetFromFEN (b, fens [i]) == 0) {
      fprintf (stdout, "\nwrong fen\n\n");
      continue;
    }
    BoardFEN (b, fen);
    fprintf(stdout, "\n%s", fen);
    BoardPrint(b);
  }

  return 0;
}
