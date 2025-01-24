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

// Chesspieces: For faster translation b/w ..
// .. chesspieces' usual notation and thier number notation 
const unsigned char MAPPING[16] = 
  { '.', '.',
    'p', 'P', 'r', 'R', 'n', 'N',
    'b', 'B', 'q', 'Q', 'k', 'K',
    '.', 'x' 
  };

const unsigned char MAPPING2[58] = 
  { 'A', WBISHOP, 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 
    WKING, 'L', 'M', WKNIGHT, 'O', WPAWN, WQUEEN, 
    WROOK, 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 
    ' ', ' ', ' ', ' ', ' ', ' ', 'a', BBISHOP, 'c', 'd', 
    'e', 'f', 'g', 'h', 'i', 'j', BKING, 'l', 'm', 
    BKNIGHT, 'o', BPAWN, BQUEEN, BROOK, 's', 't', 
    'u', 'v', 'w', 'x', 'y', 'z'
  };


unsigned char ** GAMEBOARD = NULL; //Square  
unsigned char * PIECES = NULL;    //store pieces/empty square

static inline unsigned char BoardSquareParse(char * s){
  return ( 8 * (8 - s[1] + '0' ) + s[0] - 'a' );
}

//Square or "S" is a pointer whose content is in [0,64]
#define IS_OUTSIDE(S)   ((*S) == OUTSIDE)
#define PIECE(S)        (IS_OUTSIDE(S) ? OUTSIDE : PIECES[*S])
#define PIECE_COLOR(S)  (PIECE(S) & 1)
#define SQUARE_FILE(S)  ('a' + (*S)%8)
#define SQUARE_RANK(S)  ('0' + 8 - (*S)/8)
#define PIECE_ASCII(S)  (MAPPING[PIECE(S)])
#define IS_EMPTY(S)     (PIECE(S) == EMPTY)
#define IS_PIECE(S)     (!IS_OUTSIDE(S) && !IS_EMPTY(S))

typedef struct {
  //64 pieces
  unsigned char * pieces;

  /** -----------Game Status Metadata--------------*/
  //squares to store WKING, BKING, ENPASS \in [0,63)
  unsigned char king[2], enpassante;
  //some flags: castling available?,  game on check?
  unsigned char castling, check;
  //numbers (guaranteed below UCHAR_MAX)
  unsigned char halfclock, fullclock, npieces;
  //Game Status; 
  unsigned int status; 
}_Board;

void BoardInitIterator(){
  if(GAMEBOARD) 
    return;
    
  // Allocate mem for 2-D GAMEBOARD (8x8) with ..
  // .. 2 layer padding on each sides (12x12).
  // accessible: board[-2:9][-2:9]
  // Valid part: board[0:7][0:7]
  int b = 8, p = 2;
  int tb = b + 2*p;
  unsigned char ** gboard = (unsigned char **)
    malloc(tb*sizeof(unsigned char *));
  board[0] = (unsigned char *)
   malloc(tb*tb*sizeof(unsigned char));
  for(int i=1; i<tb; ++i)
    gboard[i] = board[i-1] + tb;
  for(int i=0; i<tb; ++i)
    gboard[i] += p;
  gboard += p;
  for(int i=-2;i<10; ++i)
    for(int j=-2; j<10; ++j) {
      unsigned char square = 8*i + j;
      gboard[i][j] = (square >=0 && square < 64) ?
        square : OUTSIDE;
    }
  GAMEBOARD = gboard;
}

_Board * BoardInit(_Board * source) {
  /* GAMEBOARD is a bit different from "board". .. 
  .. GAMEBOARD is more like an iterator. It's global for ..
  .. any game, and helppt to iterate through the pieces*/
  if(!GAMEBOARD)
    BoardInitIterator();
     
  _Board * board = (_Board *) malloc (sizeof (_Board));
  unsigned char * pieces = (unsigned char *) 
    malloc(64* sizeof (unsigned char));
  //Copy in case there is a source specified 
  if(source) {
    memcpy (board, source, sizeof(_Board));
    memcpy (pieces, source->piece, 64*sizeof(unsigned char));
  }
  board->pieces = pieces;
  return board;
}

void BoardMakeAvailable(_Board * b){
  assert(GAMEBOARD);
  PIECES = b->pieces; 
}

void BoardSetFromFEN(_Board * b, char * fen){

  b->npieces = 0; 
  b->king[WHITE] = OUTSIDE; 
  g->king[BLACK] = OUTSIDE;

  /* Set board from FEN */
  unsigned char * pieces = b->pieces;
  unsigned char square = 0; // square id
  while(*fen != '\0') {
    assert(square != OUTSIDE);
    char c = *fen;
    ++fen;
    /* Empty squares */ 
    if (isdigit(c)) {
      int nempty = c - '0';
      assert(nempty > 0 && nempty <= 8);
      for (int i=0; i<nempty; ++i) { 
        *piece++ = EMPTY;
        ++square;
      }
    }
    else if (c == '/') 
      //Make sure all squares of this rank are filled
      assert( square%8 == 0 ); 
    else if (c == ' '){
      //Make sure all squares are filled
      assert( square == OUTSIDE ); 
      break;
    }
    else{
      // occupied square
      *piece++ =  MAPPING2[c - 'A']; 
      if(c == 'K' || c == 'k') {
        unsigned char color = 'K' ? WHITE : BLACK;
        //There cannot be multiple kings
        assert(b->king[color] == OUTSIDE); 
        b->king[color] = square; //where is the king?
      }
      else {
        //occupied by a another piece 
        assert((b->npieces)++ < 30);
      }
      square++;
    }
  }
  //Make sure that there are both 'k' and 'K' in the FEN;
  for(unsigned int i=0; i<2; ++i)
    assert(b->king[i] != OUTSIDE);

  //Let's see whose turn is now ('w'/'b')
  b->color = (*fen == 'w') ? WHITE : BLACK;
  assert(*fen == 'w' || *fen == 'b');
  assert(*(++fen) == ' '); 

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
  assert(*fen++ == ' ');

  b->enpassante = OUTSIDE;
  if(*fen != '-'){
    b->enpassante = BoardSquareSparse(fen);
    ++fen;
  }
  assert(*(++fen) == ' ');

  //Halfmove clocks are reset during a capture/a pawn advance
  b->halfclock = 0;
  while(*(++fen) != '\0'){
    char c = *fen;
    if( c == ' ' )
      break;
    assert(isdigit(c));
    b->halfclock = 10*b->halfclock + (c - '0');
    assert(g->halfclock <= 50);
  }
  assert(*fen == ' ');

  b->fullclock = 0;
  while(*(++fen) != '\0'){
    char c = *fen;
    assert(isdigit(c));
    b->fullclock = 10*b->fullclock + (c - '0');
    // No recorded FIDE game exceeded 300 moves.
    // .. I don't know the theoretical limit. 
    // .. I think draw (by 50moves rule) would have ..
    // .. occured before 1000 moves??
    assert(b->fullclock <= 10000);
  }
  assert(b->fullclock); //has to be greater than 0;
  assert(*fen == '\0');

  return;
}

