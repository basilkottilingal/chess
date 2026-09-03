#ifndef _CHESS_MOVE_H_
#define _CHESS_MOVE_H_

  #include "board.h"
  #include "zobrist.h"

  Array history  = {.p = NULL, .max = 0, .len = 0};
  Array movesall = {.p = NULL, .max = 0, .len = 0};
  #define MOVES_AT(b) ( & ((_Move * ) movesall.p) [b->moveLoc] )

  int three_fold (_Board * b)
  {
    /* see if this board has repeated more than once in the game's history */
    
    uint64_t z = b->zobrist;

    /* first look in the board stack (which are part of the move sequence in
    .. the tree search) */

    int clck = (int) b->halfclock - 4;
    if (clck < 2)
      return 0;
    b -= 2;
    int nply   = (int) (b - BoardStack) - 2;
    int isodd  = clck & 1;
    int nfound = 0;
    int once   = 1;

    do
    {
      while (nply > 0)
      {
        if (b->zobrist == z && nfound++ == 1)
          return 1;

        b    -= 2,
        clck -= 2,
        nply -= 2;

        if (clck < 2)
          return 0;
      }

      if (!once--)
        return 0;

      /* since you haven't reached "halfclock = 0" yet, you have to look
      .. in the gameboards stored in history */

      if ( !(nply = (int) (history.len / sizeof (_Board)) ) )
        /*
        .. warning : incomplete history. This may happen if you restart with
        .. an FEN whose 50 moves clock is > 0
        */
        return 0;

      b = & ((_Board *) history.p) [nply - 1];
      assert ( (clck = b->halfclock) >= 2 );
      if ( (clck & 2) != isodd )
      {
        nply--; b--; clck --;
      }

    } while (1);

    return 0;
  }

  /* conditions to check while moving piece from 'FROM' to 'TO' */

  #define IS_NORMAL(FROM,TO)         ( IS_EMPTY (TO) )
  #define IS_BLOCKED(FROM,TO)        ( IS_PIECE (TO) &&                       \
                                     (PIECE_COLOR (FROM) == PIECE_COLOR (TO)) )
  #define IS_CAPTURE(FROM,TO)        ( IS_PIECE (TO) &&                       \
                                     (PIECE_COLOR (FROM) != PIECE_COLOR (TO)) )
  /* use (both) carefully, assumes moving piece is WPAWN/BPAWN */
  #define IS_PROMOTION(TO) (SQUARE_RANK (TO) == '8' || SQUARE_RANK (TO) == '1')
  #define IS_ENP_CAPTURE(TO,BOARD)   ((*TO) == (BOARD)->enpassante)

  char * BoardMoveSAN (_Move * m)
  {
    /* fixme: Not et impelemented */
    assert (0);
    NOT_UNUSED (m);
    return "\0";
  }

  enum GAME_STATUS
  {
    /* Board status */
    GAME_IS_A_WIN          = 16,
    GAME_IS_A_DRAW         = 32,
    GAME_CONTINUE          = 64,

    /* Encode unknown error*/
    GAME_STATUS_ERROR      = 128,

    /* Info on WIN */
    GAME_WHO_WINS          = 1,
    GAME_IS_WON_BY_TIME    = 2,
    GAME_IS_WON_BY_FORFEIT = 4,

    /* Info on draw = (STATS & GAME_DRAW_INFO)*/
    GAME_DRAW_INFO         = 15,
    GAME_STALEMATE         = 0,
    GAME_INSUFFICIENT      = 1,
    GAME_FIFTY_MOVES       = 2,
    GAME_THREE_FOLD        = 3,
    GAME_WHITE_CANNOT      = 4,
    GAME_BLACK_CANNOT      = 5,
    GAME_AGREES            = 6,

  };

  #define GAME_CONTINUES(s) ((s) & GAME_CONTINUE)

  void BoardMove (_Board * b, _Move * move)
  {
    assert (b != BoardLast);
    
    uint8_t from = move->from.square,
      to = move->to.square;

    uint8_t piece = PIECES [to] = (move->flags & MOVE_PROMOTION) ?
      move->promotion : move->from.piece;
    assert (PIECES [from] == move->from.piece);
    PIECES [from] = EMPTY;

    b[1].move = *move;
    b[1].enpassante = 
      (piece == WPAWN && from - to == 16) ? from - 8 :
      (piece == BPAWN && to - from == 16) ? from + 8 : OUTSIDE;
    b[1].castling = b[0].castling & ~move->flags;
    b[1].halfclock = 
      ((move->flags & (MOVE_CAPTURE | MOVE_ENP_CAPTURE) ) || piece == WPAWN ||
        piece == BPAWN ) ? 0 : b[0].halfclock + 1;

    /*
    .. Switching off (respective) castling ability when rook moves/atacked.
    .. An edge case, that you can miss
    */
    if (b[1].castling)
    {
      /* Switching off castling if corner rooks move/captured */
      if (from == a8 || to == a8)
        b[1].castling &= ~CASTLING_BQ;
      else if (from == h8 || to == h8)
        b[1].castling &= ~CASTLING_BK;
      else if (from == a1 || to == a1)
        b[1].castling &= ~CASTLING_WQ;
      else if (from == h1 || to == h1)
        b[1].castling &= ~CASTLING_WK;
    }

    if (piece == WKING)
    {
      kings [WHITE] = to;
      b[1].castling &= ~(CASTLING_WQ | CASTLING_WK);
      if (move->flags & CASTLING_WQ)
      {
        /* rank 1 :  "..KR.xxx"  (x : unknown)*/
        PIECES [a1] = EMPTY;
        PIECES [d1] = WROOK;
        return;
      }
      if (move->flags & CASTLING_WK)
      {
        /* rank 1 :  "xxxx.RK."  (x : unknown)*/
        PIECES [h1] = EMPTY;
        PIECES [f1] = WROOK;
        return;
      }
      return;
    }

    if (piece == BKING)
    {
      kings [BLACK] = to;
      b[1].castling &= ~(CASTLING_BQ | CASTLING_BK);
      if (move->flags & CASTLING_BQ)
      {
        /* rank 8 :  "..kr.xxx"  (x : unknown)*/
        PIECES [a8] = EMPTY;
        PIECES [d8] = BROOK;
        return;
      }
      if (move->flags & CASTLING_BK)
      {
        /* rank 8 :  "xxxx.rk."  (x : unknown)*/
        PIECES [h8] = EMPTY;
        PIECES [f8] = BROOK;
        return;
      }
      return;
    }

    if (move->flags & MOVE_ENP_CAPTURE)
      PIECES [to + (piece == WPAWN ? 8 : -8)] = EMPTY;
  }

  void BoardUnmove (_Board * b)
  {

    _Move move = b->move;
    uint8_t from = move.from.square, to = move.to.square;

    uint8_t piece = PIECES [from] = move.from.piece;
    PIECES [to]   = move.to.piece;

    if (piece == WKING)
    {
      kings [WHITE] = from;
      if (move.flags & CASTLING_WQ)
      {
        /* rank 1 :  "R...Kxxx"  (x : unknown)*/
        PIECES [a1] = WROOK;
        PIECES [d1] = EMPTY;
        return;
      }
      if (move.flags & CASTLING_WK)
      {
        /* rank 1 :  "xxxxK..R"  (x : unknown)*/
        PIECES [h1] = WROOK;
        PIECES [f1] = EMPTY;
        return;
      }
      return;
    }

    if (piece == BKING)
    {
      kings [BLACK] = from;
      if (move.flags & CASTLING_BQ)
      {
        /* rank 8 :  "r...kxxx"  (x : unknown)*/
        PIECES [a8] = BROOK;
        PIECES [d8] = EMPTY;
        return;
      }
      if (move.flags & CASTLING_BK)
      {
        /* rank 8 :  "xxxxk..r"  (x : unknown)*/
        PIECES [h8] = BROOK;
        PIECES [f8] = EMPTY;
        return;
      }
      return;
    }

    if (move.flags & MOVE_ENP_CAPTURE)
      PIECES [to + (piece == WPAWN ? 8 : -8)] =
        piece == WPAWN ? BPAWN : WPAWN;
  }
  
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
  .. Rays along which a chesspiece can move. Ray is calculated on a 
  .. 12x12 board. So for a vector [i][j], equivalent Ray will be
  .. (unisgned char) (12*i+j).
  */

  typedef const char Ray;
  static Ray KNIGHT_MOVES[8] = 
    { 14, 25, 23, 10, -14, -25, -23, -10 };
  static Ray QUEEN_MOVES[8] =
    { 1, 13, 12, 11, -1, -13, -12, -11 }; /* warning : coupled with codes [] */
  static Ray ROOK_MOVES[4] = 
    { 1, 12, -1, -12 };
  static Ray BISHOP_MOVES[4] = 
    { 13, 11, -13, -11 };
  static Ray WPAWN_MOVES[4] = 
    { -11, -13, -12, -24 }; 
  static Ray BPAWN_MOVES[4] = 
    { 11, 13, 12, 24 };
  
  static inline 
  Flag BoardIsSquareAttacked (Square sq, Flag attackingColor)
  {
  
    #define ENCODE(P)    ((uint16_t) (1<<(P)))
    #define ENCODE2(P)   ((uint16_t) (3<<(P)))
    #define MAXONE       (ENCODE2 (BPAWN) | ENCODE2 (BKING))
    #define SWITCHOFF(CODE) CODE &= ~MAXONE
    #define ATTACKED(FROM,CODE) (ENCODE (PIECE(FROM)) & CODE)

    static const uint16_t codes [] =
    {
      ENCODE2 (BROOK) | ENCODE2 (BKING) | ENCODE2 (BQUEEN),
      ENCODE  (WPAWN) | ENCODE2 (BBISHOP) | ENCODE2 (BKING) | ENCODE2 (BQUEEN),
      ENCODE2 (BROOK) | ENCODE2 (BKING) | ENCODE2 (BQUEEN),
      ENCODE  (WPAWN) | ENCODE2 (BBISHOP) | ENCODE2 (BKING) | ENCODE2 (BQUEEN),
      ENCODE2 (BROOK) | ENCODE2 (BKING) | ENCODE2 (BQUEEN),
      ENCODE  (BPAWN) | ENCODE2 (BBISHOP) | ENCODE2 (BKING) | ENCODE2 (BQUEEN),
      ENCODE2 (BROOK) | ENCODE2 (BKING) | ENCODE2 (BQUEEN),
      ENCODE  (BPAWN) | ENCODE2 (BBISHOP) | ENCODE2 (BKING) | ENCODE2 (BQUEEN)
    };
  
    for (int iray=0; iray<8; ++iray)
    {
      Ray r = QUEEN_MOVES [iray];
      Square from = sq + r;
      int j=0;
      /* move along the ray until you are out of the board / hit a piece */
      while (!IS_OUTSIDE (from) && IS_EMPTY (from))
        j++, from += r;
      if (IS_OUTSIDE (from) || PIECE_COLOR (from) != attackingColor)
        continue;
      uint16_t code = codes [iray];
      /*
      .. abiliy to attack (for kings & pawns) are switched off if the ray has
      .. moved beyond a square
      */
      if (j)
        SWITCHOFF (code);
      if (ATTACKED (from, code))
        /* attacked by PIECES [*from] */
        return 1;
    }

    uint8_t knight = BKNIGHT | attackingColor;
    for (int iray=0; iray<8; ++iray)
      if (PIECES [sq [KNIGHT_MOVES [iray]]] == knight)
        /* attacked by opponent's knight */
        return 1;

    /* the square "sq" is safe from an attack */
    return 0;
  
    #undef ENCODE
    #undef ENCODE2
    #undef MAXONE
    #undef SWITCHOFF
    #undef ATTACKED
  }

  Flag BoardIsKingAttacked (Flag attackedColor)
  {
    /* check if the King of color "color" is attacked */
    uint8_t k = kings [attackedColor];
    return BoardIsSquareAttacked (BOARDSQ (k) , !attackedColor);
  }
  
  static inline 
  void BoardMovesFrom
  (
    Square from,
    Ray rays[],
    int nrays,
    int depth,
    Array * moves
  )
  {
  
    /* 'from' square can be neither empty nor outside the box */
    assert ( IS_PIECE (from) );
  
    for(int i=0; i<nrays; ++i)
    {
      Square to = from;
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
  
  void BoardQueenMoves (_Board *b, Square from, Array * moves)
  {
    NOT_UNUSED(b);
    BoardMovesFrom(from, QUEEN_MOVES, 8, 7, moves); 
  }

  void BoardWKingMoves(_Board *b, Square from, Array * moves)
  {
    BoardMovesFrom(from, QUEEN_MOVES, 8, 1, moves);

    if (*from != e1)
      return; 

    if (
      (b->castling & CASTLING_WK)                  &&
      PIECES [f1] == EMPTY                         &&
      PIECES [g1] == EMPTY                         &&
      BoardIsSquareAttacked (from, BLACK) == 0     &&  /* e1 is safe*/
      BoardIsSquareAttacked (from + 1, BLACK) == 0 &&  /* f1 */
      BoardIsSquareAttacked (from + 2, BLACK) == 0     /* g1 */
    )
    {
      _Move move = (_Move)
        {
          .from.piece  = WKING,
          .from.square = e1,
          .to.piece    = EMPTY,
          .to.square   = g1,
          .promotion   = EMPTY,
          .flags = CASTLING_WK
        };
      array_append (moves, &move, sizeof (_Move));
    }

    if (
      (b->castling & CASTLING_WQ)                  &&
      PIECES [b1] == EMPTY                         &&
      PIECES [c1] == EMPTY                         &&
      PIECES [d1] == EMPTY                         &&
      BoardIsSquareAttacked (from, BLACK) == 0     &&  /* e1 is safe*/
      BoardIsSquareAttacked (from - 1, BLACK) == 0 &&  /* d1 */
      BoardIsSquareAttacked (from - 2, BLACK) == 0     /* c1 */
    )
    {
      _Move move = (_Move)
        {
          .from.piece  = WKING,
          .from.square = e1,
          .to.piece    = EMPTY,
          .to.square   = c1,
          .promotion   = EMPTY,
          .flags = CASTLING_WQ
        };
      array_append (moves, &move, sizeof (_Move));
    }
  }
  
  void BoardBKingMoves(_Board *b, Square from, Array * moves)
  {
    BoardMovesFrom(from, QUEEN_MOVES, 8, 1, moves);

    if (*from != e8)
      return; 

    if (
      (b->castling & CASTLING_BK)                  &&
      PIECES [f8] == EMPTY                         &&
      PIECES [g8] == EMPTY                         &&
      BoardIsSquareAttacked (from, WHITE) == 0     &&  /* e8 is safe*/
      BoardIsSquareAttacked (from + 1, WHITE) == 0 &&  /* f8 */
      BoardIsSquareAttacked (from + 2, WHITE) == 0     /* g8 */
    )
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

    if (
      (b->castling & CASTLING_BQ)                  &&
      PIECES [b8] == EMPTY                         &&
      PIECES [c8] == EMPTY                         &&
      PIECES [d8] == EMPTY                         &&
      BoardIsSquareAttacked (from, WHITE) == 0     &&  /* e8 is safe*/
      BoardIsSquareAttacked (from - 1, WHITE) == 0 &&  /* d8 */
      BoardIsSquareAttacked (from - 2, WHITE) == 0     /* c8 */
    )
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
  
  void BoardBishopMoves (_Board * b, Square from, Array *moves)
  {
    NOT_UNUSED (b);
    BoardMovesFrom(from, BISHOP_MOVES, 4, 7, moves); 
  }
  
  void BoardKnightMoves (_Board * b, Square from, Array *moves)
  {
    NOT_UNUSED (b);
    BoardMovesFrom(from, KNIGHT_MOVES, 8, 1, moves); 
  }
  
  void BoardRookMoves (_Board * b, Square from, Array * moves)
  {
    NOT_UNUSED (b);
    BoardMovesFrom(from, ROOK_MOVES, 4, 7, moves); 
  }
  
  void BoardPawnMoves
  (
    _Board * b,
    Square from,
    Ray rays[],
    Array * moves
  )
  {
  
    for(int j=0; j<2; j++)
    {
      /* Diagonal advance of pawn : Capture / Enp Capture */
      Square to = from + rays[j];
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
      Square to = from + rays[j];
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
  
  void BoardBPawnMoves (_Board * b, Square from, Array * moves)
  {
    BoardPawnMoves(b, from, BPAWN_MOVES, moves);
  }
  
  void BoardWPawnMoves (_Board * b, Square from, Array * moves)
  {
    BoardPawnMoves(b, from, WPAWN_MOVES, moves);
  }

  /* function pointers (for move) for each chesspieces */
  void (*BoardPieceMoves[12]) (_Board *, Square, Array * ) =
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
    /*
    .. Update the location in the stack "movesall.p" where you are going to
    .. storing the moves
    */
    b [0].moveLoc = b [-1].moveLoc + b [-1].totalMoves;
    b [0].totalMoves = 0;
    movesall.len = b->moveLoc * sizeof (_Move);
    /* assert (movesall.max >= movesall.len); */

    b->status = 
      (b->halfclock > 99) ? (GAME_IS_A_DRAW | GAME_FIFTY_MOVES)  :
      npieces == 0        ? (GAME_IS_A_DRAW | GAME_INSUFFICIENT) :
      /*three_fold (b)      ? (GAME_IS_A_DRAW | GAME_THREE_FOLD)   : */
                             GAME_CONTINUE;

    if (b->status != GAME_CONTINUE)
      /* Game is a draw */
      return b->status;

    uint8_t onCheck = BoardIsKingAttacked (color);

    /* Add all moves (incl invalid moves). They are still not marked */ 
    for (int i=START; i<=END; ++i)
    {
      Square from = & BOARD [i][START];
      for(int j=START; j<=END; ++j, ++from)
      {
        if ( IS_EMPTY (from) || PIECE_COLOR (from) != color )
          continue;
        /* Generate possible moves with the 'piece' */
        BoardPieceMoves [PIECE (from)] (b, from, &movesall);
      }
    }

    /*
    .. fixme : total moves go beyond 255. (Even if you just take all pawns
    .. promoted to queens)
    */
    uint8_t legalMoves = 0, totalMoves =
      (uint8_t) ((movesall.len/sizeof (_Move)) - b->moveLoc);
    _Move * move = MOVES_AT (b);

    /*
    .. Marking Moves that are invalid && marking moves that create check.
    .. You may order moves using some static board evaluation for 
    ..  
    */
    for (int i=0; i < totalMoves; ++i, ++move)
    {
      BoardMove (b++, move);
      if (BoardIsKingAttacked(color))
        move->flags = MOVE_ILLEGAL;
      else
        legalMoves ++;
      BoardUnmove(b--);
    }


    /*See if the Board is over. Bcs no moves available */
    if(!legalMoves) 
      b->status = onCheck ? (GAME_IS_A_WIN | !color) :
        (GAME_IS_A_DRAW | GAME_STALEMATE);
    else 
      b->totalMoves = totalMoves;

    return b->status; 
  }

  void BoardStatusPrint (_Board * b)
  {
    Flag f = b->status;
    if (f == GAME_CONTINUE)
    {
      fprintf (stdout, " Game Not Over Yet");
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

  _Board * BoardRoot (const char * fen)
  {
    _Board * r = BoardSetFromFEN (fen);
    if (BoardSetFromFEN (fen) == NULL)
      return NULL;
    HashInit (r);
    BoardAllMoves (r);
    return r; 
  }

  uint8_t BoardRoll (_Board * b, _Move * move)
  {
    /*
    .. similar to BoardMove (_Board, _Move), BoardRoll (_Board, _Move) 
    .. apply the move on the board (b[0] -> b[1]), and additionally
    .. (i) calculate the hash of b[1],
    .. (ii) update color, clock, npieces
    .. (iii) make the list of the moves for b[1] config
    */
    BoardMove (b, move);
    HashReinit  (b, move);
    if (move->flags & (MOVE_CAPTURE | MOVE_ENP_CAPTURE))
      npieces--;
    fullclock++;
    color = !color;
    return BoardAllMoves (b+1);
  }

  void BoardUnroll (_Board * b)
  {
    BoardUnmove (b);
    if (b->move.flags & (MOVE_CAPTURE | MOVE_ENP_CAPTURE))
      npieces++;
    fullclock--;
    color = !color;
  }
  
  int BoardGameRoll (_Move * move)
  {

    _Board * b = BoardStack;
    array_append (&history, b, sizeof (_Board));
      
    BoardMove (b, move);
    HashReinit  (b, move);
    if (move->flags & (MOVE_CAPTURE | MOVE_ENP_CAPTURE))
      npieces--;
    fullclock++;
    color = !color;
  
    b [0] = b [1];
  
    BoardAllMoves (b);
    return 1;
  }
  
  int BoardGameUnroll () 
  {
    if (!history.len) 
      return 0;

    _Board * b = BoardStack;

    BoardUnroll (b);
    *b = ((_Board *) history.p)
      [ (history.len -= sizeof (_Board)) / sizeof (_Board) ];
    BoardAllMoves (b);

    return 1;
  }

#endif
