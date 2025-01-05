#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>

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
#define PIECE_TYPE(PIECE) (PIECE & 30)

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

typedef struct {
  unsigned int ** board;
  unsigned int enpassante, castling, color;
  unsigned int nmove;
  char ** history;
  char fen[101];
}_Game;

void GamePushHistory(_Game * g){
  /** Add current FEN to history */
}

void GamePopHistory(_Game * g){
  /** Remove last move from history. ..
  .. In case of reverting a move */
}

void GamePrintBoard(_Game * g) {
  unsigned int * pieces = g->board[0];
      
  fprintf(stdout,"\nBoard");
  for (int i=0; i<8; ++i){
  fprintf(stdout,"\n");
    for (int j=0; j<8; ++j) 
      fprintf(stdout," %c", MAPPING[*pieces++]);
  }
  fprintf(stdout,"\nWhose Turn: %u", g->color);
  fprintf(stdout,"\nCastling: %u", g->castling);
  fprintf(stdout,"\nEnpass: %u", g->enpassante);
}

void GameBoard(_Game * g) {
  /* Set board from FEN */ 
  unsigned int * square = g->board[0];
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
    else if (c == '/') 
      continue; 
    else if (c == ' ')
      break;
    else 
      *square++ = MAPPING2[c - 'A'];
  }

  size_t nsquares = (size_t) (square - g->board[0]);
  if(nsquares != 64) {
    fprintf(stderr, "Error: Wrong FEN Format (1)");
    fflush(stderr);
    exit(-1);
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

}

void GamePrintFEN(_Game * g){
  char * fen = g->fen;
  fprintf(stdout, "\n", *fen);
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

  //two dimensional board
  unsigned int ** board = 
    (unsigned int **)malloc(8*sizeof(unsigned int *));
  board[0] = (unsigned int *)malloc(64*sizeof(unsigned int));
  for(int i=1; i<8; ++i)
    board[i] = board[i-1] + 8;
  g->board = board;
  //Set the Chessboard
  GameBoard(g);
 
  return g; 
  
} 

void GameDestroy(_Game * g) {
  free(g->board[0]);
  free(g->board);
  free(g);
}

int main(){
  _Game * g = Game(NULL);
  GamePrintBoard(g);

  GameDestroy(g);

  return 0;
}
