#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include <math.h>
#include <stdint.h>

#define NOT_UNUSED(x) (void)(x)

// Square is either empty ('0')
// .. Or occupied by a piece '2' : '13'
// .. Outside ('15') the box
#define OUTSIDE 64
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

//Datatpes.To avoid unwarned type cast
typedef uint8_t Square;
typedef uint8_t Piece;
typedef uint8_t Flag;
//typedef unsigned int Lflag;
// Chesspieces: For faster translation b/w ..
// .. chesspieces' usual notation and thier number notation 
const char MAPPING[16] = 
  { '.', '.',
    'p', 'P', 'r', 'R', 'n', 'N',
    'b', 'B', 'q', 'Q', 'k', 'K',
    '.', 'x' 
  };

const Piece MAPPING2[58] = 
  { 'A', WBISHOP, 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 
    WKING, 'L', 'M', WKNIGHT, 'O', WPAWN, WQUEEN, 
    WROOK, 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 
    ' ', ' ', ' ', ' ', ' ', ' ', 'a', BBISHOP, 'c', 'd', 
    'e', 'f', 'g', 'h', 'i', 'j', BKING, 'l', 'm', 
    BKNIGHT, 'o', BPAWN, BQUEEN, BROOK, 's', 't', 
    'u', 'v', 'w', 'x', 'y', 'z'
  };
/* Input ascii and retrieve a square (in [0:63))
   or a piece (in [0:15)) */
static inline Square BoardSquareParse(char * s){
  return ( 8 * (8 - s[1] + '0' ) + s[0] - 'a' );
}
static inline Piece PieceParse(char p) {
  if (p < 'A' || p > 'z') 
    return EMPTY;
  Piece piece = MAPPING2[p- 'A'];
  if (piece > 15) 
    return EMPTY;
  return piece; // valid piece
}
//Squares [-2:9]x[-2:9] with [0:7]x[0:7] is inside
Square ** GAMEBOARD = NULL; 
Piece * PIECES = NULL;    //store pieces/empty square
//Below: A square "S" is a pointer whose content is in [0,64]
#define IS_OUTSIDE(S)   ((*S) == OUTSIDE)
#define SQUARE_PIECE(S) (IS_OUTSIDE(S) ? 16 : PIECES[*S])
#define PIECE_COLOR(S)  (SQUARE_PIECE(S) & 1)
#define SQUARE_FILE(S)  ('a' + (*S)%8)
#define SQUARE_RANK(S)  ('0' + 8 - (*S)/8)
#define PIECE_ASCII(S)  (MAPPING[SQUARE_PIECE(S)])
#define IS_EMPTY(S)     (SQUARE_PIECE(S) == EMPTY)
#define IS_PIECE(S)     (!IS_OUTSIDE(S) && !IS_EMPTY(S))
static inline Square * SquarePointer(Square s) {
  //assert(s<=OUTSIDE);
  return &(GAMEBOARD[s/8][s%8]);
}

// 8 bits of the flags (unsigned char)
#define MOVE_NORMAL     0
#define MOVE_CAPTURE    1
#define MOVE_PROMOTION  2
#define MOVE_ENPASSANTE 4
#define MOVE_kCASTLE    8
#define MOVE_KCASTLE   16
#define MOVE_qCASTLE   32
#define MOVE_QCASTLE   64
#define MOVE_CHECK    128

typedef struct {
  //64 pieces corresponding to each square on GAMEBOARD[][]
  Piece pieces[64];
  /** -----------Game Status Metadata--------------*/
  //squares to store WKING, BKING, ENPASS \in [0,63)
  Square king[2], enpassante;
  //some flags: castling available?,  game on check?
  Flag castling, check;
  //numbers (guaranteed below 51, 5000, 33 respectively)
  uint16_t halfclock, fullclock, npieces;
  //whose turn  
  Flag color;
  //Game Status; 
  Flag status; 
}_Board;

