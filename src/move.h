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
    if( !IS_EMPTY(sq) )
      assert ( PIECE_COLOR (sq) != attackingColor );
  
    /* check if the square "sq" is attacked by any pieces of color "color" */
    // fixme : move along the rays, rather than traversing through all 64 squares
    for (int i=START; i<=END; ++i) {
      const uint8_t * from = & BOARD [i][START];
      for (int j=START; j<=END; ++j, ++from)
      {
        /* Replace it with square iterator */
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
    }
  
    /* uint8_t "sq" is safe from any attack */
    return 0;
  }
  
  
  Flag BoardIsKingAttacked (_Board * b, Flag color)
  {
    /* check if the King of color "color" is attacked */
    uint8_t k = kings [color];
    return(BoardIsSquareAttacked(b, BOARDSQ (k) , !color));
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

    if (*from != e1)
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
      /* Diagonal advance of pawn : Capture / Enp Capture */
      uint8_t * to = from + rays[j];
      if (IS_OUTSIDE (to))
        continue;

      Flag flags = IS_CAPTURE (from,to) ?
        ( IS_PROMOTION (to) ? (MOVE_CAPTURE|MOVE_PROMOTION) : MOVE_CAPTURE ) :
        IS_ENP_CAPTURE (to,b) ? MOVE_ENP_CAPTURE : MOVE_NORMAL;
    
      if (flags == MOVE_NORMAL)
        continue;
  
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
        move.promotion = color == WHITE ? WROOK : BROOK;
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

    for (int j=2; j<4; j++)
    {
      /* vertical advance of pawn */
      unsigned char * to = from + rays[j];
      if (!IS_EMPTY(to))
        break; /* capture, block, outside leads to "break" */

      uint8_t flags = IS_PROMOTION (to) ? MOVE_PROMOTION : MOVE_NORMAL;
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
        move.promotion = color == WHITE ? WROOK : BROOK;
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
      if ( SQUARE_RANK(from) != (color ? '2' : '7') )
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
  
  Flag BoardAllMoves (_Board * b)
  {
    b->status = (b->halfclock == 100) ? (GAME_IS_A_DRAW | GAME_FIFTY_MOVES):
      npieces == 0 ? (GAME_IS_A_DRAW | GAME_INSUFFICIENT) : GAME_CONTINUE;

    if (b->status != GAME_CONTINUE)
      /* Game is a draw */
      return b->status;

    uint8_t onCheck = BoardIsKingAttacked (b, color);

    /* Add all moves (incl invalid moves). They are still not marked */ 
    for (int i=START; i<=END; ++i) {
      const uint8_t * from = & BOARD [i][START];
      for(int j=START; j<=END; ++j, ++from)
      {
        if ( IS_EMPTY (from) || PIECE_COLOR (from) != color )
          continue;
        /* Generate possible moves with the 'piece' */
        BoardPieceMoves [PIECE (from)] (b, from, &movesall);
      }
    }

    uint16_t legalMoves = 0,
      totalMoves = (movesall.len/sizeof (_Move)) - b->moveLoc;
    _Move * move = MOVES_AT (b);

    /*
    .. Marking Moves that are invalid && marking moves that create check.
    .. You may order moves using some static board evaluation for 
    ..  
    */
    for (int i=0; i < totalMoves; ++i, ++move)
    {
      BoardMove(b, move);
      if (BoardIsKingAttacked(b, color))
        move->flags = MOVE_ILLEGAL;
      else
        legalMoves ++;
      BoardUnmove(b, move);
    }

    assert (totalMoves < UINT8_MAX);
    b->totalMoves = (uint8_t) totalMoves;
  
    /*See if the Board is over. Bcs no moves available */
    if(!legalMoves)
      b->status = onCheck ? (GAME_IS_A_WIN | !color) :
        (GAME_IS_A_DRAW | GAME_STALEMATE);
      printf ("\n[legal %u total %u]", legalMoves, totalMoves);
    return b->status; 
  }

  void GameMove (_Move * move) {
    _Board * b = BoardStack;
    BoardMove (b, move);
    
    if (move->flags & (MOVE_CAPTURE | MOVE_ENP_CAPTURE))
      npieces --; 
    color = !color;
    fullclock++;

    b [0].moveLoc = 0;
    b [0] = b [1];
    movesall.len = 0;
  }

  void GameUndo () {
    /* history not implemented */
    assert (0);
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

    if (WinOrDraw == 1)
      return;

    fprintf(stderr, "\nERROR: Unknown game status");
  }
#endif
