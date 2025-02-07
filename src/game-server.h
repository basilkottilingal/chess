#include "game.h"
#include <ws.h>

/*
enum MSG_TYPE {
  MSG_IS_START =  "s", // send/recv
  MSG_IS_FEN   =  "f", // send only
  MSG_IS_BOARD =  "b", // recv only
  MSG_IS_MOVE  =  "m", // send/recv
  MSG_IS_MOVES =  "M", // recv only
  MSG_IS_UNDO  =  "u", // send only
  MSG_IS_RESTART = "r", // send only
  MSG_IS_META  =  "x", // send/recv
  MSG_IS_TEXT  =  "t", // send/recv
  MSG_IS_GAME_STATUS =  "g", // send/recv 
  MSG_IS_WARNING =  "w", // send/recv
  MSG_IS_ERROR =  "e", // send/recv
  MSG_IS_DEBUG =  "d"  // send/recv
}
*/

_Game * GAME_SERVER = NULL;

void ServerInit (ws_cli_conn_t client,                            
  const unsigned char *msg, uint64_t size, int type)
{ 
  if( ( msg[0] != 'f' ) || // if the message is not fen
      ( type != WS_FR_OP_TXT )  ||  // Wrong msg format
      ( size <= 20) ) {   // FEN is short
    char err[] = "eWrong Client Message (FEN) ";
	  ws_sendframe_txt(client, err);
    return; 
  }
  if(GAME_SERVER) 
    GameDestroy(GAME_SERVER);
  GAME_SERVER = GameNew(fen);
}

void ServerDestroy() {
  if(GAME_SERVER)
    GameDestroy(GAME_SERVER);
  GAME_SERVER = NULL;
}
 
void ServerRestart (ws_cli_conn_t client,                            
  const unsigned char *msg, uint64_t size, int type)
  if( ( msg[0] != 'r' ) || // if the message is not 'restart'
      ( type != WS_FR_OP_TXT )  ||  // Wrong msg format
      ( size != 1) ) {   
    // Wrong Restart msg
    char err[] = "eWrong Client Message (Restart) ";
	  ws_sendframe_txt(client, err);
    return; 
  }
  if(GAME_SERVER) 
    GameDestroy(GAME_SERVER);
  GAME_SERVER = GameNew(fen);
}

Flag ServerBoard() {
  assert(GAME_SERVER); 
  //encode new board info to
  Flag status = GameMove(GAME_SERVER, move); 
  mempcy(msg, &GAME_SERVER->board, sizeof(_Board));
  return status;
}

Flag ServerMoves(_BoardMove * move, char * msg) {
  assert(GAME_SERVER); 
  //encode new board info to
  Flag status = GameMove(GAME_SERVER, move); 
  mempcy(msg, &GAME_SERVER->board, sizeof(_Board));
  return status;
}

Flag ClientMove(char * msg) {
  Square * sq = (Square *) msg;
  assert(*sq++ == MSG_IS_MOVE);
  assert(GAME_SERVER); 
  //Decode move
  Flag nmoves = 
    (Flag) GAME_SERVER->moves->len/sizeof(_BoardMove) ;
  _BoardMove * move = (_BoardMove *) GAME_SERVER->moves->p;
  for(int i=0; i<nmoves; ++i, ++move) {
    if(move->from.sq == sq[0] && move->to.sq == sq[1]) 
      return GameMove(GAME_SERVER, move);    
  }
  return GAME_STATUS_ERROR;
}



