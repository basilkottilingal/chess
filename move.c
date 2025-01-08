#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>

#define WHITE 1
#define BLACK 0

#define BPAWN (1<<1)
#define WPAWN ((1<<1) | WHITE)
#define BROOK (2<<1) 
#define WROOK ((2<<1) | WHITE)
#define BKNIGHT (3<<1) 
#define WKNIGHT ((3<<1) | WHITE)
#define BBISHOP (4<<1) 
#define WBISHOP ((4<<1) | WHITE)
#define BQUEEN (5<<1) 
#define WQUEEN ((5<<1) | WHITE)
#define BKING (6<<1) 
#define WKING ((6<<1) | WHITE)
#define PIECE_COLOR(PIECE) (PIECE & WHITE)

#define MOVE_NORMAL 1
#define MOVE_CAPTURE 2
#define MOVE_PROMOTION 4
#define MOVE_KCASTLE 8
#define MOVE_QCASTLE 16
#define MOVE_ENPASSANTE 32

const int BPAWN_MOVES[4][3] = {
    {1,0}, {2,0}, {1,-1}, {1,1}
  };
const int WPAWN_MOVES[4][3] = {
    {-1,0}, {-2,0}, {-1,-1}, {-1,1}
  };

static inline unsigned int SQUARE64(char * s){
  return ( 8 * (8 - s[1] + '0' ) + s[0] - 'a');
}

//define VALIDMOVE(SQUARE,VECTOR) 

// Chesspieces: For faster translation b/w ..
// .. chesspieces' usual notation and thier number notation 
char MAPPING[14] = 
  { '.', '.',
    'p', 'P', 'r', 'R', 'n', 'N',
    'b', 'B', 'q', 'Q', 'k', 'K' 
  };

unsigned int MAPPING2[58] = 
  { 'A', WBISHOP, 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 
    WKING, 'L', 'M', WKNIGHT, 'O', WPAWN, WQUEEN, 
    WROOK, 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 
    ' ', ' ', ' ', ' ', ' ', ' ', 'a', BBISHOP, 'c', 'd', 
    'e', 'f', 'g', 'h', 'i', 'j', BKING, 'l', 'm', 
    BKNIGHT, 'o', BPAWN, BQUEEN, BROOK, 's', 't', 
    'u', 'v', 'w', 'x', 'y', 'z'
  };

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
  unsigned int piece;
  unsigned int from;
  unsigned int to;
  char SAN[12];
  unsigned int flags;
  unsigned int promotion;
}_GameMove;

typedef struct {
  unsigned int ** board;
  unsigned int enpassante, castling, color;
  unsigned int halfclock, fullclock;
  char ** history;
  char fen[101];
  Array * moves;
  void * move;
}_Game;

void GamePushHistory(_Game * g){
  /** Add current FEN to history */
}

void GamePopHistory(_Game * g){
  /** Remove last move from history. ..
  .. In case of reverting a move */
}

void GamePrintBoard(_Game * g) {
  unsigned int ** board = g->board;
      
  fprintf(stdout,"\nBoard");
  for (int i=0; i<8; ++i){
  fprintf(stdout,"\n");
    for (int j=0; j<8; ++j) {
      unsigned int piece = board[i][j];
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
  unsigned int ** rank = g->board;
  unsigned int * square = rank[0];
  char * fen = g->fen;
  while(*fen != '\0') {
    char c = *fen;
    ++fen;
    /* Empty squares */ 
    if (isdigit(c)) {
      int nempty = c - '0';
      assert(nempty > 0 && nempty <= 8);
      for (int i=0; i<nempty; ++i)
        *square++ = 0;
    }
    else if (c == '/') {
      //check all squares of this rank are covered
      size_t nfiles = (size_t) (square - rank[0]);
      assert(nfiles == 8);
      ++rank;
      square = rank[0];
    } 
    else if (c == ' '){
      //check all ranks are covered
      //size_t nranks = (size_t) (rank - g->board[0]);
      //assert(nranks == 8);
      break;
    }
    else 
      *square++ = MAPPING2[c - 'A'];
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

void GameFEN(char * fen, unsigned int ** board){
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
  unsigned int ** board = (unsigned int **)
    malloc(tb*sizeof(unsigned int *));
  board[0] = (unsigned int *)
    malloc(tb*tb*sizeof(unsigned int));
  for(int i=1; i<tb; ++i)
    board[i] = board[i-1] + tb;
  for(int i=0; i<tb; ++i)
    board[i] += p;
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
  unsigned int ** board = g->board;
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
  unsigned int * board = g->board[0];
  _GameMove * m = g->move;
  if(!m) {
    fprintf(stderr, "Error: Move not found");
    fflush(stdout);
    exit(-1);
  }
  board[m->to] = (m->flags & MOVE_PROMOTION) ?
    m->promotion : m->piece;

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

  GameDestroy(g);

  return 0;
}
