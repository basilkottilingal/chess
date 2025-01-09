#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>
#include <limits.h>

#define OUTSIDE 15
#define PIECE_SHIFT 1
#define WHITE 1 
#define BLACK 0

#define BPAWN ((1<<PIECE_SHIFT) | BLACK)
#define WPAWN ((1<<PIECE_SHIFT) | WHITE)
#define BROOK ((2<<PIECE_SHIFT) | BLACK) 
#define WROOK ((2<<PIECE_SHIFT) | WHITE)
#define BKNIGHT ((3<<PIECE_SHIFT) | BLACK) 
#define WKNIGHT ((3<<PIECE_SHIFT) | WHITE)
#define BBISHOP ((4<<PIECE_SHIFT | BLACK)) 
#define WBISHOP ((4<<PIECE_SHIFT) | WHITE)
#define BQUEEN ((5<<PIECE_SHIFT) | BLACK)
#define WQUEEN ((5<<PIECE_SHIFT) | WHITE)
#define BKING ((6<<PIECE_SHIFT) | BLACK)
#define WKING ((6<<PIECE_SHIFT) | WHITE)
#define PIECE_COLOR(SQUARE) (PIECE(SQUARE) & 1)

#define MOVE_NORMAL 1
#define MOVE_CAPTURE 2
#define MOVE_PROMOTION 4
#define MOVE_KCASTLE 8
#define MOVE_QCASTLE 16
#define MOVE_ENPASSANTE 32

// Chesspieces: For faster translation b/w ..
// .. chesspieces' usual notation and thier number notation 
char MAPPING[17] = 
  { '.', '.',
    'p', 'P', 'r', 'R', 'n', 'N',
    'b', 'B', 'q', 'Q', 'k', 'K',
    '.', '.', 'x' 
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

#define PIECE(SQUARE) ((SQUARE).piece)
#define BOARD_FILE(SQUARE) ('a' + (SQUARE).square%8)
#define BOARD_RANK(SQUARE) ('0' + 8 - (SQUARE).square/8)
#define BOARD_PIECE(SQUARE) (MAPPING[(SQUARE).piece])
#define TOGGLE_COLOR(SQUARE) (PIECE(SQUARE)^1)


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
const char KNIGHT_MOVES[8] = {
  14, 25, 23, 10, -14, -25, -23, -10 }; 

static inline unsigned char SQUARE64(char * s){
  return ( 8 * (8 - s[1] + '0' ) + s[0] - 'a');
}


//Array is used to allocate, reallocate memory ..
//.. without any memory mismanagement.
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

void * array_shrink (Array * a)
{
  void * p = realloc (a->p, a->len);
  free (a);
  return p;
}

typedef struct {
  unsigned char piece;
  unsigned char square;
}_GameSquare;

typedef struct {
  unsigned char piece;
  unsigned char from;
  unsigned char to;
  char SAN[8];
  unsigned char flags;
  unsigned char promotion;
}_GameMove;

typedef struct {
  _GameSquare ** board;
  unsigned char enpassante, castling, color;
  unsigned char halfclock, fullclock;
  char fen[101];
  Array * moves, * history;
  _GameMove * move;
}_Game;

static inline void GameMovesFrom( _GameSquare * from, 
    const char rays[], int nrays, int depth, Array * moves) {
  
  unsigned char piece = from->piece;
  //'from' square can be neither empty nor outside the box
  assert ( piece != OUTSIDE && piece );

  for(int i=0; i<nrays; ++i) {

    _GameSquare * to = from;
    for(int j=0; j<depth; ++j) {
      to += rays[i];
      
      // Cannot move along the ray, will end up outside board 
      if(to->piece == OUTSIDE) 
        break; 
        
      // Occupied by same color; 'break' moving along the ray
      if(PIECE_COLOR(*to) == PIECE_COLOR(*from)) 
        break;
      fprintf(stdout, "\n\"%c\" moves along %d %d", 
        MAPPING[piece], rays[i], j+1);  
    }
  }
}

void GameQueenMove(_Game * g, _GameSquare * from){
  if(g->color != PIECE_COLOR(*from))
    return;
}

void GamePushHistory(_Game * g){
  /** Add current FEN to history */
}

void GamePopHistory(_Game * g){
  /** Remove last move from history. ..
  .. In case of reverting a move */
}

void GamePrintBoard(_Game * g) {
  _GameSquare ** board = g->board;
      
  fprintf(stdout,"\nBoard");
  for (int i=0; i<8; ++i){
  fprintf(stdout,"\n");
    for (int j=0; j<8; ++j) {
      unsigned char piece = board[i][j].piece;
      fprintf(stdout," %c", MAPPING[piece]);
    }
  }
  fprintf(stdout,"\nWhose Turn: %u", g->color);
  fprintf(stdout,"\nCastling: %u", g->castling);
  fprintf(stdout,"\nEnpassante %u", g->enpassante);
  fprintf(stdout,"\nHalfclock %u", g->halfclock);
  fprintf(stdout,"\nFullclock %u", g->fullclock);
}

void GameBoard(_Game * g) {
  /* Set board from FEN */ 
  _GameSquare ** rank = g->board;
  _GameSquare * square = rank[0];
  unsigned char sid = 0; // square id
  char * fen = g->fen;
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
      //check all squares of this rank are covered
      ++rank;
      square = rank[0];
    } 
    else if (c == ' '){
      assert( sid == 64 ); //Make sure all squares are filled
      break;
    }
    else{
        square->piece =  MAPPING2[c - 'A']; // EMpty piece
        square++->square = sid++; //Square id [0:64)
    }
  }


  //Let's see whose turn is now ('w'/'b')
  if(*fen == 'w')
    g->color = WHITE;
  else if (*fen == 'b')
    g->color = WHITE;
  else {
    fprintf(stderr, "Error: Wrong Color in FEN Format");
    fflush(stderr);
    exit(-1);
  }

  ++fen;
  assert(*fen == ' '); 

  ++fen;
  g->castling = 0;
  while(*fen != '\0') {
    char c = *fen++;
    if (c == '-') 
      ++fen;
    if (c == ' ')
      break;
    if (c == 'k')
      g->castling |= 1;
    if (c == 'q')
      g->castling |= 2;
    if (c == 'K')
      g->castling |= 4;
    if (c == 'Q')
      g->castling |= 8;
  }

  g->enpassante = 0;
  if(*fen != '-'){
    g->enpassante = SQUARE64(fen);
    ++fen;
  }

  ++fen; 
  assert(*fen == ' ');

  ++fen;
  assert(isdigit(*fen));
  g->halfclock = *fen - '0'; 
  //Halfmove clocks are reset during a capture/a pawn advance

  ++fen;
  assert(*fen == ' ');

  ++fen;
  assert(isdigit(*fen));
  g->fullclock = *fen - '0'; 

  ++fen;
  assert(*fen == '\0');

  return;
}

