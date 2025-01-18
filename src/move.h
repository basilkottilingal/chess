#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include <math.h>

/* 
TODO: 
      Bug in Castling options in  GameKingMoves();  
      FEN from board 
      SAN for _GameMove * move
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
------------------------------------------------------------
--------------------------------------------------------- */

// Square is either empty ('0')
// .. Or occupied by a piece '2' : '13'
// .. Outside ('15') the box
#define OUTSIDE 15
#define EMPTY 0
#define WHITE 1 
#define BLACK 0
#define PIECE_SHIFT 1
#define BPAWN   ((1<<PIECE_SHIFT) | BLACK)
#define WPAWN   ((1<<PIECE_SHIFT) | WHITE)
#define BROOK   ((2<<PIECE_SHIFT) | BLACK) 
#define WROOK   ((2<<PIECE_SHIFT) | WHITE)
#define BKNIGHT ((3<<PIECE_SHIFT) | BLACK) 
#define WKNIGHT ((3<<PIECE_SHIFT) | WHITE)
#define BBISHOP ((4<<PIECE_SHIFT) | BLACK) 
#define WBISHOP ((4<<PIECE_SHIFT) | WHITE)
#define BQUEEN  ((5<<PIECE_SHIFT) | BLACK)
#define WQUEEN  ((5<<PIECE_SHIFT) | WHITE)
#define BKING   ((6<<PIECE_SHIFT) | BLACK)
#define WKING   ((6<<PIECE_SHIFT) | WHITE)


#define FEN_MAXSIZE 80

// Chesspieces: For faster translation b/w ..
// .. chesspieces' usual notation and thier number notation 
char MAPPING[16] = 
  { '.', '.',
    'p', 'P', 'r', 'R', 'n', 'N',
    'b', 'B', 'q', 'Q', 'k', 'K',
    '.', 'x' 
  };

unsigned char MAPPING2[58] = 
  { 'A', WBISHOP, 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 
    WKING, 'L', 'M', WKNIGHT, 'O', WPAWN, WQUEEN, 
    WROOK, 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 
    ' ', ' ', ' ', ' ', ' ', ' ', 'a', BBISHOP, 'c', 'd', 
    'e', 'f', 'g', 'h', 'i', 'j', BKING, 'l', 'm', 
    BKNIGHT, 'o', BPAWN, BQUEEN, BROOK, 's', 't', 
    'u', 'v', 'w', 'x', 'y', 'z'
  };

