#include "board.h"

/* 
TODO:     SAN for _GameMove * move
*/
/* ---------------------------------------------------------
------------------------------------------------------------
When can a game draw:
  Stalemate:	Player has no legal moves but is not in check.
  Insufficient Material:	Neither player can 
     checkmate with the pieces remaining.
  Threefold Repetition:	The same position appears three times.
  Fifty-Move Rule:	50 moves without a pawn move or capture.
  Agreement:	Players agree to end the game as a draw.
  Dead Position:	No possible moves can lead to checkmate.
  Draw by Time: A player's time runs out, but 
    their opponent cannot deliver checkmate.
*/

const char KNIGHT_MOVES[8] = 
  { 14, 25, 23, 10, -14, -25, -23, -10 };
const char QUEEN_MOVES[8] =
  { 1, 13, 12, 11, -1, -13, -12, -11 };
const char ROOK_MOVES[4] = 
  { 1, 12, -1, -12 };
const char BISHOP_MOVES[4] = 
  { 13, 11, -13, -11 };
const char WPAWN_MOVES[4] = 
  { -11, -13, -12, -24 }; 
const char BPAWN_MOVES[4] = 
  { 11, 13, 12, 24 };   
 
/* ---------------------------------------------------------
------------------------------------------------------------
  An "Array" is used to store dynamic data. You can create, 
  .. reallocate, shrink and free the "Array" ..
  .. without any memory mismanagement. 
  NOTE: realloc() function used inside array_append(),
  .. can slow the program, when you append large data ..
  .. multiple times. WHY? : Because realloc search for .. 
  .. continous chunk of memory that can fit the new ..
  .. array data which might be very large. In this case, ..
  .. impelement memory pooling of fixed sized blocks ..
  .. Refer to:
      http://www.boost.org/doc/libs/1_55_0/libs/pool/doc/html/boost_pool/pool/pooling.html
------------------------------------------------------------
--------------------------------------------------------- */
typedef struct {
  void * p;
  long max, len;
} Array;

Array * array_new()
{
  Array * a = (Array *)malloc (sizeof(Array));
  a->p = NULL;
  a->max = a->len = 0;
  return a;
}

void array_free (Array * a)
{
  free (a->p);
  free (a);
}

void array_append (Array * a, void * elem, size_t size)
{
  if (a->len + size >= a->max) {
    a->max += size >4096 ? size : 4096;
    a->p = realloc (a->p, a->max);
  }
  memcpy (((char *)a->p) + a->len, elem, size);
  a->len += size;
}

void array_shrink (Array * a)
{
  if(a->len < a->max) {
    a->p = realloc (a->p, a->len);
    a->max = a->len;
  }
}
/* ---------------------------------------------------------
------------------------------------------------------------
  3 structs commonly used in this header file.
  a) unsigned char : A square of the chessboard
  b) _GameMove : Info corresponding to a move from ..
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
  Function to print the current FEN to stdout:
    GamePrintFEN(_Game * g);
  Function to print the current board to stdout.
    GamePrintBoard(_Game * g, int persist);
  In the above function use "persist" as 1 to print ..
  .. the board on the left top of the screen with a time ..
  .. small delay (0.4 s);
------------------------------------------------------------
--------------------------------------------------------- */

static inline void GamePrintFEN(_Game * g){
  fprintf(stdout, "\n%s", g->fen);
}