void BoardCopy(_Board * b, _Board * source){
  memcpy (b, source, sizeof(_Board));
}

void BoardInitIterator(){
  if(GAMEBOARD) 
    return;
    
  // Allocate mem for 2-D GAMEBOARD (8x8) with ..
  // .. 2 layer padding on each sides (12x12).
  // accessible: squareBoard[-2:9][-2:9]
  // Valid part: squareBoard[0:7][0:7]
  int b = 8, p = 2;
  int tb = b + 2*p;
  Square ** squareBoard = (Square **)
    malloc(tb*sizeof(Square *));
  squareBoard[0] = (Square *)
     malloc(tb*tb*sizeof(Square));
  for(int i=1; i<tb; ++i)
    squareBoard[i] = squareBoard[i-1] + tb;
  for(int i=0; i<tb; ++i)
    squareBoard[i] += p;
  squareBoard += p;
  for(int r=-2;r<10; ++r)
    for(int f=-2; f<10; ++f) {
      squareBoard[r][f] = (r<0 || r>=8 || f<0 || f>=8) ?
        OUTSIDE : (Square)  (8*r + f);
    }
  GAMEBOARD = squareBoard;
}

Square ** BoardMakeAvailable(_Board * b){
  PIECES = b->pieces; 
  assert(GAMEBOARD);
  return GAMEBOARD; 
}

Flag BoardSetFromFEN(_Board * b, char * fen){
  b->npieces = 0; 
  b->king[WHITE] = OUTSIDE; 
  b->king[BLACK] = OUTSIDE;

  /* Set board from FEN */
  Piece * piece = b->pieces;
  Square square = 0; // square id
  while(*fen != '\0') {

    if( !(square <= OUTSIDE) )
      return 0;
    char c = *fen;
    ++fen;
    /* Empty squares */ 
    if (isdigit(c)) {
      int nempty = c - '0';
      if ( !(nempty > 0 && nempty <= 8) )
        return 0; 
      for (int i=0; i<nempty; ++i) { 
        *piece++ = EMPTY;
        ++square;
      }
    }
    else if (c == '/') {
      //Make sure all squares of this rank are filled
      if( !(square%8 == 0 ) )
        return 0;
    }
    else if (c == ' '){
      //Make sure all squares are filled
      if( !( square == OUTSIDE ) ) 
        return 0;
      break;
    }
    else{
      if (!(c == 'p' || c == 'P' ||  c == 'b' || c == 'B' ||
            c == 'n' || c == 'N' ||  c == 'r' || c == 'R' ||
            c == 'q' || c == 'Q' ||  c == 'k' || c == 'K') )
      { 
          return 0; 
      }
      // occupied square
      *piece++ =  PieceParse(c); //MAPPING2[c - 'A']; 
      if(c == 'K' || c == 'k') {
        Flag color = (c == 'K') ? WHITE : BLACK;
        //There cannot be multiple kings of same color
        if ( !(b->king[color] == OUTSIDE) )
          return 0;
        b->king[color] = square; //the king is here
      }
      else {
        //occupied by a piece other than 'kings'
        if ( !((b->npieces)++ < 30) )
          return 0;
      }
      ++square;
    }
  }
  //Make sure that there are both 'k' and 'K' in the FEN;
  for(Flag c=0; c<2; ++c)
    if(b->king[c] == OUTSIDE)
      return 0;

  //Let's see whose turn is now ('w'/'b')
  b->color = (*fen == 'w') ? WHITE : BLACK;
  if( !(*fen == 'w' || *fen == 'b') )
    return 0;
  if( (*(++fen) != ' ') )
    return 0;

  b->castling = '\0';
  while(*(++fen) != '\0') {
    char c = *fen;
    if (c == ' ')
      break;
    if (c == 'k')
      b->castling |= MOVE_kCASTLE;
    if (c == 'q')
      b->castling |= MOVE_qCASTLE;
    if (c == 'K')
      b->castling |= MOVE_KCASTLE;
    if (c == 'Q')
      b->castling |= MOVE_QCASTLE;
  }
  if (*fen++ != ' ')
    return 0;

  b->enpassante = OUTSIDE+1;
  if(*fen != '-'){
    b->enpassante = BoardSquareParse(fen);
    ++fen;
  }
  if(*(++fen) != ' ')
    return 0;

  //Halfmove clocks are reset during a capture/a pawn advance
  b->halfclock = 0;
  while(*(++fen) != '\0'){
    char c = *fen;
    if( c == ' ' )
      break;
    if( !(isdigit(c)) )
      return 0;
    b->halfclock = 10*b->halfclock + (uint16_t) (c - '0');
    if (! (b->halfclock <= 50) )
      return 0;
  }
  if(*fen != ' ')
    return 0;

  b->fullclock = 0;
  while(*(++fen) != '\0'){
    char c = *fen;
    if( !(isdigit(c)) )
      return 0;
    b->fullclock = 10*b->fullclock + (uint16_t) (c - '0');
    // No recorded FIDE game exceeded 300 moves.
    // .. I don't know the theoretical limit. 
    // .. I think draw (by 50moves rule) would have ..
    // .. occured before 1000 moves??
    if (! (b->fullclock <= 10000) )
      return 0;
  }
  if( !(b->fullclock) ) //has to be greater than 0;
    return 0;
  if(*fen != '\0')
    return 0;

  return 1; // succesful
}