/*
const char QUEEN_MOVES[8][2] = {
  {1,0}, {1,1}, {0,1}, {-1,1},
  {-1,0}, {-1,-1}, {0,-1}, {1,-1} };
const char BISHOP_MOVES[4][2] = {
  {1,1}, {-1,1}, {-1,-1}, {1,-1} };
const char ROOK_MOVES[4][2] = {
  {1,0}, {0,1}, {-1,0}, {0,-1} };
const char KNIGHT_MOVES[8][2] = {
  {2,1}, {1,2}, {1,-2}, {2,-1},
  {-2,-1}, {-1,-2}, {-1,2}, {-2,1} };
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

static inline unsigned char SQUARE64(char * s){
  return ( 8 * (8 - s[1] + '0' ) + s[0] - 'a' );
}

// 8 bits of the flags (unsigned char)
#define MOVE_NORMAL 0
#define MOVE_CAPTURE 1
#define MOVE_PROMOTION 2
#define MOVE_ENPASSANTE 4
#define MOVE_kCASTLE 8
#define MOVE_KCASTLE 16
#define MOVE_qCASTLE 32
#define MOVE_QCASTLE 64
#define MOVE_CHECK 128

//Identifying rank, file, piece color
#define PIECE_COLOR(SQUARE) ((SQUARE).piece & 1)
#define BOARD_FILE(SQUARE)  ('a' + (SQUARE).square%8)
#define BOARD_RANK(SQUARE)  ('0' + 8 - (SQUARE).square/8)
#define BOARD_PIECE(SQUARE) (MAPPING[(SQUARE).piece])

//conditions to check while moving piece from 'FROM' to 'TO'
#define IS_OUTSIDE(SQUARE)  ((SQUARE).piece == OUTSIDE)
#define IS_EMPTY(SQUARE)    ((SQUARE).piece == EMPTY)
#define IS_PIECE(SQUARE)    (!IS_OUTSIDE(SQUARE) && \
  !IS_EMPTY(SQUARE) )
#define IS_NORMAL(FROM,TO)  ((TO).piece == EMPTY)
#define IS_BLOCKED(FROM,TO) (IS_PIECE(TO) && \
  (PIECE_COLOR(FROM) == PIECE_COLOR(TO)) )
#define IS_CAPTURE(FROM,TO) (IS_PIECE(TO) && \
  (PIECE_COLOR(FROM) != PIECE_COLOR(TO)) )
#define IS_PROMOTION(FROM,TO)\
  ( (( (FROM).piece == WPAWN) && (BOARD_RANK(TO) == '8')) || \
    (( (FROM).piece == BPAWN) && (BOARD_RANK(TO) == '1')) )
#define IS_ENPASSANTE(FROM,TO,GAME) \
  ( ((FROM).piece == WPAWN && BOARD_RANK(FROM) == '5' && \
     (TO).square == (GAME).enpassante ) ||\
    ((FROM).piece == BPAWN && BOARD_RANK(FROM) == '4' && \
     (TO).square == (GAME).enpassante ) )
 
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
  a) _GameSquare : A square of the chessboard
  b) _GameMove : Info corresponding to a move from ..
    the current board location
  c) _Game : Store all the information corresponding to ..
    a chess game including board information, other ..
    informations like whose move, location of each kings,
    any active en-passante, are castling available, etc.
------------------------------------------------------------
--------------------------------------------------------- */

typedef struct {
  unsigned char piece;
  unsigned char square;
}_GameSquare;

typedef struct {
  /* In case of promotion (flags & PROMOTION == 1), 
  .. "promotion" will store the promoted piece */
  _GameSquare from, to;
  unsigned char flags, promotion;
  char SAN[8];
}_GameMove;

typedef struct {
  //8x8 board with padding
  _GameSquare ** board;  
  //location of kings
	_GameSquare * king[2]; 
  unsigned char enpassante, castling, color;
  unsigned char halfclock, fullclock, npieces;
  unsigned char check;
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
  _GameSquare ** board = g->board;
  
  //if persist. It clears the window
  if(persist) {
    clock_t start_time = clock();
    clock_t wait_time = 0.01*CLOCKS_PER_SEC ; //sleep time 
    while (clock() - start_time < wait_time) {};

    printf("\033[2J");       // Clear the screen
    printf("\033[1;1H");     //Cursor on the left top left
  } 
  fprintf(stdout,"\nBoard");
    
  GamePrintFEN(g);

  for (int i=0; i<8; ++i){
    fprintf(stdout,"\n %c ", '0'+8-i);
    for (int j=0; j<8; ++j) {
      unsigned char piece = board[i][j].piece;
      fprintf(stdout," %c", MAPPING[piece]);
    }
  }
    
  fprintf(stdout,"\n\n   ");
  for (int j=0; j<8; ++j) {
    fprintf(stdout," %c", 'a'+j);
  }
  fprintf(stdout,"\n");
}

/* ---------------------------------------------------------
------------------------------------------------------------
  The following function
    GameSetBoard(_Game * g, char * _fen); //.. 
  .. sets the game and board from the parsed FEN "_fen"
------------------------------------------------------------
--------------------------------------------------------- */

