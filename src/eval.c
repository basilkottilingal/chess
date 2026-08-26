#ifndef _CHESS_STATIC_EVAL_H_
#define _CHESS_STATIC_EVAL_H_

  #include "move.h"

  static int32_t piece_mobility (Square sq, Ray rays [], int nrays, int depth)
  {
    assert (IS_PIECE (from));
    int32_t n = 0;
    /* 'from' square can be neither empty nor outside the box */
    for (int i=0; i<nrays; ++i)
    {
      Square to = from + rays [i];
      for (int j=0; j<depth; ++j, to += rays [i])
      {
        /*
        .. cannot move along the ray, will end up outside board or hit a piece of
        .. the same color
        */
        if (IS_OUTSIDE (to) || IS_BLOCKED (from, to))
          break;
        n++;
      }
    }
    return n;
  }

  int16_t BoardEval (_Board * b)
  {

    /* piece square table */
    static const uint8_t psq [64] =
    {
      0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 1, 2, 2, 1, 0, 0,
      0, 1, 2, 3, 3, 2, 1, 0,
      0, 2, 3, 5, 5, 3, 2, 0,
      0, 2, 3, 5, 5, 3, 2, 0,
      0, 1, 2, 3, 3, 2, 1, 0,
      0, 0, 1, 2, 2, 1, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0,
    };

    int16_t score = 0, eg = 0, mg = 0, val = 0;
    int16_t passed, doubled;
    uint8_t bishop_color [2][2] = {0};

    /* a naive board eval */
    for (char r = '8'; r >= '1'; --r)
    {
      Square sq = & BOARD [r - '8' + START][START];
      for (char f = 'a'; f <= 'h'; ++f, ++sq)
      {
        switch (PIECES [*sq])
        {
          case EMPTY :
            break;

          case BPAWN :
            score -= 100;
            /* passed pawn bonus */
            passed = 1, doubled = 0;
            for (Square s = sq + 12; !IS_OUTSIDE (s); s += 12)
            {
              if (PIECES [*s] == BPAWN)
                doubled = 1;
              if (PIECES [s[1]] == WPAWN || PIECE [s[-1]] == WPAWN)
                passed = 0;
            }
            if (passed)
              eg -= 20 + 5 * ('2' - r + 5) + 20 * (r == '2'),
              mg -= 10 + 2 * ('2' - r + 5) + 10 * (r == '2');
            /* doubled pawn penalty */
            if (doubled)
              mg += 8, eg += 10;
            /* connected pawn bonus */
            if (PIECES [s[1]] == BPAWN)
              mg -= 1, eg -= 3;
            if (PIECES [s[-1]] == BPAWN)
              mg -= 1, eg -= 3;
            /* connected + supported */ 
            if (PIECES [s[-13]] == BPAWN)
              mg -= 1, eg -= 3;
            if (PIECES [s[-11]] == BPAWN)
              mg -= 1, eg -= 3;
            break;

          case WPAWN :
            score += 100;
            /* passed pawn bonus */
            passed = 1, doubled = 0;
            for (Square s = sq - 12; !IS_OUTSIDE (s); s -= 12)
            {
              if (PIECES [*s] == WPAWN)
                doubled = 1;
              if (PIECES [s[1]] == BPAWN || PIECE [s[-1]] == BPAWN)
                passed = 0;
            }
            if (passed)
              eg += 20 + 5 * (r - '2') + 20 * (r == '7'),
              mg += 10 + 2 * (r - '2') + 10 * (r == '7');

            /* doubled pawn penalty */
            if (doubled)
              mg -= 8, eg -= 10;
            /* connected pawn bonus */
            if (PIECES [s[1]] == WPAWN)
              mg += 1, eg += 3;
            if (PIECES [s[-1]] == WPAWN)
              mg += 1, eg += 3;
            /* connected + supported */ 
            if (PIECES [s[13]] == WPAWN)
              mg += 1, eg += 3;
            if (PIECES [s[11]] == WPAWN)
              mg += 1, eg += 3;
            break;

          case BKNIGHT :
            val += 1;
            score -= 320;
            /* centre bonus */
            score -= psq [*sq];
            mg -= 2*piece_mobility (sq, KNIGHT_MOVES, 8, 1);
            break;

          case WKNIGHT :
            val += 1;
            score += 320;
            /* centre bonus */
            score += psq [*sq];
            mg += 2*piece_mobility (sq, KNIGHT_MOVES, 8, 1);
            break;

          case BROOK :
            val += 2;
            score -= 500;
            score -= pst [*sq];
            mg -= 2*piece_mobility (sq, ROOK_MOVES, 4, 7);
            break;

          case WROOK :
            val += 2;
            score += 500;
            score += pst [*sq];
            mg += 2*piece_mobility (sq, ROOK_MOVES, 4, 7);
            break;

          case BBISHOP :
            val += 1;
            score -= 330;
            score -= psq [*sq];
            bishop_color [BLACK][(r & (char) 1) ^ (f & (char) 1)] = 1;
            mg -= 2*piece_mobility (sq, BISHOP_MOVES, 4, 7);
            break;

          case WBISHOP :
            val += 1;
            score += 330;
            score += psq [*sq];
            bishop_color [WHITE][(r & (char) 1) ^ (f & (char) 1)] = 1;
            mg += 2*piece_mobility (sq, BISHOP_MOVES, 4, 7);
            break;

          case BQUEEN :
            val += 4;
            score -= 900;
            score -= psq [*sq];
            mg -= piece_mobility (sq, QUEEN_MOVES, 8, 7);
            break;

          case WQUEEN :
            val += 4;
            score += 900;
            score += psq [*sq];
            mg += piece_mobility (sq, QUEEN_MOVES, 8, 7);
            break;

          case BKING :
            break;

          case WKING :
            break;

          default :
            assert (0);
        }
      }
    }

    /* bishop pair */
    if (bishop_color [BLACK][0] && bishop_color [BLACK][1])
      mg -= 20, eg -= 30;
    if (bishop_color [WHITE][0] && bishop_color [WHITE][1])
      mg += 20, eg += 30;

    /* val in ~[0,256], is a measure of midgame/endgame */
    val = (val * 256)/24;
    score += (mg * val + eg * (256 - val)) / 256;
    return color == WHITE ? score : -score;
  }

  
  Flag BoardAllMovesOrdered (_Board * b)
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
      (b->halfclock == 100) ? (GAME_IS_A_DRAW | GAME_FIFTY_MOVES) :
      npieces == 0          ? (GAME_IS_A_DRAW | GAME_INSUFFICIENT) :
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
      BoardMove(b, move);
      if (BoardIsKingAttacked(color))
        move->flags = MOVE_ILLEGAL;
      else
        legalMoves ++;
      BoardUnmove(move);
    }

    /*See if the Board is over. Bcs no moves available */
    if(!legalMoves) 
      b->status = onCheck ? (GAME_IS_A_WIN | !color) :
        (GAME_IS_A_DRAW | GAME_STALEMATE);
    else 
      b->totalMoves = totalMoves;

    return b->status; 
  }

  #if 0
  int16_t TreeEval (_Board * b)
  {
    do {
      do
      {
        if (!BoardNextLevel (&b))
          break;
        --depth;

        /* skip */
        _Entry * e = HashLoc (b->zobrist);
        if (e->hash == b->zobrist /*&& e->depth > REQD */)
        {
          break;
        }

        traversed [LEVEL(depth) - 1] ++;
      } while (depth);

      /*
      .. reduction
      */

      BoardPrevLevel (&b);

      /* new entry to TT or update deth */
      _Entry * e = HashLoc (b[1].zobrist);
      e->depth = (uint8_t) depth;
      e->hash = b[1].zobrist;

    } while (++depth <= DEPTHMAX);
  }
  #endif

#endif