void BoardFEN(_Board * b, char * fen) {
  Piece * piece = b->pieces;
  unsigned char nempty;
  //traverse through board
  for(int i=0; i<8; ++i) {
    nempty = 0;
    for (int j=0; j<8; ++j) {
      if(*piece) {
        if(nempty) {
          *fen++ = '0' + nempty;
          nempty = 0;
        }
        *fen++ = MAPPING[*piece];
      }
      else
        nempty++;
      //piece in the next square
      piece++;
    }
    if(nempty)
      *fen++ = '0' + nempty;
    *fen++ = i<7 ? '/' : ' '; 
  }
  
  //color
  *fen++ = b->color ? 'w' : 'b';
  *fen++ = ' ';

  //Castling
  if(!b->castling)
    *fen++ = '-';
  else{
    if(b->castling & MOVE_KCASTLE)
      *fen++ = 'K';
    if(b->castling & MOVE_QCASTLE)
      *fen++ = 'Q';
    if(b->castling & MOVE_kCASTLE)
      *fen++ = 'k';
    if(b->castling & MOVE_qCASTLE)
      *fen++ = 'q';
  }
  *fen++ = ' ';

  //enpassante
  if(b->enpassante == OUTSIDE+1)
    *fen++ = '-';
  else {  
    *fen++ = 'a' + (b->enpassante)%8;
    *fen++ = '0' + 8 - (b->enpassante)/8;
  }
  *fen++ = ' ';

  //clocks
  uint16_t gameclock[2] = {b->halfclock, b->fullclock};
  for(int i=0; i<2; ++i) {
    uint16_t n = gameclock[i], pos = 4;
    //otherwise: weird clocknumbers
    assert(n <= (i ? 5000 : 50)); 
    unsigned char h[5];
    h[pos] = i ? '\0' : ' ';
    while(pos) {
      h[--pos] = '0' + n%10;
      n /= 10;   
      if(!n) break;
    }
    for(int j=pos; j<5; ++j)
      *fen++ = h[j];
  }
}

_Board * Board(_Board * source) {
  /* GAMEBOARD is a bit different from "board". .. 
  .. GAMEBOARD is more like an iterator. It's global for ..
  .. any game, and help through each ..
  .. square ( = GAMEBOARD[r][c])  of the board ..
  .. and can be used to find the piece = (PIECES[square]) */
  if(!GAMEBOARD)
    BoardInitIterator();
     
  _Board * board = (_Board *) malloc (sizeof (_Board));
  if(source) 
    //Copy in case there is a source specified 
    BoardCopy(board, source);
  
  return board;
}

