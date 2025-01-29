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

Flag BoardAllMoves(_Board * b, Array * moves){

  assert(moves);
  BoardMakeAvailable(b);

  /* Find all moves by rule*/ 

  b->status = GAME_CONTINUE;
  //Look for draw
  if(b->halfclock == 50) 
    //Draw by 50 moves rule.
    b->status = (GAME_IS_A_DRAW | GAME_FIFTY_MOVES); 
  if (!b->npieces) 
    //Insufficient pieces
    b->status = (GAME_IS_A_DRAW | GAME_INSUFFICIENT); 

  moves->len = 0;
  if(b->status) 
    //Game over
    return b->status;

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
  
  //Removing Invalid Moves
  size_t smove = sizeof(_BoardMove);
  int nmoves =  (int) (moves->len / smove);
  _BoardMove * move = (_BoardMove *) (moves->p);
  _BoardMove * m = move;
  for(int i=0; i<nmoves; ++i, ++move) {
    if(BoardIsMoveValid(b, move)) {
      if(!(m == move)) //To avoid memcpy to same dest
        memcpy(m, move, smove);
      ++m;
    }
    else 
      moves->len -= smove;
  }

  //See if theBoard is over. Bcs no moves available
  if(!moves->len) {
    b->status =  b->check ?     // was the board on check?
      (GAME_IS_A_WIN | b->color) : // then: someone wins 
      (GAME_IS_A_DRAW | GAME_STALEMATE); // otherwise:Stalemate
  } 

  return b->status; 
}

Flag  
BoardUpdate(_Board * b, _BoardMove * move, Array * moves){

  if(!move) {
    fprintf(stderr, "\nError: Move not chosen");
    fprintf(stderr, "\nProbably loaded game is over");
    fflush(stdout);
    return 1; //Game Stopped
  }

  //Move
  BoardMove(b, move); 

  //Udate the halfclock, fullclock
  b->fullclock += (!b->color);
  b->halfclock = (move->flags & MOVE_CAPTURE) ? 0 :
    ((move->from.piece == WPAWN || move->from.piece == BPAWN) 
      ? 0 : (b->halfclock + 1));
  //Change the Turn
  b->color = !b->color;
  //Is the board on Check?
  b->check = move->flags & MOVE_CHECK;
  //Set En-Passante square while double pawn advance
  if( move->from.piece == WPAWN &&
      (move->from.square - move->to.square == 16) ) 
    b->enpassante = move->from.square - 8;
  else if( move->from.piece == BPAWN && 
      (move->to.square - move->from.square == 16) ) 
    b->enpassante = move->from.square + 8;
  else
    b->enpassante = OUTSIDE;
  
  if(b->castling) {
    //Switching off castling if king move moves
    if(move->from.square == 4)
      b->castling &= ~(MOVE_qCASTLE | MOVE_kCASTLE);
    else if (move->from.square == 60)
      b->castling &= ~(MOVE_QCASTLE | MOVE_KCASTLE);
  
    //Switching off castling if corner rooks move/captured
    if(move->from.square == 0 || move->to.square == 0)
      b->castling &= ~MOVE_qCASTLE;
    if(move->from.square == 7 || move->to.square == 7)
      b->castling &= ~MOVE_kCASTLE;
    if(move->from.square == 56 || move->to.square == 56)
      b->castling &= ~MOVE_QCASTLE;
    if(move->from.square == 63 || move->to.square == 63)
      b->castling &= ~MOVE_KCASTLE;
  }

  //Total number of pieces
  if(move->flags & MOVE_CAPTURE)
    --(b->npieces);
 
  //Update the list of moves; 
  return(BoardAllMoves(b, moves));
}

void BoardStatusPrint(_Board * b) {
  Flag f = b->status;
  if(f == GAME_CONTINUE) {
    fprintf(stdout, " Game Not Over Yet");
    fflush(stdout);
    return;
  }
  Flag cases = 0;
  if (f & GAME_IS_A_WIN) {
    ++cases;
    fprintf(stdout, "\n %s wins by %s", 
      f & GAME_WHO_WINS ? "WHITE" : "BLACK",
      (f & GAME_IS_WON_BY_TIME) ? "time" : 
      (f & GAME_IS_WON_BY_FORFEIT) ? "opponent's forfeit" :
      "checkmate");
  }
  if (f & GAME_IS_A_DRAW) {
    fprintf(stdout, "\n Draw : ");
    Flag info = f & GAME_DRAW_INFO;
    fprintf(stdout, "%s",
      (info == GAME_STALEMATE)  ? "Stalemate" :
      (info == GAME_INSUFFICIENT)  ? "Insufficient Material" :
      (info == GAME_FIFTY_MOVES)  ? "Fifty moves rule" :
      (info == GAME_THREE_FOLD)  ? "Three fold rule" :
      (info == GAME_WHITE_CANNOT)  ? 
        "Black runs out of time and white cannot win" :
      (info == GAME_BLACK_CANNOT)  ? 
        "White runs out of time and black cannot win" :
      (info == GAME_AGREES)  ? "Players agree" :
        "UNKNOWN GAME STATUS"); 
  }
  assert(cases == 1);

  fflush(stdout);
}