void GamePrintBoard(_Game * g, int persist) {
  
  //if persist. It clears the window
  if(persist) {
    clock_t start_time = clock();
    clock_t wait_time = 0.01*CLOCKS_PER_SEC ; //sleep time 
    while (clock() - start_time < wait_time) {};

    printf("\033[2J");       // Clear the screen
    printf("\033[1;1H");     //Cursor on the left top left
  } 
  GamePrintFEN(g);
  BoardPrint(g->board);
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

void GameSetBoard(_Game * g, char * _fen) {
  char * fen = _fen ? _fen : FEN_DEFAULT;
  g->fen[0] = '\0';
  for (int i=0; fen[i] != '\0'; ++i){
    if(i == FEN_MAXSIZE - 1){
      fprintf(stderr, "Error: Very Long FEN");
      fflush(stderr);
      exit(-1);
    }
    g->fen[i] = fen[i];
    g->fen[i+1] = '\0';  
  }
  BoardSetFromFEN(g->b, g->fen);
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
  BoardSetFEN(g->board, g->fen);
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


static inline 
int GameIsAttackedByPiece ( unsigned char * from, 
    const char rays[], int nrays, int depth, 
    unsigned char * sq) {
  //'from' and 'sq' should be the iterator in GAMEBOARD!!

  //'from' square can be neither empty nor outside the box
  assert ( !IS_OUTSIDE(from) && !IS_EMPTY(from) );

  for(int i=0; i<nrays; ++i) {
    unsigned char * to = from;
    for(int j=0; j<depth; ++j) {
      to += rays[i];
      if(IS_OUTSIDE(to)) 
        break; 
      if(sq == to) {      //comparing pointers 
        assert(!IS_BLOCKED (from,to));
        //Making sure that the piece occupying "to" ..
        // .. is not of the same color as the attacking piece 
        return 1;       //yes, the square "sq" is attacked.
      }
      if(!IS_EMPTY(to))
        break;          //Blocked by another piece
    }
  }
  return 0; // the square "sq" is safe from an attack
}

int GameIsAttackedByBPawn ( unsigned char * from, 
    unsigned char * sq) {
  GameIsAttackedByPiece(from, BPAWN_MOVES, 2, 1, sq);
}

int GameIsAttackedByWPawn ( unsigned char * from, 
    unsigned char * sq) {
  GameIsAttackedByPiece(from, WPAWN_MOVES, 2, 1, sq);
}

int GameIsAttackedByRook ( unsigned char * from, 
    unsigned char * sq) {
  GameIsAttackedByPiece(from, ROOK_MOVES, 4, 7, sq);
}

int GameIsAttackedByBishop ( unsigned char * from, 
    unsigned char * sq) {
  GameIsAttackedByPiece(from, BISHOP_MOVES, 4, 7, sq);
}

int GameIsAttackedByKnight ( unsigned char * from, 
    unsigned char * sq) {
  GameIsAttackedByPiece(from, KNIGHT_MOVES, 8, 1, sq);
}

int GameIsAttackedByQueen ( unsigned char * from, 
    unsigned char * sq) {
  GameIsAttackedByPiece(from, QUEEN_MOVES, 8, 7, sq);
}

int GameIsAttackedByKing ( unsigned char * from, 
    unsigned char * sq) {
  GameIsAttackedByPiece(from, QUEEN_MOVES, 8, 1, sq);
}

int (*GameIsSquareAttackedByPiece [14])
  (unsigned char *from, unsigned char * To) = {
    NULL, NULL, GameIsAttackedByBPawn, GameIsAttackedByWPawn, 
    GameIsAttackedByRook, GameIsAttackedByRook,
    GameIsAttackedByKnight, GameIsAttackedByKnight,
    GameIsAttackedByBishop, GameIsAttackedByBishop,
    GameIsAttackedByQueen, GameIsAttackedByQueen,
    GameIsAttackedByKing, GameIsAttackedByKing 
};
/* ---------------------------------------------------------
------------------------------------------------------------
  The function
    GameIsSquareAttacked( g, sq , color);
  .. check if the square "sq" is attacked by any piece of ..
  .. color "color".
    GameIsKingAttacked( g, color);
  .. is used to see if the king of color "color" is under ..
  .. any attack. This function can be used to (1) see if ..
  .. a move is valid or not; and to (2) see if a move ..
  .. produces a check.
------------------------------------------------------------
--------------------------------------------------------- */
int BoardIsSquareAttacked(_Board * b, 
    unsigned char s, unsigned char attackingcolor) {
   
  BoardMakeAvailable(b);
  unsigned char * sq = &(GAMEBOARD[s/8][s%8]);
  if( !IS_EMPTY(sq) )
    if( PIECE_COLOR(sq) == attackingcolor ) {
      // weird condition. This should't arise.
      // This condn arises when we are looking if 'sq' ..
      // .. occupied by a piece of color 'color' is being ..
      // .. checked if it's being attacked by pieces of ..
      // .. color 'color'!!!
      fprintf(stderr, "Warning: Weird attack query");
      fflush(stderr);
      assert(0);
    }

  /*check if the square "sq" is attacked by ..
  .. any pieces of color "color"*/
  for (int i=0; i<8; ++i)
    for(int j=0; j<8; ++j) {
      //Replace it with square iterator
      unsigned char * from = &(GAMEBOARD[i][j]);
      if( IS_EMPTY(from) )
  continue; //empty
      if( from == sq )
	continue;
      if( PIECE_COLOR(from) != attackingcolor )
  continue; //Occupied by the other color
      //Generate possible moves with the 'piece' ..
      // to see if 'piece' can attack 'sq' 
      if(GameIsSquareAttackedByPiece[from->piece](from, sq))
  return 1; //Some piece attacks "sq"
    }

  //Square "sq" is safe from any attack
  return 0;
}


int BoardIsKingAttacked(_Board * b, unsigned char color)  {
  /*check if the King of color "color" is attacked by ..
  .. any opponent piece */
  BoardIsSquareAttacked(b, b->king[color], !color);
}

int BoardIsMoveValid(_Board * b, _BoardMove * move) {

    BoardMove(b, move);
    int valid = !GameIsKingAttacked(g, g->color);
    if(valid)
      if(GameIsKingAttacked(g, !g->color))
        move->flags |= MOVE_CHECK;
    GameUnmovePiece(g, move);

    return valid;
}


_GameMove * GameBot(_Game * g) {
  // Algorithm Not yet implemented
  // Random move (As of now)
  _GameMove * move = NULL;
      
  int random_number = rand();
  if(g->moves->len) {
    
    int nmoves = (g->moves->len) / sizeof(_GameMove);

    move = (_GameMove *) g->moves->p;
    int imove = floor (((double) nmoves)*rand()/RAND_MAX);
    move += imove;
  }

  return move;
}

void GameError(unsigned int error) {
  if (error & 15) {
    unsigned int e = error & 15;
    if (e == 1) {
      fprintf(stderr, "Warning: Loaded game has no moves");
    }
    fflush(stderr);
  }
  if( error & 16) {
    fprintf(stdout, "\n %s wins by %s", 
      error & 32 ? "WHITE" : "BLACK",
      error & 64 ? "time" : "checkmate");
  }
  else if (error & 128) {
    unsigned int e = (error >> 8) & (1 | 2 | 4);
    fprintf(stdout, "\n Draw:");
    if(e == 0)
      fprintf(stdout, "Stalemate");
    else if (e == 1)
      fprintf(stdout, "Insufficient Material");
    else if (e == 2)
      fprintf(stdout, "Fifty-Move Rule");
    else if (e == 3)
      fprintf(stdout, "Threefold rule");
    else if (e == 4)
      fprintf(stdout, 
        "BLACK runs out of time and WHITE cannot win");
    else if (e == 5)
      fprintf(stdout, 
        "WHITE runs out of time and BLACK cannot win");
    else
      fprintf(stderr, "Error : Unknown Draw Reason");
  }
  fflush(stdout);
}

//Next Move
int GameMove(_Game * g, _GameMove * move){

  unsigned char * board = g->board[0];

  if(!move) {
    fprintf(stderr, "\nError: Move not chosen by bot");
    fprintf(stderr, "\nProbably loaded game is over");
    fflush(stdout);
    return 1; //Game Stopped
  }

  //Move
  GameMovePiece(g, move); 

  //Udate the halfclock, fullclock
  g->fullclock += (!g->color);
  g->halfclock = (move->flags & MOVE_CAPTURE) ? 0 :
    ((move->from.piece == WPAWN || move->from.piece == BPAWN) 
      ? 0 : (g->halfclock + 1));
  //Change the Turn
  g->color = !g->color;
  //Is the board on Check?
  g->check = move->flags & MOVE_CHECK;
  //Set En-Passante square while double pawn advance
  if( move->from.piece == WPAWN &&
      (move->from.square - move->to.square == 16) ) 
    g->enpassante = move->from.square - 8;
  else if( move->from.piece == BPAWN && 
      (move->to.square - move->from.square == 16) ) 
    g->enpassante = move->from.square + 8;
  else
    g->enpassante = OUTSIDE;
  
  if(g->castling) {
    //Switching off castling if king move moves
    if(move->from.square == 4)
      g->castling &= ~(MOVE_qCASTLE | MOVE_kCASTLE);
    else if (move->from.square == 60)
      g->castling &= ~(MOVE_QCASTLE | MOVE_KCASTLE);
  
    //Switching off castling if corner rooks move/captured
    if(move->from.square == 0 || move->to.square == 0)
      g->castling &= ~MOVE_qCASTLE;
    if(move->from.square == 7 || move->to.square == 7)
      g->castling &= ~MOVE_kCASTLE;
    if(move->from.square == 56 || move->to.square == 56)
      g->castling &= ~MOVE_QCASTLE;
    if(move->from.square == 63 || move->to.square == 63)
      g->castling &= ~MOVE_KCASTLE;
  }

  //Total number of pieces
  if(move->flags & MOVE_CAPTURE)
    --(g->board->npieces);

  //move is completed
  //move = NULL;
  
  //Update FEN
  GameFEN(g);
  //Creates list of moves for the new board
  //Game continues if (g->status == 0)
  return (GameAllMoves(g));
}

unsigned int Game(_Game * g) {
  srand(time(0)); 
  while ( !g->status ){
    _GameMove * move = NULL;
    if(1)
      move = GameBot(g);
    else {
      //Input from an input device
    }
    GameMove(g, move); 
    GamePrintBoard(g, 1);
  }
  assert(g->status); //Game is over
  return (g->status);
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
  g->board = Board(NULL);
  //Allocate memory for possible moves,
  g->moves = array_new();
  //Allocate memory for game history.
  g->history = array_new();
  //Set the Chessboard
  GameSetBoard(g, fen);
  //FIXME:BoardStatus();
  //See if the king (whose turn) is on check    
  g->check = GameIsKingAttacked(g, g->color);
  //Creates list of moves for the new board
  GameAllMoves(g);
  if(g->status) {
    fprintf(stderr, "\nWARNING : Game loaded has no moves");
    GameError(g->status);
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