void BoardDestroy(_Board * b){
  free(b);
}

void BoardPrint(_Board * board){
  /* ASCII Board */
  Piece * piece = board->pieces;
  fprintf(stdout,"\n        BOARD");
  for (int i=0; i<8; ++i){
    fprintf(stdout,"\n %c ", '0'+8-i);
    for (int j=0; j<8; ++j) 
      fprintf(stdout," %c", MAPPING[*piece++]);
  }
    
  fprintf(stdout,"\n\n   ");
  for (int j=0; j<8; ++j) 
    fprintf(stdout," %c", 'a'+j);
  fprintf(stdout,"\n");
}

/*------------------------------------------------------------
conditions to check while moving piece from 'FROM' to 'TO'
--------------------------------------------------------- */
#define IS_NORMAL(FROM,TO)  ( IS_EMPTY(TO) )
#define IS_BLOCKED(FROM,TO) ( IS_PIECE(TO) && \
  (PIECE_COLOR(FROM) == PIECE_COLOR(TO)) )
#define IS_CAPTURE(FROM,TO) ( IS_PIECE(TO) && \
  (PIECE_COLOR(FROM) != PIECE_COLOR(TO)) )
#define IS_PROMOTION(FROM,TO)\
  ( (( SQUARE_PIECE(FROM) == WPAWN) && \
     (SQUARE_RANK(TO) == '8')) || \
    (( SQUARE_PIECE(FROM) == BPAWN) && \
     (SQUARE_RANK(TO) == '1')) )
#define IS_ENPASSANTE(FROM,TO,BOARD) \
  ( (SQUARE_PIECE(FROM) == WPAWN && SQUARE_RANK(FROM) == '5' \
      && (*TO) == (BOARD)->enpassante ) ||\
    (SQUARE_PIECE(FROM) == BPAWN && SQUARE_RANK(FROM) == '4' \
      && (*TO) == (BOARD)->enpassante ) )
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


/* ---------------------------------------------------------
------------------------------------------------------------
  The function
    BoardMove(b, move);
  .. temporarily updates the board of the game "g" with ..
  .. the move "move" (i.e. only board is updated, move ..
  .. is not finalised.).
  And the function
    BoardUnmove(b, move);
  .. undo the effect of BoardMovePiece(g,move);
------------------------------------------------------------
--------------------------------------------------------- */
typedef struct {
  Piece piece;
  Square square;
}_BoardSquare;

typedef struct {
  /* In case of promotion (flags & MOVE_PROMOTION == 1), 
  .. "promotion" will store the promoted piece */
  _BoardSquare from, to;
  Flag flags;
  Piece promotion;
  //char SAN[8];
}_BoardMove;

Flag BoardMoveCompare(_BoardMove * m, _BoardMove * M) {
  //Careful: doesn't compare all fields.
  //It works only if _Board is same.
  Flag isSame = 
    (m->from.square == M->from.square  &&
     m->to.square == M->to.square) ? 1 : 0;
  isSame &= 
    !(m->flags & MOVE_PROMOTION) ? 1 : 
    (m->promotion == M->promotion);
    
  return isSame;
}

char* BoardMoveSAN (_BoardMove * m) {
  //fixme: Not et impelemented 
  assert(0);
  NOT_UNUSED(m);
  return "\0";
}

