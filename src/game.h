#include "move.h"

/* ---------------------------------------------------------
------------------------------------------------------------
  3 structs commonly used in this header file.
  a) unsigned char : A square of the chessboard
  b) _BoardMove : Info corresponding to a move from ..
    the current board location
  c) _Game : Store all the information corresponding to ..
    a chess game including board information, other ..
    informations like whose move, location of each kings,
    any active en-passante, are castling available, etc.
------------------------------------------------------------
--------------------------------------------------------- */

typedef struct {
  //Board and the board status metadata
  _Board * board;
  //Other Info
  char fen[FEN_MAXSIZE];
  Array * moves, * history;
}_Game;

/* ---------------------------------------------------------
------------------------------------------------------------
  Function to print the current board to stdout.
    GamePrintBoard(_Game * g, int persist);
  In the above function use "persist" as 1 to print ..
  .. the board on the left top of the screen with a time ..
  .. small delay (0.4 s);
------------------------------------------------------------
--------------------------------------------------------- */

void GamePrintBoard(_Game * g, unsigned int microSec) {
  
  //if persist. It clears the window
  if(microSec) {  
    microSec = microSec > 1000 ? 1000 : microSec;
    clock_t start_time = clock();
    clock_t wait_time = (0.001*microSec)*CLOCKS_PER_SEC ; //sleep time 
    while (clock() - start_time < wait_time) {};

    printf("\033[2J");       // Clear the screen
    printf("\033[1;1H");     //Cursor on the left top left
  } 
  fprintf(stdout, "\n%s", g->fen);
  BoardPrint(g->board);
}

Flag GameStatus(_Game * g) {
  return g->board->status;
}

/* ---------------------------------------------------------
------------------------------------------------------------
  The following function
    GameSetBoard(_Game * g, char * _fen); //.. 
  .. sets the game and board from the parsed FEN "_fen"
------------------------------------------------------------
--------------------------------------------------------- */
const char FEN_DEFAULT[] = 
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

void GameSetBoard(_Game * g, const char * _fen) {
  char * fen = _fen ? _fen : FEN_DEFAULT;
  g->fen[0] = '\0';
  for (int i=0; fen[i] != '\0'; ++i){
    if(i == FEN_MAXSIZE - 1){
      fprintf(stderr, "\nERROR: Very Long FEN");
      fflush(stderr);
      exit(-1);
    }
    g->fen[i] = fen[i];
    g->fen[i+1] = '\0';  
  }
  BoardSetFromFEN(g->board, g->fen);
}

/* ---------------------------------------------------------
------------------------------------------------------------
  Updating the game history. The functions ..
    GamePushHistory(_Game * g); 
  .. may be called after each move, in case you need to ..
  .. revert any move (while analysing the game) or you ..
  .. want to save the game. 
  The function
    GamePopHistory(_Game * g);
  .. undo the last move.
------------------------------------------------------------
--------------------------------------------------------- */

void GamePushHistory(_Game * g){
  /** Add current FEN to history */
  array_append ( g->history, g->fen, FEN_MAXSIZE);
  //TODO: Add move also to the history 
}

void GamePopHistory(_Game * g){
  /** Remove last move from history. ..
  .. In case of reverting a move */
  Array * h = g->history;
  if(!h->len){
    fprintf(stderr, "Warning: Empty history");
    return;
  }
  h->len -= FEN_MAXSIZE;
  char * fen = (char *) (h->p) + h->len;
  GameSetBoard(g, fen); 
}

/* ---------------------------------------------------------
------------------------------------------------------------
  Update current FEN (using the board "g->board") after ..
  .. the move. The FEN is required to update history.
------------------------------------------------------------
--------------------------------------------------------- */
void GameFEN(_Game * g){
  /* Get FEN from Board */
  BoardFEN(g->board, g->fen);
}

/* ---------------------------------------------------------
------------------------------------------------------------
  The function 
    GameIsAttackedByPiece (from, rays[], nrays, depth, sq);
  .. can be used to see if a square "sq" is attacked by ..
  .. the the piece in the square "from". The array  ..
  .. "rays[]" stores the rays along which piece can move.
  .. "nrays" is the size of "rays[]", "depth" is the ..
  .. the max depth along the ray the piece can move. It ..
  .. 7 or rook, queen, and bishop.
  Function pointers corresponding to each chesspiece, that ..
  .. can see if a square is attacked by this piece are ..
  .. listed. 
------------------------------------------------------------
--------------------------------------------------------- */

_BoardMove * GameBot(_Game * g) {
  // Algorithm Not yet implemented
  // Random move (As of now)
  _BoardMove * move = NULL;
      
  if(g->moves->len) {
    
    int nmoves = (g->moves->len) / sizeof(_BoardMove);

    move = (_BoardMove *) g->moves->p;
    int imove = floor (((double) nmoves)*rand()/RAND_MAX);
    move += imove;
  }

  return move;
}

//void GameError(_Game * g) {
//  BoardStatusPrint(g->board);
//}

//Next Move
Flag GameMove(_Game * g, _BoardMove * move){
  Flag status = BoardNext(g->board,move,g->moves);
  GameFEN(g);
  return status;
}

Flag Game(_Game * g) {
  srand(time(0));
  _Board * b = g->board;
  while ( b->status == GAME_CONTINUE ){
    _BoardMove * move = NULL;
    if(1)
      move = GameBot(g);
    else {
      //Input from an input device
    }
    GameMove(g, move); 
    GamePrintBoard(g, 300);
  }
  //assert(b->status); //Game is over
  BoardStatusPrint(b);
  return (b->status);
}

/* ---------------------------------------------------------
------------------------------------------------------------
  To create a game (_Game) instance, call
    GameNew(char * fen);
  To start with default board position, call as
    GameNew(NULL);
  To free the memory created for the game instance, call ..
    GameDestroy(Game * g);
------------------------------------------------------------
--------------------------------------------------------- */

_Game * GameNew(char * fen){
  BoardInitIterator();
  //Game instance
  _Game * g = (_Game *) malloc (sizeof (_Game));
  _Board * b = Board(NULL);
  g->board = b; 
  GameSetBoard(g, fen);
  //Allocate memory for possible moves,
  Array * moves = array_new();
  g->moves = moves; 
  //Allocate memory for game history.
  Array * history = array_new();
  g->history = history;
  //See if the king (whose turn) is on check    
  b->check = BoardIsKingAttacked(b, b->color);
  //Creates list of moves for the new board
  BoardAllMoves(b, g->moves);
  if(b->status) {
    BoardPrint(b);
    fprintf(stderr, "\nWARNING : Game loaded has no moves");
    BoardStatusPrint(b);
  }
 
  return g; 
} 

void GameDestroy(_Game * g) {
  BoardDestroy(g->board);
  if(g->moves) 
    array_free(g->moves);
  if(g->history)
    array_free(g->history);
  free(g);
}

