#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>
#include <limits.h>
#include <time.h>

/* switch off en passante,
reset half clock.
When can a game draw
  Stalemate:	Player has no legal moves but is not in check.
  Insufficient Material:	Neither player can checkmate with the pieces remaining.
  Threefold Repetition:	The same position appears three times.
  Fifty-Move Rule:	50 moves without a pawn move or capture.
  Agreement:	Players agree to end the game as a draw.
  Dead Position:	No possible moves can lead to checkmate.
  Draw by Time: A player's time runs out, but their opponent cannot deliver checkmate.
*/

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
  { -11, -13, -12, -24}; 
const char BPAWN_MOVES[4] = 
  { 11, 13, 12, 24};   

static inline unsigned char SQUARE64(char * s){
  return ( 8 * (8 - s[1] + '0' ) + s[0] - 'a' );
}

// 8 bits of the flags (unsigned char)
#define MOVE_NORMAL 0
#define MOVE_CAPTURE 1
#define MOVE_PROMOTION 2
#define MOVE_ENPASSANTE 4
#define MOVE_KCASTLE 8
#define MOVE_QCASTLE 16
#define MOVE_kCASTLE 32
#define MOVE_qCASTLE 64
#define MOVE_CHECK 128

//Identifying rank, file, piece color
#define PIECE_COLOR(SQUARE) ((SQUARE).piece & 1)
#define BOARD_FILE(SQUARE)  ('a' + (SQUARE).square%8)
#define BOARD_RANK(SQUARE)  ('0' + 8 - (SQUARE).square/8)
#define BOARD_PIECE(SQUARE) (MAPPING[(SQUARE).piece])

//Some conditions
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
  /* In case of promotion (flags & PROMOTION == 1), 
  .. to.piece will store the promoted piece */
  _GameSquare from, to;
  unsigned char flags, promotion;
  char SAN[8];
}_GameMove;

typedef struct {
  _GameSquare ** board;  //8x8 board with padding
	_GameSquare * king[2]; //location of kings
  unsigned char enpassante, castling, color;
  unsigned char halfclock, fullclock;
  char fen[FEN_MAXSIZE];
  Array * moves, * history;
  _GameMove * move;
}_Game;

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

// Returns 1, if the square "sq"(of color "color") ..
// .. is attacked by the piece in "from". Returns 0, otherwise
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
        //assert(!IS_BLOCKED (*from,*to));
          //Cannot be attacked by same color.
        return 1;       //yes, the square "sq" is attacked.
      }
      if(!IS_EMPTY(*to))
        break;          //Blocked by another piece
    }
  }
  return 0; // the square "sq" is safe from an attack
}

int GameIsKingAttacked(_Game * g, unsigned char color) {
	_GameSquare * king = g->king[color];   
  _GameSquare ** board = g->board;
  for (int i=0; i<8; ++i)
    for(int j=0; j<8; ++j) {
      _GameSquare * from = &(board[i][j]);
      if( IS_EMPTY(*from) )
  continue; //empty
      if( PIECE_COLOR(*from) != color )
  continue; //Occupied by the other color
      //Generate possible moves with the 'piece' 
      //GamePieceMoves[from->piece](g, from);
    }
}

void GameQueenMoves(_Game * g, _GameSquare * from){
  GameMovesFrom(from, QUEEN_MOVES, 8, 7, g->moves); 
}