enum GAME_STATUS{
  GAME_CONTINUE = 0,      // Game continues;
  GAME_IS_A_WIN = 16,     // One wins
  GAME_IS_A_DRAW = 32,    // Draws
  /* Intermediate stages of board.*/
  GAME_METADATA_NOTUPDATED = 64, //
  GAME_STATUS_NOTUPDATED = 64|128, //
  /* Encode unknown error*/
  GAME_STATUS_ERROR = 128,
  /* Info on WIN */
  GAME_WHO_WINS = 1,
  GAME_IS_WON_BY_TIME = 2,
  GAME_IS_WON_BY_FORFEIT = 4,
  /* Info on draw = (STATS & GAME_DRAW_INFO)*/
  GAME_DRAW_INFO = 15,
  GAME_STALEMATE = 0,
  GAME_INSUFFICIENT = 1,
  GAME_FIFTY_MOVES = 2,
  GAME_THREE_FOLD = 3,
  GAME_WHITE_CANNOT = 4,
  GAME_BLACK_CANNOT = 5,
  GAME_AGREES = 6
};
 
void BoardMove(_Board * b, _BoardMove * move){

  b->status |= GAME_METADATA_NOTUPDATED;
  assert(move);
  Square from = move->from.square,
    to = move->to.square;
  Piece * pieces = b->pieces;

  
  if(pieces[from] != move->from.piece) {
    BoardPrint(b);
    Square * A = SquarePointer(from);
    Square * B = SquarePointer(to);
    fprintf(stdout, "\n %c%c %c/%c -> %c%c %c",
      SQUARE_FILE(A), SQUARE_RANK(A), 
      MAPPING[pieces[from]], MAPPING[move->from.piece],
      SQUARE_FILE(B), SQUARE_RANK(B), MAPPING[move->to.piece]);
    fflush(stdout);
  }
  
  assert(pieces[from] == move->from.piece);
  
  pieces[from] = EMPTY;
  pieces[to]   = (move->flags & MOVE_PROMOTION) ? 
    move->promotion : move->from.piece;

  if(pieces[to] == WKING) {
    b->king[WHITE] = to;
    if(move->flags & MOVE_QCASTLE) {
      pieces[56] = EMPTY;
      pieces[59] = WROOK;
      return;
    }
    if(move->flags & MOVE_KCASTLE) {
      pieces[63] = EMPTY;
      pieces[61] = WROOK;
      return;
    }
  }
  else if(pieces[to] == BKING) {
    b->king[BLACK] = to;
    if(move->flags & MOVE_qCASTLE) {
      pieces[0] = EMPTY;
      pieces[3] = BROOK;
      return;
    }
    if(move->flags & MOVE_kCASTLE) {
      pieces[7] = EMPTY;
      pieces[5] = BROOK;
      return;
    }
  }

  if(move->flags & MOVE_ENPASSANTE) { 
    //assert(b->enpassante == to);
    pieces[to + (b->color ? 8 : -8)] = EMPTY;
  }
  
} 
  
void BoardUnmove(_Board * b, _BoardMove * move){
  
  //assert(b->status == GAME_METADATA_NOTUPDATED);
  b->status &= ~GAME_METADATA_NOTUPDATED;
  assert(move);
  Square from = move->from.square,
    to = move->to.square;
  Piece * pieces = b->pieces;

  pieces[from] = move->from.piece;
  pieces[to]   = move->to.piece;

  if(pieces[from] == WKING) {
    b->king[WHITE] = from;
    if(move->flags & MOVE_QCASTLE) {
      pieces[56] = WROOK;
      pieces[59] = EMPTY;
      return;
    }
    if(move->flags & MOVE_KCASTLE) {
      pieces[63] = WROOK;
      pieces[61] = EMPTY;
      return;
    }
  }
  else if(pieces[from] == BKING) {
    b->king[BLACK] = from;
    if(move->flags & MOVE_qCASTLE) {
      pieces[0] = BROOK;
      pieces[3] = EMPTY;
      return;
    }
    if(move->flags & MOVE_kCASTLE) {
      pieces[7] = BROOK;
      pieces[5] = EMPTY;
      return;
    }
  }
   
  if(move->flags & MOVE_ENPASSANTE) {
    //assert(b->enpassante == to); 
    pieces[to + (b->color ? 8 : -8)]
      = b->color ? BPAWN : WPAWN;
  }
} 
