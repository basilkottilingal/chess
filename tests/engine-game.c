#define TREE_MAX_DEPTH 3
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

Flag ClientMakesAMove(_Game * g) {
  int nmoves = (g->moves->len) / sizeof(_BoardMove);
  return floor (((double) nmoves)*rand()/RAND_MAX);
}

Flag TreeNodePrint(_Tree * node){
  clock_t start_time = clock();
  clock_t wait_time = 0.01*CLOCKS_PER_SEC ; //sleep time 
  while (clock() - start_time < wait_time) {};
  printf("\033[2J");   // Clear the screen
  printf("\033[1;1H");     //Cursor on the left top left
 
  fprintf(stdout, "\nd%d, max%d", 
    node->depth, node->depthmax);
  BoardPrint(&node->board);
  return 1;
}

int main(){
  //BoardSetFromFEN(b,NULL);
  //BoardSetFromFEN(b,"8/P7/8/8/8/8/8/k6K w - - 0 1");
  //BoardSetFromFEN(b,"8/Q7/8/q7/8/8/8/k6K b - - 0 1");
  //BoardSetFromFEN(b,"k7/4np2/8/7n/8/8/PP6/R3K2R w KQ - 0 30");
  //BoardSetFromFEN(b,"r1bqkbnr/pppp1ppp/2n5/4p3/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - 0 2");
  //BoardSetFromFEN(b,"rnbqkbnr/1pp1pppp/8/p2pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 3");
  //BoardSetFromFEN(b,"8/k7/8/K7/8/8/8/8 b - - 0 1");
  //BoardSetFromFEN(b,"k7/1Q6/K7/8/8/8/8/8 b - - 0 1");
  //BoardSetFromFEN(b,"rn1qk1n1/pBp1ppbr/8/4P1p1/3p3p/NP5Q/P1PP1PPP/1RB1K1NR w Kq - 1 2");
  char fen[] = "rnB1k1n1/p1p1pp1r/4Q2b/1q2P1p1/7p/NP1p1N2/PBPP1PPP/1R1K3R w q - 2 6";
  //BoardSetFromFEN(b,"8/8/8/8/p3k3/4p3/5bK1/8 w - - 0 145");
  //BoardSetFromFEN(b,"r2q2n1/2p1p1kr/3p1p1b/p3B2p/p1PP1Pb1/4KR1P/RP2P3/nN3BN1 w - - 2 22");
  //BoardSetFromFEN(b,"3r1n2/8/1b2k3/6P1/2p3K1/1p6/4p1B1/8 b - - 2 120");
  //BoardSetFromFEN(b,"8/7p/8/7P/8/8/8/5k1K b - - 0 1");
  //BoardSetFromFEN(b,"8/6p1/7P/8/8/8/8/5k1K w - - 0 1");
  //BoardSetFromFEN(b,"8/6p1/7P/8/8/8/8/5k1K w - - 0 1");
  //_Game * g = GameNew("8/6p1/7P/8/8/8/8/5k1K w - - 0 1");
  //_Game * g = GameNew("8/6p1/6p1/6PP/8/8/8/5k1K w - - 0 1");

  //char * fen = NULL;

  //Server starts a game with an engine who plays for WHITE
  _Game * server = GameNew(fen);
  GameEngineStart(server, WHITE);

  //Client represents BLACK and makes a random move.

  srand(time(0));
  while(GameStatus(server) == GAME_CONTINUE) {
  
    if(server->board->color == WHITE) {
      //White engine plays
      _BoardMove * move = GameEngineMove(server);
      if(!move) {
        fprintf(stderr, 
          "Error : Engine couldn't evaluate a move");
        break;
      }
      
      //TreeEachNode(server->engine->tree, 1, TreeNodePrint);
      //Communicate to client;
    }
    else {
      Flag imove = ClientMakesAMove(server);
      //_BoardMove * move = &server->tree->children[imove]->move;
      _BoardMove ClientMove;
      memcpy(&ClientMove, (_BoardMove *) server->moves->p
        + imove, sizeof(_BoardMove));

      //tell it to the sever engine
      Flag success = GamePlayerMove(server, &ClientMove);
      if(!success) {
        fprintf(stderr, 
          "Error : Engine couldn't find the move from Client");
        break;
      }
    }

    GamePrintBoard(server, 100);
  }
  BoardStatusPrint(server->board);

  GameDestroy(server);

  return 0;
}
