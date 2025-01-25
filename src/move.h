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

static inline void  BoardMovesFrom( unsigned char * from, 
    const char rays[], int nrays, int depth, Array * moves) {
  
  unsigned char piece = from->piece;
  //'from' square can be neither empty nor outside the box
  assert ( !IS_OUTSIDE(from) && !IS_EMPTY(from) );

  for(int i=0; i<nrays; ++i) {
    unsigned char * to = from;
    for(int j=0; j<depth; ++j) {
      to += rays[i];
      // Cannot move along the ray, will end up outside board 
      if(IS_OUTSIDE(to))
        break; 
      // Occupied by same color; 'break' moving along the ray
      if(IS_BLOCKED(from, to))
        break;
      unsigned char flags = IS_EMPTY(to)  
        ? MOVE_NORMAL : MOVE_CAPTURE;
      //create a new array for moves if not already created; 
      _BoardMove move = {
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

void BoardQueenMoves(_Board *b, unsigned char *from, Array *m){
  BoardMovesFrom(from, QUEEN_MOVES, 8, 7, m); 
}

void BoardKingMoves(_Board *b, unsigned char *from, Array *m){
  BoardMovesFrom(from, QUEEN_MOVES, 8, 1, m);
  //Check for castling ability
  if ( (MOVE_kCASTLE << b->color) & (b->castling) ) {
    //King Side castling
    int available = 1;
    unsigned char * king = &(BOARDPIECES[b->color][4]);
    assert(SQUARE_PIECE(king) == (BKING|b->color));
    assert(king == from);
    assert(king[3] == (b->color ? WROOK : BROOK));  
    for(int i=1; i<3; ++i) {
      // see if 2 squares b/w king and rook are empty
      if(!IS_EMPTY(king[i])) {
        available = 0;
        break;
      }
    }
    if(available) {
      for(int i=0; i<4; ++i) {
        // see if king, rook and the 2 squares in b/w them ..
        // .. are under attack
        if(GameIsSquareAttacked (g, king + i, !b->color)) {
          available = 0;
          break;
        }
      }
    }
    
    if(available) {
      _BoardMove move = {
        .from.piece = kinb->piece,
        .from.square = kinb->square,
        .to.piece = EMPTY,
        .to.square = king[2].square,
        .flags = MOVE_kCASTLE << b->color
      };
      array_append(b->moves,&move, sizeof(move));
    }
  }
  if ( (MOVE_qCASTLE << b->color) & (b->castling) ) {
    //Queen Side castling
    int available = 1;
    unsigned char * king = b->king[b->color];
    assert(kinb->square == (4 + 8*7*(b->color)));
    assert(king == from);
    assert(king[-4].piece == (b->color ? WROOK : BROOK));  
    for(int i=-3; i<0; ++i) {
      // see if 3 squares b/w king and rook are empty
      if(!IS_EMPTY(king[i])) {
        available = 0;
        break;
      }
    }
    if(available) {
      for(int i=-4; i<1; ++i) {
        // see if king, rook and the 3 squares in b/w them ..
        // .. are under attack
        if(GameIsSquareAttacked (g, king + i, !b->color)) {
          available = 0;
          break;
        }
      }
    }
    
    if(available) {
      _BoardMove move = {
        .from.piece = kinb->piece,
        .from.square = kinb->square,
        .to.piece = EMPTY,
        .to.square = king[-2].square,
        .flags = MOVE_qCASTLE << b->color
      };
      array_append(b->moves,&move, sizeof(move));
    }
  }
}

void BoardBishopMoves(_Board * b, unsigned char * from){
  BoardMovesFrom(from, BISHOP_MOVES, 4, 7, b->moves); 
}

void BoardKnightMoves(_Board * b, unsigned char * from){
  BoardMovesFrom(from, KNIGHT_MOVES, 8, 1, b->moves); 
}

void BoardRookMoves(_Board * b, unsigned char * from){
  BoardMovesFrom(from, ROOK_MOVES, 4, 7, b->moves); 
}

void BoardPawnMoves(_Board * b, unsigned char * from, 
    const char rays[]){

  for(int j=0; j<2; j++) {
    //Diagonal advance of pawn
    unsigned char * to = from + rays[j];
    unsigned char flags = IS_CAPTURE(*from,*to) ? 
      MOVE_CAPTURE : IS_ENPASSANTE(*from,*to,*g) ?
      MOVE_ENPASSANTE : 0;
  
    //Pawn move diagonally only if it's a  ..
    //.. capture or an "en-passante" capture
    if(!flags) 
      continue;

    flags |= IS_PROMOTION(*from, *to) ? MOVE_PROMOTION : 0;

    _BoardMove move = {
      .from.piece = from->piece,
      .from.square = from->square,
      .to.piece = to->piece,
      .to.square = to->square,
      .flags = flags
    };
     
    Array * moves = b->moves;
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
    unsigned char * to = from + rays[j];
    if(!IS_EMPTY(*to))
      break; //capture, block, outside leads to "break"
    unsigned char flags = MOVE_NORMAL;
    flags |= IS_PROMOTION(*from, *to) ? MOVE_PROMOTION : 0;
    _BoardMove move = {
      .from.piece = from->piece,
      .from.square = from->square,
      .to.piece = to->piece,
      .to.square = to->square,
      .flags = flags
    };
     
    Array * moves = b->moves;
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
    if( SQUARE_RANK(*from) != (b->color ? '2' : '7') )
      break;  
  }
}

void BoardBPawnMoves(_Board * b, unsigned char * from){
  BoardPawnMoves(g, from, BPAWN_MOVES);
}

void BoardWPawnMoves(_Board * b, unsigned char * from){
  BoardPawnMoves(g, from, WPAWN_MOVES);
}

void (*GamePieceMoves[14]) (_Game *, unsigned char *) = 
  { NULL, NULL, BoardBPawnMoves, BoardWPawnMoves, 
    BoardRookMoves, BoardRookMoves,
    BoardKnightMoves, BoardKnightMoves,
    BoardBishopMoves, BoardBishopMoves,
    BoardQueenMoves, BoardQueenMoves,
    BoardKingMoves, BoardKingMoves };

/* ---------------------------------------------------------
------------------------------------------------------------
  Functions that generate moves for each pieces . 
  Each Function pointers orresonding to each pieces ..
  .. can be called as 
   BoardPieceMoves[from->piece](g, from);
  The possible moves are appended to the array "b->moves".
  NOTE: The moves generated include illegal moves ( or ..
  .. those moves that allow the king on "attack" ). Those ..
  .. moves will be later removed. For more, Read the function
   BoardAllMoves(_Board * b);
------------------------------------------------------------
--------------------------------------------------------- */

Array * BoardAllMoves(_Board * b, Array * m){
  /* Find all moves by rule*/ 
  b->status = 0;
  //Look for draw
  if(b->halfclock == 50) {
    b->status = (128 | (2 << 8)); //Draw by 50 moves rule.
  }
  if (!b->npieces) {
    b->status = (128 | (1 << 8)); //Insufficient pieces
  }

  if(b->status) {
    if(m) m->len = 0;
    //There is no valid moves
    return (m ? m : NULL);
  }
      
  Array * moves = !m ? array_new() : m;
  moves->len = 0;

  unsigned char ** board = b->board;
  for (int i=0; i<8; ++i)
    for(int j=0; j<8; ++j) {
      unsigned char * from = &(board[i][j]);
      if( IS_EMPTY(*from) )
  continue; //empty
      if( PIECE_COLOR(*from) != b->color )
  continue; //Occupied by the other color
      //Generate possible moves with the 'piece' 
     BoardPieceMoves[from->piece](g, from);
    }
  
  //Removing Invalid Moves
  size_t smove = sizeof(_BoardMove);
  int nmoves =  (int) (moves->len / smove);
  _BoardMove * move = (_BoardMove *) (b->moves->p);
  _BoardMove * m = move;
  for(int i=0; i<nmoves; ++i, ++move) {
    if(GameIsMoveValid(g, move)) {
      if(!(m == move)) //To avoid memcpy to same dest
        memcpy(m, move, smove);
      ++m;
    }
    else 
      moves->len -= smove;
  }

  //See if theBoard is over. Bcs no moves available
  if(!b->moves->len) {
    if(b->check)
      b->status = (16 | (b->color ? 0 : 32)); //someone wins 
    else
      b->status = 128; //stalemate 
  } 
  
  return b->status; //Game continues if (b->status == 0)
    
}