void GameKingMoves(_Game * g, _GameSquare * from){
  GameMovesFrom(from, QUEEN_MOVES, 8, 1, g->moves);
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
    if(!flags)
      continue;
    flags |= IS_PROMOTION(*from, *to) ? MOVE_PROMOTION : 0;
fprintf(stdout,"--p%d, %d,%d,%u",to->piece, IS_PIECE(*to),
PIECE_COLOR(*from) != PIECE_COLOR(*to),flags);
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
    if(from->square/8 != (g->color ? 6 : 1))
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

static inline void GamePrintFEN(_Game * g){
  fprintf(stdout, "\n%s", g->fen);
}

void GamePrintBoard(_Game * g, int persist) {
  _GameSquare ** board = g->board;
  
  //if persist. It clears the window
  if(persist) {
    printf("\033[2J");          // Clear the screen
    printf("\033[1;1H");        //Cursor on the left to
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

void GameBoard(_Game * g, char * _fen) {
  //Set Current FEN
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
				if(square->piece == WKING)
					g->king[WHITE] = square;
				else if(square->piece == BKING)
					g->king[BLACK] = square;
        square++->square = sid++; //Square id [0:64)
    }
  }


  //Let's see whose turn is now ('w'/'b')
  if(*fen == 'w')
    g->color = WHITE;
  else if (*fen == 'b')
    g->color = BLACK;
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

  g->enpassante = 64;
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
  GameBoard(g, fen); 
}

void GameFEN(_Game * g){
  /* Get FEN from Board */
}

_Game * Game(char * fen){

  //Game instance
  _Game * g = (_Game *) malloc (sizeof (_Game));

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
  g->board = board;

  //Allocate memory for possible moves,
  g->moves = array_new();
  g->move  = NULL;

  //Allocate memory for game history.
  g->history = array_new();

  //Set the Chessboard
  GameBoard(g, fen);
 
  return g; 
  
} 

void GameDestroy(_Game * g) {
  _GameSquare ** board = g->board;
  int p = 2;
  board -= p;
  board[0] -= p; 
  free(board[0]);
  free(board);

  array_free(g->moves);
  array_free(g->history);

  free(g);
}
 
  
void GameMovePiece(_Game * g, _GameMove * move){
  assert(move);

  unsigned char f = move->from.square,
    t = move->to.square;
  _GameSquare ** board = g->board;
  _GameSquare * from = &(board[f/8][f%8]);
  _GameSquare * to   = &(board[t/8][t%8]);
  from->piece = EMPTY;
  to->piece = (move->flags & MOVE_PROMOTION) ? 
    move->promotion : move->from.piece;
  if(move->flags <= MOVE_CAPTURE)
    return; //In case of normal move, or just capture
  if(move->flags & MOVE_ENPASSANTE) {
    _GameSquare * en = to + (g->color ? 12 : -12); 
    en->piece = EMPTY;
    return;
  }
  //FIXME: All te four conditions may be compressed
  if(move->flags & MOVE_QCASTLE) {
    board[7][0].piece = EMPTY;
    board[7][3].piece = WROOK;
    return;
  }
  if(move->flags & MOVE_qCASTLE) {
    board[0][0].piece = EMPTY;
    board[0][3].piece = BROOK;
    return;
  }
  if(move->flags & MOVE_KCASTLE) {
    board[7][7].piece = EMPTY;
    board[7][5].piece = WROOK;
    return;
  }
  if(move->flags & MOVE_kCASTLE) {
    board[0][7].piece = EMPTY;
    board[0][5].piece = BROOK;
    return;
  }
} 
  
void GameUnmovePiece(_Game * g, _GameMove * move){
  assert(move);

  unsigned char f = move->from.square,
    t = move->to.square;
  _GameSquare ** board = g->board;
  _GameSquare * from = &(board[f/8][f%8]);
  _GameSquare * to   = &(board[t/8][t%8]);
  to->piece = move->to.piece;
  from->piece = move->from.piece;
  if(move->flags <= MOVE_CAPTURE)
    return; //In case of normal move, or just capture
  if(move->flags & MOVE_ENPASSANTE) {
    _GameSquare * en = to + (g->color ? 12 : -12); 
    en->piece = g->color ? BPAWN : WPAWN;
    return;
  }
  //FIXME: All te four conditions may be compressed
  if(move->flags & MOVE_QCASTLE) {
    board[7][3].piece = EMPTY;
    board[7][0].piece = WROOK;
    return;
  }
  if(move->flags & MOVE_qCASTLE) {
    board[0][3].piece = EMPTY;
    board[0][0].piece = BROOK;
    return;
  }
  if(move->flags & MOVE_KCASTLE) {
    board[7][5].piece = EMPTY;
    board[7][7].piece = WROOK;
    return;
  }
  if(move->flags & MOVE_kCASTLE) {
    board[0][5].piece = EMPTY;
    board[0][7].piece = BROOK;
    return;
  }
} 

int GameAllMoves(_Game * g){
  /* Find all moves by rule. 
   If on check and no moves return 0. Game over 
   If on check and there are atleast 1 move which nullify check.
   If Not on check and there are no moves return 2. Stalemate.
   If Not on check and there are some moves return 3 */
  
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
  
  int nmoves =  (int) (moves->len /sizeof(_GameMove));
  _GameMove * m = (_GameMove *) (g->moves->p);
  for(int i=0; i<nmoves; ++i, ++m) {
    clock_t start_time = clock();
    clock_t wait_time = 0.4*CLOCKS_PER_SEC ; //5s 
    while (clock() - start_time < wait_time) {};
    GamePrintBoard(g,1);

    GameMovePiece(g, m);

    start_time = clock();
    while (clock() - start_time < wait_time) {};
    GamePrintBoard(g,1);

    GameUnmovePiece(g, m);
  }

  if(!moves->len) {
    //Game Over
  }
    
}

int GameUpdateMove(_Game * g){

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

/*Sample FEN's for verifying
1) Fool's Mate (Black Checkmates White)
r1bqkbnr/pppp1ppp/2n5/4p3/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - 0 2
2) Promotable white pawn
8/P7/8/8/8/8/8/k6K w - - 0 1
3) En passante 
rnbqkbnr/ppp1pppp/8/3pP3/8/8/PPPP1PPP/RNBQKBNR b KQkq d6 0 2
rnbqkbnr/1pp1pppp/8/p2pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 3
*/

int main(){
  //_Game * g = Game(NULL);
  _Game * g = Game("8/P7/8/8/8/8/8/k6K w - - 0 1");
  //_Game * g = Game("r1bqkbnr/pppp1ppp/2n5/4p3/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - 0 2");
  //_Game * g = Game("rnbqkbnr/1pp1pppp/8/p2pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 3");
  GamePrintBoard(g, 0);

  //_GameSquare * from = &(g->board[7][1]);
  //GameMovesFrom(from, KNIGHT_MOVES, 8, 1, g->moves);

  GameAllMoves(g);
   
  GamePrintBoard(g, 1);

  GameDestroy(g);

  return 0;
}
