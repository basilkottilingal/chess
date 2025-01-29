#include "../src/move.h"


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
  _Board * b = Board(NULL);
  //BoardSetFromFEN(b,NULL);
  //BoardSetFromFEN(b,"8/P7/8/8/8/8/8/k6K w - - 0 1");
  //BoardSetFromFEN(b,"8/Q7/8/q7/8/8/8/k6K b - - 0 1");
  //BoardSetFromFEN(b,"k7/4np2/8/7n/8/8/PP6/R3K2R w KQ - 0 30");
  //BoardSetFromFEN(b,"r1bqkbnr/pppp1ppp/2n5/4p3/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - 0 2");
  BoardSetFromFEN(b,"rnbqkbnr/1pp1pppp/8/p2pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 3");
  //BoardSetFromFEN(b,"8/k7/8/K7/8/8/8/8 b - - 0 1");
  //BoardSetFromFEN(b,"k7/1Q6/K7/8/8/8/8/8 b - - 0 1");
  //BoardSetFromFEN(b,"rn1qk1n1/pBp1ppbr/8/4P1p1/3p3p/NP5Q/P1PP1PPP/1RB1K1NR w Kq - 1 2");
  //BoardSetFromFEN(b, "rnB1k1n1/p1p1pp1r/4Q2b/1q2P1p1/7p/NP1p1N2/PBPP1PPP/1R1K3R w q - 2 6");

  //GamePrintBoard(g, 0);
  //_GameSquare * from = &(g->board[7][1]);
  //GameMovesFrom(from, KNIGHT_MOVES, 8, 1, g->moves);
  //GameAllMoves(g);
  BoardPrint(b);

  Array moves = {.p = NULL, .len = 0, .max = 0};
  Flag status = BoardAllMoves(b, &moves);
  if(status == GAME_CONTINUE) {
    _BoardMove * move = moves.p;
    for(int i=0; i<(int) moves.len/sizeof(_BoardMove); ++i, ++move){
{
      clock_t start_time = clock();
      clock_t wait_time = 0.8*CLOCKS_PER_SEC ; //sleep time 
      while (clock() - start_time < wait_time) {};
      printf("\033[2J");       // Clear the screen
      printf("\033[1;1H");     //Cursor on the left top left
}
      BoardPrint(b);
      BoardMove(b, move);
{
      clock_t start_time = clock();
      clock_t wait_time = 0.8*CLOCKS_PER_SEC ; //sleep time 
      while (clock() - start_time < wait_time) {};
      printf("\033[2J");       // Clear the screen
      printf("\033[1;1H");     //Cursor on the left top left
}
      BoardPrint(b);
      BoardUnmove(b, move);
      
    }  
  }

  //unsigned int status = Game(g);
  //GameError(status);

  BoardDestroy(b);

  return 0;
}