void GameSetBoard(_Game * g, char * _fen) {
  char _fen0[] = 
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
  char * fen = _fen ? _fen : _fen0;
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

  g->npieces = 0; 
  g->king[WHITE] = NULL; g->king[BLACK] = NULL;

  /* Set board from FEN */ 
  _GameSquare ** rank = g->board;
  _GameSquare * square = rank[0];
  unsigned char sid = 0; // square id
  while(*fen != '\0') {
    char c = *fen;
    ++fen;
    /* Empty squares */ 
    if (isdigit(c)) {
      int nempty = c - '0';
      assert(nempty > 0 && nempty <= 8);
      for (int i=0; i<nempty; ++i) {
        square->piece = 0; // EMpty piece
        square++->square = sid++; //Square id [0:64)
      }
    }
    else if (c == '/') {
      assert( sid%8 == 0 ); 
        //Make sure all squares of this rank are filled
      ++rank;
      square = rank[0];
    } 
    else if (c == ' '){
      assert( sid == 64 ); //Make sure all squares are filled
      break;
    }
    else{
      square->piece =  MAPPING2[c - 'A']; // occupied square
      if(square->piece == WKING) {
        assert(!g->king[WHITE]); 
        //There cannot be multiple kings
        g->king[WHITE] = square;
      }
      else if(square->piece == BKING) {
        assert(!g->king[BLACK]); 
        //There cannot be multiple kings
        g->king[BLACK] = square; 
      }
      else { 
        assert((g->npieces)++ < 30);
      }
      square++->square = sid++; //Square id [0:64)
    }
  }
  //Make sure that there are both 'k' and 'K' in the FEN;
  assert(g->king[WHITE] && g->king[BLACK]);

  //Let's see whose turn is now ('w'/'b')
  g->color = (*fen == 'w') ? WHITE : BLACK;
  assert(*fen == 'w' || *fen == 'b');
  assert(*(++fen) == ' '); 

  g->castling = 0;
  while(*(++fen) != '\0') {
    char c = *fen;
    if (c == ' ')
      break;
    if (c == 'k')
      g->castling |= MOVE_kCASTLE;
    if (c == 'q')
      g->castling |= MOVE_qCASTLE;
    if (c == 'K')
      g->castling |= MOVE_KCASTLE;
    if (c == 'Q')
      g->castling |= MOVE_QCASTLE;
  }
  assert(*fen++ == ' ');

  g->enpassante = 64;
  if(*fen != '-'){
    g->enpassante = SQUARE64(fen);
    ++fen;
  }
  assert(*(++fen) == ' ');

  //Halfmove clocks are reset during a capture/a pawn advance
  g->halfclock = 0;
  while(*(++fen) != '\0'){
    char c = *fen;
    if( c == ' ' )
      break;
    assert(isdigit(c));
    g->halfclock = 10*g->halfclock + (c - '0');
    assert(g->halfclock <= 50);
  }
  assert(*fen == ' ');

  g->fullclock = 0;
  while(*(++fen) != '\0'){
    char c = *fen;
    assert(isdigit(c));
    g->fullclock = 10*g->fullclock + (c - '0');
    // No recorded FIDE game exceeded 300 moves.
    // .. I don't know the theoretical limit. 
    // .. I think draw (by 50moves rule) would have ..
    // .. occured before 1000 moves??
    assert(g->fullclock <= 10000);
  }
  assert(g->fullclock); //has to be greater than 0;
  assert(*fen == '\0');

  return;
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
}

/* ---------------------------------------------------------
------------------------------------------------------------
  The function
    GameMovePiece(g, move);
  .. temporarily updates the board of the game "g" with ..
  .. the move "move" (i.e. only board is updated, move ..
  .. is not finalised.). And the function
    GameUnmovePiece(g, move);
  .. undo the effect of GameMovePiece(g,move);
------------------------------------------------------------
--------------------------------------------------------- */
 
  
void GameMovePiece(_Game * g, _GameMove * move){
  assert(move);

  unsigned char f = move->from.square, t = move->to.square;
  _GameSquare ** board = g->board,
    * from = &(board[f/8][f%8]),
    * to   = &(board[t/8][t%8]);
  from->piece = EMPTY;
  to->piece = (move->flags & MOVE_PROMOTION) ? 
    move->promotion : move->from.piece;

  if(to->piece == WKING) {
    g->king[WHITE] = to;
    if(move->flags & MOVE_QCASTLE) {
      board[7][0].piece = EMPTY;
      board[7][3].piece = WROOK;
      return;
    }
    if(move->flags & MOVE_KCASTLE) {
      board[7][7].piece = EMPTY;
      board[7][5].piece = WROOK;
      return;
     }
  }
  else if(to->piece == BKING) {
    g->king[BLACK] = to;
    if(move->flags & MOVE_qCASTLE) {
      board[0][0].piece = EMPTY;
      board[0][3].piece = BROOK;
      return;
    }
    if(move->flags & MOVE_kCASTLE) {
      board[0][7].piece = EMPTY;
      board[0][5].piece = BROOK;
      return;
    }
  }

  if(move->flags & MOVE_ENPASSANTE) {
    _GameSquare * en = to + (g->color ? 12 : -12); 
    en->piece = EMPTY;
    return;
  }
} 
  
