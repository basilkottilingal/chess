#ifndef _CHESS_TREE_H_
#define _CHESS_TREE_H_

  int BoardNextLevel (_Board ** y)
  {

    _Board * b = * y;
    /*
    .. assumes moves are already stored in 
    .. [b->moveLoc, b->moveLoc + b->totalMoves)
    */
    _Move * moves = MOVES_AT (b), move = moves + (b->totalMoves - 1);
    while (b->totalMoves && move->flag == MOVE_ILLEGAL)
    {
      b->totalMoves --;
      move --;
    }

    if (!b->totalMoves)  /* Moves exhausted */
      return 0;

    BoardMove (b, move);

    /*
    .. global iterators representing board config
    .. except PIECES [], and other params stored in _Board.
    */
    if (move->flags & (MOVE_CAPTURE | MOVE_ENP_CAPTURE))
      npieces --; 
    color = !color;
    fullclock++;

    b[1].moveLoc = b[0].moveLoc + b[0].totalMoves;
    /* go one level deeper in the tree traversal */
    (*y)++;
  }

  void BoardPrevLevel (_Board ** b)
  {
    if (move->flags & (MOVE_CAPTURE | MOVE_ENP_CAPTURE))
      npieces ++; 
    color = !color;
    fullclock--;

    BoardUnmove (*b, move);

    (*b)--;
  }

#endif
