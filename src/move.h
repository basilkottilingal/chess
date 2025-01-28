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
  size_t max, len;
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
Flag BoardIsAttackedByPiece ( Square * from, 
    const char rays[], int nrays, int depth, 
    Square * sq) {
  //FIXME: remove;
  //BoardMakeAvailable(b);

  //'from' square can be neither empty nor outside the box
  assert ( !IS_OUTSIDE(from) && !IS_EMPTY(from) );

  for(int i=0; i<nrays; ++i) {
    Square * to = from;
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

Flag BoardIsAttackedByBPawn (Square * from, Square * sq) {
  return(BoardIsAttackedByPiece(from, BPAWN_MOVES, 2, 1, sq));
}

Flag BoardIsAttackedByWPawn (Square * from, Square * sq) {
  return(BoardIsAttackedByPiece(from, WPAWN_MOVES, 2, 1, sq));
}

Flag BoardIsAttackedByRook (Square * from, Square * sq) {
  return(BoardIsAttackedByPiece(from, ROOK_MOVES, 4, 7, sq));
}

Flag BoardIsAttackedByBishop (Square * from, Square * sq) {
  return(BoardIsAttackedByPiece(from, BISHOP_MOVES, 4, 7, sq));
}

Flag BoardIsAttackedByKnight (Square * from, Square * sq) {
  return(BoardIsAttackedByPiece(from, KNIGHT_MOVES, 8, 1, sq));
}

Flag BoardIsAttackedByQueen (Square * from, Square * sq) {
  return(BoardIsAttackedByPiece(from, QUEEN_MOVES, 8, 7, sq));
}

Flag BoardIsAttackedByKing (Square * from, Square * sq) {
  return(BoardIsAttackedByPiece(from, QUEEN_MOVES, 8, 1, sq));
}

Flag (*BoardIsSquareAttackedByPiece [14])
  (Square *from, Square * to) = { NULL, NULL, 
    BoardIsAttackedByBPawn, BoardIsAttackedByWPawn, 
    BoardIsAttackedByRook, BoardIsAttackedByRook,
    BoardIsAttackedByKnight, BoardIsAttackedByKnight,
    BoardIsAttackedByBishop, BoardIsAttackedByBishop,
    BoardIsAttackedByQueen, BoardIsAttackedByQueen,
    BoardIsAttackedByKing, BoardIsAttackedByKing 
};
/* ---------------------------------------------------------
------------------------------------------------------------
  The function
    BoardIsSquareAttacked( g, sq , color);
  .. check if the square "sq" is attacked by any piece of ..
  .. color "color".
    BoardIsKingAttacked( g, color);
  .. is used to see if the king of color "color" is under ..
  .. any attack. This function can be used to (1) see if ..
  .. a move is valid or not; and to (2) see if a move ..
  .. produces a check.
------------------------------------------------------------
--------------------------------------------------------- */
Flag BoardIsSquareAttacked(_Board * b, 
    Square *sq, Flag attackingColor) {
   
  BoardMakeAvailable(b);
  //Square * sq = SquarePointer(s);
  if( !IS_EMPTY(sq) )
    if( PIECE_COLOR(sq) == attackingColor ) {
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
      Square * from = &(GAMEBOARD[i][j]);
      if( IS_EMPTY(from) )
  continue; //empty
      if( from == sq )
	continue;
      if( PIECE_COLOR(from) != attackingColor )
  continue; //Occupied by the other color
      //Generate possible moves with the 'piece' ..
      // to see if 'piece' can attack 'sq'
      Piece piece = SQUARE_PIECE(from); 
      if(BoardIsSquareAttackedByPiece[piece](from, sq))
  return 1; //Some piece attacks "sq"
    }

  //Square "sq" is safe from any attack
  return 0;
}


Flag BoardIsKingAttacked(_Board * b, Flag color)  {
  /*check if the King of color "color" is attacked by ..
  .. any opponent piece */
  Square * king = SquarePointer(b->king[color]);
  return(BoardIsSquareAttacked(b, king, !color));
}

Flag BoardIsMoveValid(_Board * b, _BoardMove * move) {

    BoardMove(b, move);
    Flag valid = !BoardIsKingAttacked(b, b->color);
    if(valid)
      if(BoardIsKingAttacked(b, !b->color))
        move->flags |= MOVE_CHECK;
    BoardUnmove(b, move);

    return valid;
}

static inline 
void BoardMovesFrom( Square * from, 
    const char rays[], int nrays, int depth, 
    Array * moves) {

  //'from' square can be neither empty nor outside the box
  assert ( !IS_OUTSIDE(from) && !IS_EMPTY(from) );

  for(int i=0; i<nrays; ++i) {
    Square * to = from;
    for(int j=0; j<depth; ++j) {
      to += rays[i];
      // Cannot move along the ray, will end up outside board 
      if(IS_OUTSIDE(to))
        break; 
      // Occupied by same color; 'break' moving along the ray
      if(IS_BLOCKED(from, to))
        break;
      Flag flags = IS_EMPTY(to)  
        ? MOVE_NORMAL : MOVE_CAPTURE;
      //create a new array for moves if not already created; 
      _BoardMove move = {
        .from.piece = SQUARE_PIECE(from),
        .from.square = *from,
        .to.piece = SQUARE_PIECE(to),
        .to.square = *to,
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

void BoardQueenMoves(_Board *b, Square * from, Array * moves){
  NOT_UNUSED(b);
  BoardMovesFrom(from, QUEEN_MOVES, 8, 7, moves); 
}

void BoardKingMoves(_Board *b, Square * from, Array * moves){
  BoardMovesFrom(from, QUEEN_MOVES, 8, 1, moves);
  //Check for castling ability
  if ( (MOVE_kCASTLE << b->color) & (b->castling) ) {
    //King Side castling
    Flag available = 1;
    Square * king = &(GAMEBOARD[7*b->color][4]);
    Square * rook = &(GAMEBOARD[7*b->color][7]);
    assert(SQUARE_PIECE(king) == (BKING|b->color));
    assert(SQUARE_PIECE(rook) == (BROOK|b->color));  
    assert(king == from);
    for(int i=1; i<3; ++i) {
      Square * sq = king + i;
      // see if 2 squares b/w king and rook are empty
      if(!IS_EMPTY(sq)) {
        available = 0;
        break;
      }
    }
    if(available) {
      for(int i=0; i<4; ++i) {
        // see if king, rook and the 2 squares in b/w them ..
        // .. are under attack
        if(BoardIsSquareAttacked (b, king + i, !b->color)) {
          available = 0;
          break;
        }
      }
    }
    
    if(available) {
      _BoardMove move = {
        .from.piece = SQUARE_PIECE(king),
        .from.square = *king,
        .to.piece = EMPTY,
        .to.square = 2 + *king,
        .flags = MOVE_kCASTLE << b->color
      };
      array_append(moves,&move, sizeof(move));
    }
  }
  if ( (MOVE_qCASTLE << b->color) & (b->castling) ) {
    //Queen Side castling
    Square * king = &(GAMEBOARD[7*b->color][4]);
    Square * rook = &(GAMEBOARD[7*b->color][0]);
    assert(SQUARE_PIECE(king) == (BKING|b->color));
    assert(SQUARE_PIECE(rook) == (BROOK|b->color));  
    assert(king == from);
    Flag available = 1;
    for(int i=-3; i<0; ++i) {
      Square * sq = king + i;
      // see if 2 squares b/w king and rook are empty
      if(!IS_EMPTY(sq)) {
        available = 0;
        break;
      }
    }
    if(available) {
      for(int i=-4; i<1; ++i) {
        // see if king, rook and the 3 squares in b/w them ..
        // .. are under attack
        if(BoardIsSquareAttacked (b, king + i, !b->color)) {
          available = 0;
          break;
        }
      }
    }
    
    if(available) {
      _BoardMove move = {
        .from.piece = SQUARE_PIECE(king),
        .from.square = *king,
        .to.piece = EMPTY,
        .to.square = -2 + *king,
        .flags = MOVE_qCASTLE << b->color
      };
      array_append(moves,&move, sizeof(move));
    }
  }
}

void BoardBishopMoves(_Board * b, Square * from, Array *moves){
  NOT_UNUSED(b);
  BoardMovesFrom(from, BISHOP_MOVES, 4, 7, moves); 
}

void BoardKnightMoves(_Board * b, Square * from, Array *moves){
  NOT_UNUSED(b);
  BoardMovesFrom(from, KNIGHT_MOVES, 8, 1, moves); 
}

void BoardRookMoves(_Board * b, Square * from, Array * moves){
  NOT_UNUSED(b);
  BoardMovesFrom(from, ROOK_MOVES, 4, 7, moves); 
}

void BoardPawnMoves(_Board * b, Square * from, 
    const char rays[], Array * moves){

  for(int j=0; j<2; j++) {
    //Diagonal advance of pawn
    Square * to = from + rays[j];
    Flag flags = IS_CAPTURE(from,to) ? 
      MOVE_CAPTURE : IS_ENPASSANTE(from,to,b) ?
      MOVE_ENPASSANTE : 0;
  
    //Pawn move diagonally only if it's a  ..
    //.. capture or an "en-passante" capture
    if(!flags) 
      continue;

    flags |= IS_PROMOTION(from, to) ? MOVE_PROMOTION : 0;

    _BoardMove move = {
      .from.piece = SQUARE_PIECE(from),
      .from.square = *from,
      .to.piece = SQUARE_PIECE(to),
      .to.square = *to,
      .flags = flags
    };
     
    if(flags & MOVE_PROMOTION) {
      move.from.piece = SQUARE_PIECE(from);
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
    if(!IS_EMPTY(to))
      break; //capture, block, outside leads to "break"
    Flag flags = MOVE_NORMAL;
    flags |= IS_PROMOTION(from, to) ? MOVE_PROMOTION : 0;
    _BoardMove move = {
      .from.piece = SQUARE_PIECE(from),
      .from.square = *from,
      .to.piece = SQUARE_PIECE(to),
      .to.square = *to,
      .flags = flags
    };
    if(flags & MOVE_PROMOTION) {
      move.from.piece = SQUARE_PIECE(from);
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
    if( SQUARE_RANK(from) != (b->color ? '2' : '7') )
      break;  
  }
}

void BoardBPawnMoves(_Board * b, Square * from, Array * moves){
  BoardPawnMoves(b, from, BPAWN_MOVES, moves);
}

void BoardWPawnMoves(_Board * b, Square * from, Array * moves){
  BoardPawnMoves(b, from, WPAWN_MOVES, moves);
}

void (*BoardPieceMoves[14]) (_Board *, Square *, Array * ) 
  = { NULL, NULL, BoardBPawnMoves, BoardWPawnMoves, 
      BoardRookMoves, BoardRookMoves,
      BoardKnightMoves, BoardKnightMoves,
      BoardBishopMoves, BoardBishopMoves,
      BoardQueenMoves, BoardQueenMoves,
      BoardKingMoves, BoardKingMoves };

enum GAME_STATUS{
  GAME_CONTINUE = 0,      // Game continues;
  GAME_IS_A_WIN = 16,     // One wins
  GAME_IS_A_DRAW = 32,    // Draws
  GAME_STATUS_ERROR = 64, // Unknown status
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

  BoardMakeAvailable(b);

  /* Find all moves by rule*/ 

  b->status = GAME_CONTINUE;
  //Look for draw
  if(b->halfclock == 50) {
    //Draw by 50 moves rule.
    b->status = (GAME_IS_A_DRAW | GAME_FIFTY_MOVES); 
  }
  if (!b->npieces) {
    //Insufficient pieces
    b->status = (GAME_IS_A_DRAW | GAME_INSUFFICIENT); 
  }

  if(b->status) {
    //Game over
    if(m)
      m->len = 0;       
    return NULL;
  }
      
  Array * moves = (m == NULL) ? array_new() : m;
  moves->len = 0;

  fprintf(stdout, "A"); fflush(stdout);

  for (int i=0; i<8; ++i)
    for(int j=0; j<8; ++j) {
      Square * from = &(GAMEBOARD[i][j]);
      if( IS_EMPTY(from) )
  continue; //empty
      if( PIECE_COLOR(from) != b->color )
  continue; //Occupied by the other color
      //Generate possible moves with the 'piece' 
      BoardPieceMoves[SQUARE_PIECE(from)](b, from, moves);
    }
  fprintf(stdout, "B"); fflush(stdout);
  
  //Removing Invalid Moves
  size_t smove = sizeof(_BoardMove);
  int nmoves =  (int) (moves->len / smove);
  _BoardMove * move = (_BoardMove *) (moves->p);
  _BoardMove * _move = move;
  for(int i=0; i<nmoves; ++i, ++move) {
    if(BoardIsMoveValid(b, move)) {
      if(!(_move == move)) //To avoid memcpy to same dest
        memcpy(_move, move, smove);
      ++_move;
    }
    else 
      moves->len -= smove;
  }

  //See if theBoard is over. Bcs no moves available
  if(!moves->len) {
    if(b->check)
      //someone wins 
      b->status = (GAME_IS_A_WIN | b->color); 
    else
      b->status = (GAME_IS_A_DRAW | GAME_STALEMATE); 
    if(!m) {
      array_free(moves);
      return NULL;
    } 
  } 

  if(!m)
    array_shrink(moves);
  
  /* Array * moves contains all legal moves */
  return moves; 
}