void GameUnmovePiece(_Game * g, _GameMove * move){
  assert(move);

  unsigned char f = move->from.square, t = move->to.square;
  _GameSquare ** board = g->board,
    * from = &(board[f/8][f%8]),
    * to   = &(board[t/8][t%8]);
  to->piece = move->to.piece;
  from->piece = move->from.piece;

  if(from->piece == WKING) {
    g->king[WHITE] = from;
    if(move->flags & MOVE_QCASTLE) {
      board[7][3].piece = EMPTY;
      board[7][0].piece = WROOK;
      return;
    }
    if(move->flags & MOVE_KCASTLE) {
      board[7][5].piece = EMPTY;
      board[7][7].piece = WROOK;
      return;
    }
  }
  else if(from->piece == BKING) {
    g->king[BLACK] = from;
    //FIXME: All te four conditions may be compressed
    if(move->flags & MOVE_qCASTLE) {
      board[0][3].piece = EMPTY;
      board[0][0].piece = BROOK;
      return;
    }
    if(move->flags & MOVE_kCASTLE) {
      board[0][5].piece = EMPTY;
      board[0][7].piece = BROOK;
      return;
    }
  }
   
  if(move->flags & MOVE_ENPASSANTE) {
    _GameSquare * en = to + (g->color ? 12 : -12); 
    en->piece = g->color ? BPAWN : WPAWN;
    return;
  }
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

static inline int GameIsAttackedByPiece ( _GameSquare * from, 
    const char rays[], int nrays, int depth, 
    _GameSquare * sq) {

  //'from' square can be neither empty nor outside the box
  assert ( !IS_OUTSIDE(*from) && !IS_EMPTY(*from) );

  for(int i=0; i<nrays; ++i) {
    _GameSquare * to = from;
    for(int j=0; j<depth; ++j) {
      to += rays[i];
      if(IS_OUTSIDE(*to)) 
        break; 
      if(sq == to) {      //comparing pointers 
        assert(!IS_BLOCKED (*from,*to));
        //Making sure that the piece occupying "to" ..
        // .. is not of the same color as the attacking piece 
        return 1;       //yes, the square "sq" is attacked.
      }
      if(!IS_EMPTY(*to))
        break;          //Blocked by another piece
    }
  }
  return 0; // the square "sq" is safe from an attack
}

int GameIsAttackedByBPawn ( _GameSquare * from, 
    _GameSquare * sq) {
  GameIsAttackedByPiece(from, BPAWN_MOVES, 2, 1, sq);
}

int GameIsAttackedByWPawn ( _GameSquare * from, 
    _GameSquare * sq) {
  GameIsAttackedByPiece(from, WPAWN_MOVES, 2, 1, sq);
}

int GameIsAttackedByRook ( _GameSquare * from, 
    _GameSquare * sq) {
  GameIsAttackedByPiece(from, ROOK_MOVES, 4, 7, sq);
}

int GameIsAttackedByBishop ( _GameSquare * from, 
    _GameSquare * sq) {
  GameIsAttackedByPiece(from, BISHOP_MOVES, 4, 7, sq);
}

int GameIsAttackedByKnight ( _GameSquare * from, 
    _GameSquare * sq) {
  GameIsAttackedByPiece(from, KNIGHT_MOVES, 8, 1, sq);
}

int GameIsAttackedByQueen ( _GameSquare * from, 
    _GameSquare * sq) {
  GameIsAttackedByPiece(from, QUEEN_MOVES, 8, 7, sq);
}

int GameIsAttackedByKing ( _GameSquare * from, 
    _GameSquare * sq) {
  GameIsAttackedByPiece(from, QUEEN_MOVES, 8, 1, sq);
}

int (*GameIsSquareAttackedByPiece [14])
  (_GameSquare *from, _GameSquare * To) = {
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

int GameIsSquareAttacked(_Game * g, 
    _GameSquare * sq, unsigned char color) {

  /*check if the square "sq" is attacked by ..
  .. any pieces of color "color"*/
  _GameSquare ** board = g->board;
  for (int i=0; i<8; ++i)
    for(int j=0; j<8; ++j) {
      _GameSquare * from = &(board[i][j]);
      if( IS_EMPTY(*from) )
  continue; //empty
      if( from == sq )
	continue;
      if( PIECE_COLOR(*from) != color )
  continue; //Occupied by the other color
      //Generate possible moves with the 'piece' ..
      // to see if 'piece' can attack 'sq' 
      if(GameIsSquareAttackedByPiece[from->piece](from, sq))
  return 1; //Some piece attacks "sq"
    }

  //Square "sq" is safe from any attack
  return 0;
}


int GameIsKingAttacked(_Game * g, unsigned char color)  {
  /*check if the King of color "color" is attacked by ..
  .. any opponent piece */
  GameIsSquareAttacked(g, g->king[color], !color);
}

int GameIsMoveValid(_Game * g, _GameMove * move) {
    GameMovePiece(g, move);
    //GamePrintBoard(g,1);

    int valid = !GameIsKingAttacked(g, g->color);
    if(valid) {
      int check = GameIsKingAttacked(g, !g->color);
      if(check) {
        //fprintf(stdout, " Check");
        move->flags |= MOVE_CHECK;
      }
    }
    else {
      //fprintf(stdout, " Invalid");
    }

    GameUnmovePiece(g, move);
    //GamePrintBoard(g,1);

    return valid;
}

/* ---------------------------------------------------------
------------------------------------------------------------
  Functions that generate moves for each pieces . 
  Each Function pointers orresonding to each pieces ..
  .. can be called as 
    GamePieceMoves[from->piece](g, from);
  The possible moves are appended to the array "g->moves".
  NOTE: The moves generated include illegal moves ( or ..
  .. those moves that allow the king on "attack" ). Those ..
  .. moves will be later removed. For more, Read the function
    GameAllMoves(_Game * g);
------------------------------------------------------------
--------------------------------------------------------- */


static inline void GameMovesFrom( _GameSquare * from, 
    const char rays[], int nrays, int depth, Array * moves) {
  
  unsigned char piece = from->piece;
  //'from' square can be neither empty nor outside the box
  assert ( !IS_OUTSIDE(*from) && !IS_EMPTY(*from) );

  for(int i=0; i<nrays; ++i) {
    _GameSquare * to = from;
    for(int j=0; j<depth; ++j) {
      to += rays[i];
      // Cannot move along the ray, will end up outside board 
      if(IS_OUTSIDE(*to))
        break; 
      // Occupied by same color; 'break' moving along the ray
      if(IS_BLOCKED(*from, *to))
        break;
      unsigned char flags = IS_EMPTY(*to)  
        ? MOVE_NORMAL : MOVE_CAPTURE;
      
      _GameMove move = {
        .from.piece = from->piece,
        .from.square = from->square,
        .to.piece = to->piece,
        .to.square = to->square,
        .flags = flags
      };
      //Add to the list of possible moves.
      array_append( moves, &move, sizeof(move) );

      // Cannot move further beyond a capture.
      if(flags & MOVE_CAPTURE)
        break;
    }
  }
}

void GameQueenMoves(_Game * g, _GameSquare * from){
  GameMovesFrom(from, QUEEN_MOVES, 8, 7, g->moves); 
}

void GameKingMoves(_Game * g, _GameSquare * from){
  GameMovesFrom(from, QUEEN_MOVES, 8, 1, g->moves);
  //Check for castling ability
  if ( (MOVE_kCASTLE << g->color) & (g->castling) ) {
    //King Side castling
    int available = 1;
    _GameSquare * king = g->king[g->color];
    assert(king->square == (4 + 8*7*(g->color)));
    assert(king == from);
    assert(king[3].piece == (g->color ? WROOK : BROOK));  
    for(int i=0; i<4; ++i) {
      // see if king, rook and the 2 squares in b/w them ..
      // .. are under attack
      if(GameIsSquareAttacked (g, king + i, !g->color)) {
        available = 0;
        break;
      }
    }
    for(int i=1; i<3; ++i) {
      // see if 2 squares b/w king and rook are empty
      if(!IS_EMPTY(king[i])) {
        available = 0;
        break;
      }
    }
    
    if(available) {
      _GameMove move = {
        .from.piece = king->piece,
        .from.square = king->square,
        .to.piece = EMPTY,
        .to.square = king[2].square,
        .flags = MOVE_kCASTLE << g->color
      };
      array_append(g->moves,&move, sizeof(move));
    }
  }
  if ( (MOVE_qCASTLE << g->color) & (g->castling) ) {
    //Queen Side castling
    int available = 1;
    _GameSquare * king = g->king[g->color];
    assert(king->square == (4 + 8*7*(g->color)));
    assert(king == from);
    assert(king[-4].piece == (g->color ? WROOK : BROOK));  
    for(int i=-4; i<1; ++i) {
      // see if king, rook and the 3 squares in b/w them ..
      // .. are under attack
      if(GameIsSquareAttacked (g, king + i, !g->color)) {
        available = 0;
        break;
      }
    }
    for(int i=-3; i<0; ++i) {
      // see if 3 squares b/w king and rook are empty
      if(!IS_EMPTY(king[i])) {
        available = 0;
        break;
      }
    }
    
    if(available) {
      _GameMove move = {
        .from.piece = king->piece,
        .from.square = king->square,
        .to.piece = EMPTY,
        .to.square = king[-2].square,
        .flags = MOVE_qCASTLE << g->color
      };
      array_append(g->moves,&move, sizeof(move));
    }
  }
}

void GameBishopMoves(_Game * g, _GameSquare * from){
  GameMovesFrom(from, BISHOP_MOVES, 4, 7, g->moves); 
}

void GameKnightMoves(_Game * g, _GameSquare * from){
  GameMovesFrom(from, KNIGHT_MOVES, 8, 1, g->moves); 
}

void GameRookMoves(_Game * g, _GameSquare * from){
  GameMovesFrom(from, ROOK_MOVES, 4, 7, g->moves); 
}

void GamePawnMoves(_Game * g, _GameSquare * from, 
    const char rays[]){

  for(int j=0; j<2; j++) {
    //Diagonal advance of pawn
    _GameSquare * to = from + rays[j];
    unsigned char flags = IS_CAPTURE(*from,*to) ? 
      MOVE_CAPTURE : IS_ENPASSANTE(*from,*to,*g) ?
      MOVE_ENPASSANTE : 0;
  
    //Pawn move diagonally only if it's a  ..
    //.. capture or an "en-passante" capture
    if(!flags) 
      continue;

    flags |= IS_PROMOTION(*from, *to) ? MOVE_PROMOTION : 0;

    _GameMove move = {
      .from.piece = from->piece,
      .from.square = from->square,
      .to.piece = to->piece,
      .to.square = to->square,
      .flags = flags
    };
     
    Array * moves = g->moves;
    if(flags & MOVE_PROMOTION) {
      move.promotion = from->piece;
      for(int i=0; i<4; ++i) {
        //'p' is promoted to 'r','b','n' and 'q'
        move.promotion += 1 << PIECE_SHIFT; 
        array_append (moves, &move, sizeof(move));
      } 
    }
    else {
      array_append (moves, &move, sizeof(move));
    }
  
  }
  //FIXME: can be made into :for(int j=0; j<4; ++j) {}
  for(int j=2; j<4; j++) {
    //vertical advance of pawn
    _GameSquare * to = from + rays[j];
    if(!IS_EMPTY(*to))
      break; //capture, block, outside leads to "break"
    unsigned char flags = MOVE_NORMAL;
    flags |= IS_PROMOTION(*from, *to) ? MOVE_PROMOTION : 0;
    _GameMove move = {
      .from.piece = from->piece,
      .from.square = from->square,
      .to.piece = to->piece,
      .to.square = to->square,
      .flags = flags
    };
     
    Array * moves = g->moves;
    if(flags & MOVE_PROMOTION) {
      move.promotion = from->piece;
      for(int i=0; i<4; ++i) {
        //'p' is promoted to 'r','b','n' and 'q'
        move.promotion += 1 << PIECE_SHIFT; 
        array_append (moves, &move, sizeof(move));
      } 
    }
    else {
      array_append (moves, &move, sizeof(move));
    }

    //double advance only for starting pawns
    if( BOARD_RANK(*from) != (g->color ? '2' : '7') )
      break;  
  }
}

void GameBPawnMoves(_Game * g, _GameSquare * from){
  GamePawnMoves(g, from, BPAWN_MOVES);
}

void GameWPawnMoves(_Game * g, _GameSquare * from){
  GamePawnMoves(g, from, WPAWN_MOVES);
}

void (*GamePieceMoves[14]) (_Game *, _GameSquare *) = 
  { NULL, NULL, GameBPawnMoves, GameWPawnMoves, 
    GameRookMoves, GameRookMoves,
    GameKnightMoves, GameKnightMoves,
    GameBishopMoves, GameBishopMoves,
    GameQueenMoves, GameQueenMoves,
    GameKingMoves, GameKingMoves };


int GameAllMoves(_Game * g){
  /* Find all moves by rule. 
   If on check and no moves return 0. Game over 
   If on check and there are atleast 1 move which nullify check.
   If Not on check and there are no moves return 2. Stalemate.
   If Not on check and there are some moves return 3 */
  
  if (!g->npieces) {
      return (128 | (1 << 8)); //Insufficient pieces
  }

  //Empty the moves array.
  Array * moves = g->moves;
  moves->len = 0;

  _GameSquare ** board = g->board;
  for (int i=0; i<8; ++i)
    for(int j=0; j<8; ++j) {
      _GameSquare * from = &(board[i][j]);
      if( IS_EMPTY(*from) )
  continue; //empty
      if( PIECE_COLOR(*from) != g->color )
  continue; //Occupied by the other color
      //Generate possible moves with the 'piece' 
      GamePieceMoves[from->piece](g, from);
    }
  
  //Removing Invalid Moves
  size_t smove = sizeof(_GameMove);
  int nmoves =  (int) (moves->len / smove);
  _GameMove * move = (_GameMove *) (g->moves->p);
  _GameMove * m = move;
  for(int i=0; i<nmoves; ++i, ++move) {
    if(GameIsMoveValid(g, move)) {
      if(!(m == move)) //To avoid memcpy to same dest
        memcpy(m, move, smove);
      ++m;
    }
    else 
      moves->len -= smove;
  }

  //See if the Game is over. Bcs no moves available
  if(!g->moves->len) {
    if(g->check)
      return (16 | (g->color ? 0 : 32)); //someone wins 
    else
      return (128); //stalemate 
  } 
  
  return 0; //Game continues
    
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
    else
      fprintf(stdout, "Any Other Reasons");
  }
  fflush(stdout);
}

//Next Move
int GameMove(_Game * g, _GameMove * move){

  _GameSquare * board = g->board[0];

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
  if(g->halfclock == 50) {
    return (128 | (2 << 8)); //Draw by 50 moves rule.
  }
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
    g->enpassante = 64;
  
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

//Switch of this later.
#if 1
if(g->castling){
  for(int i=0; i<2;++i) {
    if(g->castling & (MOVE_kCASTLE<<i)){
      _GameSquare * king = g->king[i];
      assert(king->square == (4 + 8*7*(i)));
      assert(king[0].piece == (i ? WKING : BKING));  
      if(king[3].piece != (i ? WROOK : BROOK)){
fprintf(stderr, "\nk ? ? r\n");
for(int i=0; i<4; ++i) 
  fprintf(stderr, "%c ", MAPPING[king[i].piece]);
fprintf(stderr, "\n");
for(int i=0; i<4; ++i) 
  fprintf(stderr, "%c ", MAPPING[(king+i)->piece]);
fflush(stderr);
exit(-1);
      }
    }
    if(g->castling & (MOVE_qCASTLE<<i)){
      _GameSquare * king = g->king[i];
      assert(king->square == (4 + 8*7*(i)));
      assert(king[0].piece == (i ? WKING : BKING));  
      if(king[-4].piece != (i ? WROOK : BROOK)){ 
fprintf(stderr, "\nr ? ? ? r\n");
for(int i=0; i<4; ++i) 
  fprintf(stderr, "%c ", MAPPING[king[i].piece]);
fprintf(stderr, "\n");
for(int i=-4; i<=0; ++i) 
  fprintf(stderr, "%c ", MAPPING[(king+i)->piece]);
fprintf(stderr, "\n");
for(int i=4; i>=0; --i) 
  fprintf(stderr, "%c ", MAPPING[(king-i)->piece]);
fflush(stderr);
exit(-1);
      }
    }
  }
}
#endif

  //Total number of pieces
  if(move->flags & MOVE_CAPTURE)
    --(g->npieces);

  //move is completed
  //move = NULL;

  //Creates list of moves for the new board
  int status = GameAllMoves(g);
  
  return status; //Game continues if status = 0
}

unsigned int Game(_Game * g) {
  srand(time(0)); 
  unsigned int status = 0;
  while ( !status ){
    _GameMove * move = NULL;
    if(1)
      move = GameBot(g);
    else {
      //Input from an input device
    }
    status = GameMove(g, move); 
    GamePrintBoard(g, 1);
  }
  assert(status);
  return status;
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

static inline 
_GameSquare ** GameBoard(){
  //Allocate mem for 2-D board (8x8) with ..
  //.. 2 layer padding on each sides (12x12).
  //accessible: board[-2:9][-2:9]
  //Valid part: board[0:7][0:7]
  int b = 8, p = 2;
  int tb = b + 2*p;
  _GameSquare ** board = (_GameSquare **)
    malloc(tb*sizeof(_GameSquare *));
  board[0] = (_GameSquare *)
    malloc(tb*tb*sizeof(_GameSquare));
  for(int i=1; i<tb; ++i)
    board[i] = board[i-1] + tb;
  for(int i=0; i<tb; ++i) {
    for(int j=0; j<tb; ++j)
      board[i][j].piece = OUTSIDE; 
    board[i] += p;
  }
  board += p;
  return board;
}

_Game * GameNew(char * fen){

  //Game instance
  _Game * g = (_Game *) malloc (sizeof (_Game));

  g->board = GameBoard();;

  //Allocate memory for possible moves,
  g->moves = array_new();

  //Allocate memory for game history.
  g->history = array_new();

  //Set the Chessboard
  GameSetBoard(g, fen);
  //See if the king (whose turn) is on check    
  g->check = GameIsKingAttacked(g, g->color);
  //Creates list of moves for the new board
  unsigned int status = GameAllMoves(g);
  if(status) {
    fprintf(stderr, "\nWARNING : Game loaded is over");
    GameError(status);
  }
 
  return g; 
  
} 

void GameDestroy(_Game * g) {
  _GameSquare ** board = g->board;
  int p = 2;
  board -= p;
  board[0] -= p; 
  free(board[0]);
  free(board);
  
  if(g->moves) 
    array_free(g->moves);
  
  if(g->history)
    array_free(g->history);

  free(g);
}