void GamePrintFEN(_Game * g){
  char * fen = g->fen;
  fprintf(stdout, "\n");
  for (int i=0; fen[i] != '\0'; ++i, ++fen){
      fprintf(stdout, "%c", *fen);
  }
}

void GameFEN(char * fen, unsigned char ** board){
  /* Get FEN from Board */
}

_Game * Game(char * _fen){

  //Game instance
  _Game * g = (_Game *) malloc (sizeof (_Game));

  //Set Current FEN
  char _fen0[] = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
  char * fen = _fen ? _fen : _fen0;
  g->fen[0] = '\0';
  for (int i=0; fen[i] != '\0'; ++i){
    g->fen[i] = fen[i];
    g->fen[i+1] = '\0';  
    if(i == 100){
      fprintf(stderr, "Error: Very Long FEN");
      fflush(stderr);
      exit(-1);
    }
  }

  //2-dimensional 8x8 board with 2 layer padding on each sides
  //board[-2:9][-2:9]
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
  g->board = board;
  //Set the Chessboard
  GameBoard(g);

  //Allocate memory for possible moves,
  g->moves = array_new();
  g->move  = NULL;
 
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

  free(g);
}

int GameAllMoves(_Game * g){
  /* Find all moves by rule. 
   If on check and no moves return 0. Game over 
   If on check and there are atleast 1 move which nullify check.
   If Not on check and there are no moves return 2. Stalemate.
   If Not on check and there are some moves return 3 */ 
  
}

int GameMove(_Game * g){

  //move
  _GameSquare * board = g->board[0];
  _GameMove * m = g->move;
  if(!m) {
    fprintf(stderr, "Error: Move not found");
    fflush(stdout);
    exit(-1);
  }
  //board[m->to] = (m->flags & MOVE_PROMOTION) ?
  //  m->promotion : m->piece;

  m = NULL;
   
  //update_flags
  

  GameAllMoves(g);
  if(!g->moves->len) {
    fprintf(stdout, "Game Over!. %c wins",
      g->color ? 'w' : 'b');
  }
  else {
    g->fullclock += (!g->color);
    // Swap Color
    g->color = !g->color;
    //update halfmove if CAPTURE or pawnmove
  }
} 

int main(){
  _Game * g = Game(NULL);
  GamePrintBoard(g);

  _GameSquare * from = &(g->board[7][1]);
  fprintf(stdout, "\n from: %c%c%c piece \n",
    BOARD_PIECE(*from), BOARD_FILE(*from), BOARD_RANK(*from));

  GameMovesFrom(from, KNIGHT_MOVES, 8, 1, g->moves);

  GameDestroy(g);

  return 0;
}
