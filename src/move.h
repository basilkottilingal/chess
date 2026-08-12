#ifndef _CHESS_MOVE_H_
#define _CHESS_MOVE_H_

  #include "board.h"
  
  /* 
  TODO:     
  (1) SAN for _GameMove * move
  (2) Three fold.
  */
  
  /*
  .. When can a game draw:
  .. 1. Stalemate:	Player has no legal moves but is not in check.
  .. 2. Insufficient Material:	Neither player can checkmate with the remaining
  ..    pieces
  .. 3. Threefold Repetition:	The same position appears three times.
  .. 4. Fifty-Move Rule:	50 moves without a pawn move or capture.
  .. 5. Agreement:	Players agree to end the game as a draw.
  .. 6. Dead Position:	No possible moves can lead to checkmate.
  .. 7. Draw by Time: A player's time runs out, but their opponent cannot
  ..    deliver checkmate.
  */

  /*
  .. Rays along which a chesspiece can move
  */
  static const char KNIGHT_MOVES[8] = 
    { 14, 25, 23, 10, -14, -25, -23, -10 };
  static const char QUEEN_MOVES[8] =
    { 1, 13, 12, 11, -1, -13, -12, -11 };
  static const char ROOK_MOVES[4] = 
    { 1, 12, -1, -12 };
  static const char BISHOP_MOVES[4] = 
    { 13, 11, -13, -11 };
  static const char WPAWN_MOVES[4] = 
    { -11, -13, -12, -24 }; 
  static const char BPAWN_MOVES[4] = 
    { 11, 13, 12, 24 };   
  
  static inline 
  Flag BoardIsAttackedByPiece ( uint8_t * from, const char rays[], int nrays,
    int depth, uint8_t * sq)
  {
    /* 'from' square can be neither empty nor outside the box */
    assert ( IS_PIECE (from) );
  
    for (int i=0; i<nrays; ++i)
    {
      uint8_t * to = from;
      for (int j=0; j<depth; ++j)
      {
        to += rays[i];
        if (IS_OUTSIDE (to)) 
          break; 
        if (sq == to)       /* comparing pointers */
        {
          /*
          .. Making sure that the piece occupying "to" is not of the same color
          .. as the attacking piece
          */
          assert (!IS_BLOCKED (from, to));

          /* yes, the square "sq" is attacked. */
          return 1;
        }
        if (!IS_EMPTY(to))
          /* blocked by another piece */
          break;
      }
    }
    return 0; // the square "sq" is safe from an attack
  }
  
  Flag BoardIsAttackedByBPawn (uint8_t * from, uint8_t * sq)
  {
    return (BoardIsAttackedByPiece (from, BPAWN_MOVES, 2, 1, sq));
  }
  
  Flag BoardIsAttackedByWPawn (uint8_t * from, uint8_t * sq)
  {
    return (BoardIsAttackedByPiece (from, WPAWN_MOVES, 2, 1, sq));
  }
  
  Flag BoardIsAttackedByRook (uint8_t * from, uint8_t * sq)
  {
    return (BoardIsAttackedByPiece (from, ROOK_MOVES, 4, 7, sq));
  }
  
  Flag BoardIsAttackedByBishop (uint8_t * from, uint8_t * sq)
  {
    return (BoardIsAttackedByPiece (from, BISHOP_MOVES, 4, 7, sq));
  }
  
  Flag BoardIsAttackedByKnight (uint8_t * from, uint8_t * sq)
  {
    return (BoardIsAttackedByPiece (from, KNIGHT_MOVES, 8, 1, sq));
  }
  
  Flag BoardIsAttackedByQueen (uint8_t * from, uint8_t * sq)
  {
    return (BoardIsAttackedByPiece (from, QUEEN_MOVES, 8, 7, sq));
  }
  
  Flag BoardIsAttackedByKing (uint8_t * from, uint8_t * sq)
  {
    return (BoardIsAttackedByPiece (from, QUEEN_MOVES, 8, 1, sq));
  }

  /*
  .. function pointer (to see if a square is attacked) for each chesspiece
  */  
  Flag (*BoardIsSquareAttackedByPiece [12]) (uint8_t *from, uint8_t * to) =
    {
      BoardIsAttackedByRook,   BoardIsAttackedByRook,
      BoardIsAttackedByKnight, BoardIsAttackedByKnight,
      BoardIsAttackedByBishop, BoardIsAttackedByBishop,
      BoardIsAttackedByQueen,  BoardIsAttackedByQueen,
      BoardIsAttackedByBPawn,  BoardIsAttackedByWPawn, 
      BoardIsAttackedByKing,   BoardIsAttackedByKing 
    };

  /*
  .. The function "Flag BoardIsSquareAttacked ( g, sq , color)" check if the
  .. square "sq" is attacked by any piece of color "color".
  .. The function "Flag BoardIsKingAttacked (g, color);" is used to see if the
  .. king of color "color" is under any attack. This function can be used
  .. (a) to see if a move is valid or not; and
  .. (b) see if a move produces a check.
  */
  Flag BoardIsSquareAttacked (_Board * b, uint8_t *sq, Flag attackingColor)
  {
    BoardMakeAvailable(b);
    if( !IS_EMPTY(sq) )
      assert ( PIECE_COLOR (sq) != attackingColor );
  
    /* check if the square "sq" is attacked by any pieces of color "color" */
    // fixme : move along the rays, rather than traversing through all 64 squares
    for (int i=START; i<=END; ++i)
      for (int j=START; j<=END; ++j)
      {
        /* Replace it with square iterator */
        uint8_t * from = & BOARD [i][j];
        if ( IS_EMPTY (from) ) 
          continue; /* empty */
        if ( from == sq )
  	      continue;
        if ( PIECE_COLOR(from) != attackingColor )
          continue; /* Occupied by the other color */

        /* Generate possible moves of piece & see if 'piece' can attack 'sq' */
        if (BoardIsSquareAttackedByPiece [ PIECE (from) ] (from, sq))
          return 1; /* "sq" is attacked */
      }
  
    /* uint8_t "sq" is safe from any attack */
    return 0;
  }
  
  
  Flag BoardIsKingAttacked (_Board * b, Flag color)
  {
    /* check if the King of color "color" is attacked */
    uint8_t k = b->king [color];
    return(BoardIsSquareAttacked(b, BOARDSQ (k) , !color));
  }
  
  Flag BoardIsMoveValid (_Board * b, _Move * move)
  {
    BoardMove(b, move);
    Flag valid = !BoardIsKingAttacked(b, b->color);
    if (valid)
      if (BoardIsKingAttacked(b, !b->color))
        move->flags |= MOVE_CHECK;
    BoardUnmove(b, move);
  
    return valid;
  }
  
  static inline 
  void BoardMovesFrom( uint8_t * from, const char rays[], int nrays, int depth,
    Array * moves)
  {
  
    /* 'from' square can be neither empty nor outside the box */
    assert ( IS_PIECE (from) );
  
    for(int i=0; i<nrays; ++i)
    {
      uint8_t * to = from;
      for(int j=0; j<depth; ++j)
      {
        to += rays[i];

        /* Cannot move along the ray, will end up outside board */
        if (IS_OUTSIDE (to))
          break;

        /* Occupied by same color; 'break' moving along the ray */
        if (IS_BLOCKED (from, to))
          break;

        Flag flags = IS_EMPTY (to) ? MOVE_NORMAL : MOVE_CAPTURE;

        /*create a new array for moves if not already created; */
        _Move move =
        {
          .from.piece  = PIECE(from),
          .from.square = *from,
          .to.piece    = PIECE(to),
          .to.square   = *to,
          .promotion   = EMPTY,
          .flags       = flags
        };

        /* Add to the list of possible moves. */
        array_append ( moves, &move, sizeof(move) );
  
        /* Cannot move further beyond a capture. */
        if (flags & MOVE_CAPTURE)
          break;
      }
    }
  }
  
  void BoardQueenMoves (_Board *b, uint8_t * from, Array * moves)
  {
    NOT_UNUSED(b);
    BoardMovesFrom(from, QUEEN_MOVES, 8, 7, moves); 
  }

  void BoardWKingMoves(_Board *b, uint8_t * from, Array * moves)
  {
    BoardMovesFrom(from, QUEEN_MOVES, 8, 1, moves);

    if ( *from != e1)
      return; 

    if (b->castling & CASTLING_WK)
    {
      Flag available = 1;
      if (PIECES [f1] != EMPTY || PIECES [g1] != EMPTY)
        available = 0;
      if (available)
      {
        /* see if king, rook and the 2 squares in b/w are under attack */
        for (int i=0; i<4; ++i)
          if (BoardIsSquareAttacked (b, from + i, BLACK))
          {
            available = 0;
            break;
          }
      }
      
      if(available)
      {
        _Move move =
        {
          .from.piece  = WKING,
          .from.square = e1,
          .to.piece    = EMPTY,
          .to.square   = g1,
          .promotion   = EMPTY,
          .flags = CASTLING_WK
        };
        array_append(moves, &move, sizeof(move));
      }
    }

    /* Queen Side castling */
    if (b->castling & CASTLING_WQ)
    {
      Flag available = 1;
      if (PIECES [b1] != EMPTY || PIECES [c1] != EMPTY || PIECES [d1] != EMPTY)
        available = 0;
      if (available)
      {
        /* see if king, rook and the 2 squares in b/w are under attack */
        for (int i=-4; i<=0; ++i)
          if (BoardIsSquareAttacked (b, from + i, BLACK))
          {
            available = 0;
            break;
          }
      }
      
      if(available)
      {
        _Move move =
        {
          .from.piece  = WKING,
          .from.square = e1,
          .to.piece    = EMPTY,
          .to.square   = c1,
          .promotion   = EMPTY,
          .flags = CASTLING_WQ
        };
        array_append (moves, &move, sizeof(move));
      }
    }
  }
  
  void BoardBKingMoves(_Board *b, uint8_t * from, Array * moves)
  {
    BoardMovesFrom(from, QUEEN_MOVES, 8, 1, moves);

    if (*from != e8)
      return; 

    if (b->castling & CASTLING_BK)
    {
      Flag available = 1;
      if (PIECES [f8] != EMPTY || PIECES [g8] != EMPTY)
        available = 0;
      if (available)
      {
        /* see if king, rook and the 2 squares in b/w are under attack */
        for (int i=0; i<4; ++i)
          if (BoardIsSquareAttacked (b, from + i, WHITE))
          {
            available = 0;
            break;
          }
      }
      
      if(available)
      {
        _Move move =
        {
          .from.piece  = BKING,
          .from.square = e8,
          .to.piece    = EMPTY,
          .to.square   = g8,
          .promotion   = EMPTY,
          .flags = CASTLING_BK
        };
        array_append(moves, &move, sizeof(move));
      }
    }


    /* Queen Side castling */
    if (b->castling & CASTLING_BQ)
    {
      Flag available = 1;
      if (PIECES [b8] != EMPTY || PIECES [c8] != EMPTY || PIECES [d8] != EMPTY)
        available = 0;
      if (available)
      {
        /* see if king, rook and the 2 squares in b/w are under attack */
        for (int i=-4; i<=0; ++i)
          if (BoardIsSquareAttacked (b, from + i, BLACK))
          {
            available = 0;
            break;
          }
      }
      
      if(available)
      {
        _Move move =
        {
          .from.piece  = BKING,
          .from.square = e8,
          .to.piece    = EMPTY,
          .to.square   = c8,
          .promotion   = EMPTY,
          .flags = CASTLING_BQ
        };
        array_append(moves, &move, sizeof(move));
      }
    }
  }
  
  void BoardBishopMoves (_Board * b, uint8_t * from, Array *moves)
  {
    NOT_UNUSED (b);
    BoardMovesFrom(from, BISHOP_MOVES, 4, 7, moves); 
  }
  
  void BoardKnightMoves (_Board * b, uint8_t * from, Array *moves)
  {
    NOT_UNUSED (b);
    BoardMovesFrom(from, KNIGHT_MOVES, 8, 1, moves); 
  }
  
  void BoardRookMoves (_Board * b, uint8_t * from, Array * moves)
  {
    NOT_UNUSED (b);
    BoardMovesFrom(from, ROOK_MOVES, 4, 7, moves); 
  }
  
  void BoardPawnMoves (_Board * b, uint8_t * from,
    const char rays[], Array * moves)
  {
  
    for(int j=0; j<2; j++)
    {
      /* Diagonal advance of pawn */
      uint8_t * to = from + rays[j];
      Flag flags = IS_CAPTURE (from,to) ? 
        MOVE_CAPTURE : IS_ENPASSANTE (from,to,b) ?
        MOVE_ENP_CAPTURE : 0;
    
      /*
      .. Pawn move diagonally only if it's a capture or an "en-passante"
      .. capture
      */
      if(!flags) 
        continue;
  
      flags |= IS_PROMOTION (from, to) ? MOVE_PROMOTION : 0;
  
      _Move move =
      {
        .from.piece  = PIECE (from),
        .from.square = *from,
        .to.piece    = PIECE (to),
        .to.square   = *to,
        .promotion   = EMPTY,
        .flags       = flags
      };
       
      if (flags & MOVE_PROMOTION)
      {
        move.promotion = b->color == WHITE ? WROOK : BROOK;
        for(int i=0; i<4; ++i)
        {
          /* 'p' is promoted to 'r','b','n' and 'q' */
          array_append (moves, &move, sizeof(move));
          move.promotion += 2;
        }
      }
      else /* if (flag == MOVE_CAPTURE | MOVE_ENP_CAPTURE)) */
      {
        array_append (moves, &move, sizeof(move));
      }
    
    }

    /* FIXME: can be made into :for(int j=0; j<4; ++j) {} */
    for (int j=2; j<4; j++)
    {
      /* vertical advance of pawn */
      unsigned char * to = from + rays[j];
      if (!IS_EMPTY(to))
        break; /* capture, block, outside leads to "break" */
      Flag flags = MOVE_NORMAL;
      flags |= IS_PROMOTION (from, to) ? MOVE_PROMOTION : 0;
      _Move move =
      {
        .from.piece  = PIECE(from),
        .from.square = *from,
        .to.piece    = PIECE(to),
        .to.square   = *to,
        .promotion   = EMPTY,
        .flags       = flags
      };

      if(flags & MOVE_PROMOTION)
      {
        move.promotion = b->color == WHITE ? WROOK : BROOK;
        for(int i=0; i<4; ++i)
        {
          /* 'p' is promoted to 'r','b','n' and 'q' */
          array_append (moves, &move, sizeof(move));
          move.promotion += 2;
        }
      }
      else /* if (flag == MOVE_NORMAL) */
      {
        array_append (moves, &move, sizeof(move));
      }
  
      /* double advance only for starting pawns */
      if ( SQUARE_RANK(from) != (b->color ? '2' : '7') )
        break;  
    }
  }
  
  void BoardBPawnMoves (_Board * b, uint8_t * from, Array * moves)
  {
    BoardPawnMoves(b, from, BPAWN_MOVES, moves);
  }
  
  void BoardWPawnMoves (_Board * b, uint8_t * from, Array * moves)
  {
    BoardPawnMoves(b, from, WPAWN_MOVES, moves);
  }

  /* function pointers (for move) for each chesspieces */
  void (*BoardPieceMoves[12]) (_Board *, uint8_t *, Array * ) =
    {
        BoardRookMoves,   BoardRookMoves,
        BoardKnightMoves, BoardKnightMoves,
        BoardBishopMoves, BoardBishopMoves,
        BoardQueenMoves,  BoardQueenMoves,
        BoardBPawnMoves,  BoardWPawnMoves, 
        BoardBKingMoves,  BoardWKingMoves
    };
  
  /*
  .. Functions that generate moves for each pieces. Each Function pointers
  .. corresonding to each pieces can be called as 
  .. "BoardPieceMoves[from->piece](g, from);". The possible moves are appended
  .. to the array "b->moves". NOTE: The moves generated include illegal moves
  .. ( or those moves that allow the king on "attack" ). Those moves will be
  .. later removed. 
  */
  
  Flag BoardAllMoves (_Board * b, Array * moves)
  {
    if (!moves)
      return GAME_STATUS_ERROR;
  
    /* Find all moves by rule*/ 
    b->status = GAME_CONTINUE;

    /* Look for draw */
    if(b->halfclock == 100) 
      /* Draw by 50 moves rule. */
      b->status = (GAME_IS_A_DRAW | GAME_FIFTY_MOVES); 
    if (!b->npieces) 
      /* Insufficient pieces */
      b->status = (GAME_IS_A_DRAW | GAME_INSUFFICIENT); 
  
    moves->len = 0;
    if(b->status) 
      /* Game over */
      return b->status;
  
    for (int i=START; i<=END; ++i)
      for(int j=START; j<=END; ++j)
      {
        uint8_t * from = & BOARD [i][j];

        if ( IS_EMPTY (from) || PIECE_COLOR (from) != b->color )
          continue;

        /* Generate possible moves with the 'piece' */
        BoardPieceMoves [PIECE (from)] (b, from, moves);
      }
    
    /* Removing Invalid Moves */
    size_t smove = sizeof(_Move);
    int nmoves =  (int) (moves->len / smove);
    _Move * move = (_Move *) (moves->p);
    _Move * m = move;
    for (int i=0; i<nmoves; ++i, ++move)
    {
      if(BoardIsMoveValid(b, move))
      {
        /* To avoid memcpy to same dest */
        if(!(m == move))
          memcpy(m, move, smove);
        ++m;
      }
      else 
        moves->len -= smove;
    }
  
    /*See if the Board is over. Bcs no moves available */
    if(!moves->len)
    {
      b->status =  b->check ?
        (GAME_IS_A_WIN | (!b->color)) :
        (GAME_IS_A_DRAW | GAME_STALEMATE);
    } 
 
    return b->status; 
  }
  
  Flag BoardUpdateMetadata (_Board * b, _Move * move)
  {
  
    /* update the halfclock, fullclock */
    if(!b->color)
      ++(b->fullclock);
    b->halfclock = (move->flags & MOVE_CAPTURE) ? 0 :
      ((move->from.piece == WPAWN || move->from.piece == BPAWN) 
        ? 0 : (b->halfclock + 1));

    /* change the turn */
    b->color = !b->color;

    /* Is the board on Check? */
    b->check = move->flags & MOVE_CHECK;

    /* Set En-Passante square while double pawn advance */
    if( move->from.piece == WPAWN &&
        (move->from.square - move->to.square == 16) ) 
      b->enpassante = move->from.square - 8;
    else if ( move->from.piece == BPAWN && 
        (move->to.square - move->from.square == 16) ) 
      b->enpassante = move->from.square + 8;

    if (move->flags & MOVE_ENP_CAPTURE)
      b->enpassante = OUTSIDE;
    
    if (b->castling)
    {
      /* Switching off castling if king move moves */
      if(move->from.square == 4)
        b->castling &= ~(MOVE_qCASTLE | MOVE_kCASTLE);
      else if (move->from.square == 60)
        b->castling &= ~(MOVE_QCASTLE | MOVE_KCASTLE);
    
      /* Switching off castling if corner rooks move/captured */
      if (move->from.square == 0 || move->to.square == 0)
        b->castling &= ~MOVE_qCASTLE;
      if (move->from.square == 7 || move->to.square == 7)
        b->castling &= ~MOVE_kCASTLE;
      if (move->from.square == 56 || move->to.square == 56)
        b->castling &= ~MOVE_QCASTLE;
      if (move->from.square == 63 || move->to.square == 63)
        b->castling &= ~MOVE_KCASTLE;
    }
  
    /* Total number of pieces */
    if(move->flags & MOVE_CAPTURE)
      --(b->npieces);
  
    if (b->status != GAME_METADATA_NOTUPDATED)
      return GAME_STATUS_ERROR;

    b->status = GAME_STATUS_NOTUPDATED;
    return 0;
  }
  
  Flag  
  BoardNext (_Board * b, _Move * move, Array * moves)
  {
  
    if ( !move )
    {
      GameError ("BoardNext() : Aborted");
      return GAME_STATUS_ERROR; 
    }
  
    /* Move the bitboard */
    BoardMove(b, move);

    /* Update the associated metadata of the board */
    if ( BoardUpdateMetadata(b, move) == GAME_STATUS_ERROR )
    {
      GameError("BoardNext() : Metadata update failed");
      return GAME_STATUS_ERROR;
    }
   
    return (BoardAllMoves (b, moves));
  }
  
  void BoardStatusPrint (_Board * b)
  {
    Flag f = b->status;
    if (f == GAME_CONTINUE) {
      fprintf (stdout, " Game Not Over Yet");
      fflush (stdout);
      return;
    }

    Flag WinOrDraw = 0;

    if (f & GAME_IS_A_WIN)
    {
      ++WinOrDraw;
      fprintf (stdout, "\n %s wins by %s", 
        f & GAME_WHO_WINS ? "White" : "Black",
        (f & GAME_IS_WON_BY_TIME) ? "time" : 
        (f & GAME_IS_WON_BY_FORFEIT) ? "opponent's forfeit" :
        "checkmate");
    }

    if (f & GAME_IS_A_DRAW)
    {
      ++WinOrDraw;
      Flag info = f & GAME_DRAW_INFO;
      fprintf(stdout, "\n Draw : %s",
        (info == GAME_STALEMATE)     ? "Stalemate" :
        (info == GAME_INSUFFICIENT)  ? "Insufficient Material" :
        (info == GAME_FIFTY_MOVES)   ? "Fifty moves rule" :
        (info == GAME_THREE_FOLD)    ? "Three fold rule" :
        (info == GAME_WHITE_CANNOT)  ? 
          "Black ran out of time and White cannot win" :
        (info == GAME_BLACK_CANNOT)  ? 
          "White ran out of time and Black cannot win" :
        (info == GAME_AGREES)        ? "Players agree" :
          "ERROR: Unkown reason for a draw!! "); 
    }

    fflush(stdout);

    if (WinOrDraw != 1)
    {
      fprintf(stderr, "\nERROR: Game 'ended' has to be exclusively draw/win");
      fflush(stderr);
    }
    else
      return;

    if (f == GAME_METADATA_NOTUPDATED)
      fprintf(stderr, "\nERROR: Incomplete Move. Metadata/status not updated");
    else if (f == GAME_STATUS_NOTUPDATED)
      fprintf(stderr, "\nERROR: Incomplete Move. Status not updated");
    else
      fprintf(stderr, "\nERROR: Unknown game status");

    fflush(stderr);
  }
#endif
