#include "../src/game.h"

//Run this test script using
//$ gcc -Winline -o del test.c -lm&& ./del

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
*/

int main(){
  _Game * g = GameNew(NULL);
  //_Game * g = GameNew("8/P7/8/8/8/8/8/k6K w - - 0 1");
  //_Game * g = GameNew("8/Q7/8/q7/8/8/8/k6K b - - 0 1");
  //_Game * g = GameNew("k7/4np2/8/7n/8/8/PP6/R3K2R w KQ - 0 30");
  //_Game * g = GameNew("r1bqkbnr/pppp1ppp/2n5/4p3/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - 0 2");
  //_Game * g = GameNew("rnbqkbnr/1pp1pppp/8/p2pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 3");
  //_Game * g = GameNew("8/k7/8/K7/8/8/8/8 b - - 0 1");
  //_Game * g = GameNew("k7/1Q6/K7/8/8/8/8/8 b - - 0 1");

  //_GameSquare * from = &(g->board[7][1]);
  //GameMovesFrom(from, KNIGHT_MOVES, 8, 1, g->moves);

  //GameAllMoves(g);
   
  //GamePrintBoard(g, 1);

  //unsigned int status = Game(g);
  //GameError(status);
  Game(g);
  GameDestroy(g);

  return 0;
}
