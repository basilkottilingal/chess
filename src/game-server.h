/**
.. TODO 
.. 1. multithread few functions, like expanding tree computation , while 
.. waiting for client's move.
.. 2. alphabeta pruning. expand pool a bit.
.. 3. Optimized tree data structure. (minimal and aligned).
.. 4. Tree search even if out of treepool memory.
.. 5. a fork/parallel thread (for game engine) to handle any crash 
*/

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#ifndef _WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#else
  non compilable line. not yet implemented in _WIN32
#endif

#include "tree.h"
#include <ws.h>


static inline 
int server_error (ws_cli_conn_t client, const char * err)
{
  /* flush [to stderr] any error being sent to client */
  assert (err[0] == 'e' || err[0] == 'w');
  ws_sendframe_txt (client, err);
  GameError (err);
  GameErrorPrint ();
  return 0;
}

static inline
int server_send (ws_cli_conn_t client, const char * msg)
{
  /*
  .. types of messages send from server to client
  .. h: handshake
  .. s: start,
  .. S: success,
  .. m: move,
  .. x: metadata,
  .. t: text,
  .. g: game status,
  .. d: debug flag
  .. e: error msg
  .. w: warning msg
  */
  static char allowed [] = "hsSfmxtgd"; /* except errors / warnings */
  assert (strchr (allowed, msg[0]) != NULL);
  fprintf (stdout, "\nserver msg to client: %s", msg);
  ws_sendframe_txt (client, msg);
  return 0;
}

static
void server_handshake (ws_cli_conn_t client, const char * msg)
{
  /* fixme : add a key */ /* h for handshake */
  assert (!strcmp (msg, "hVersion-1.0"));
  server_send (client, "hVersion-1.0");
}

static inline
void server_msg_unknown (ws_cli_conn_t client, const char * msg)
{
  char buff [200];
  snprintf (err, sizeof (buff),
    "error: unknown command from client. \n msg :", msg); 
  server_error (client, err);
}

#define COLOR_UNDEF 2
static uint8_t mycolor = COLOR_UNDEF;
static int running = 0;

static _Array history = {.p = NULL, .len = 0, .max = 0};
typedef struct
{
  _Board b;
  _Move m;
} _History;

int game_undo (ws_cli_conn_t client, const char * msg)
{
  assert (msg [1] == '\0');
  assert (running);
  if (!history.len)
    return 0;
  assert (history.len % sizeof (_History) == 0);
  _History * h = ((_History *) history.p)
    [ (history.len -= sizeof (_History))/ sizeof (_History) ];
  _Move * move = & h->m;
  FINISH_UNMOVE (move);
  BoardUnmove (move);
  BoardStack [0] = h->b;
  BoardAllMoves (b);
  return 1;
}

static int game_start (ws_cli_conn_t client, const char * msg)
{
  /* fixme : start clock */
  assert (msg [1] == '\0');
  if (mycolor == COLOR_UNDEF)
    color = BLACK; /* Let the client be white by default */
  running = 1;
  return 1;
}

static int game_reset (ws_cli_conn_t client, const char * msg)
{
  /* 'r' for restart */
  assert (msg [1] == '\0');
  assert (running);
  while (game_undo (client, "u")) {};
game_start (client, "s"); // fixme : there is no option to "start" yet.
  return 1;
}

static int game_set (ws_cli_conn_t client, const char * msg)
{
  /* 'r' for restart */
  _Board * b = BoardSetFromFEN (msg);
  if (b == NULL)
    return 0;
  BoardFindAllMoves (b);
  history.len = 0;
  mycolor = COLOR_UNDEF;
  running = 0;
game_start (client, "s"); // fixme : there is no option to "start" yet.
  return 1;
}

static int game_engine (ws_cli_conn_t client, const char * msg)
{
  assert (!running);
  if (running)
    return server_error (client, "error : cannot set a player. a game running");
  char c = msg [1];
  if (c != 'w' && c != 'b')
    return server_error (client, "error : player should be 'w' or 'b'");
  mycolor = c == 'w' ? WHITE : BLACK;
  assert (msg [2] == '\0');
  /* fixme : engine  */
  return 1;
}

static int game_move (ws_cli_conn_t client, _Move * move)
{
  if (!running)
    return server_error (client, "error : cannot make a move. no game running");
  _Board * b = BoardStack;
  array_append (&history, & (_History) {.b = *b, .m = *move}, sizeof (_History));
  BoardMove (b, move);
  FINISH_MOVE (move);
  b [0] = b [1];
  BoardAllMoves (b);
  return 1;
}

int game_end (ws_cli_conn_t client)
{
  _Array * a [2] = {& history, & movesall};
  for (int i=0; i<2; ++i)
  {
    if (a [i]->p != NULL)
      free (a[i]->p);
    a [i]->p = NULL;
    a [i]->len = a [i]->max = 0;
  }
  running = 0;
  mycolor = COLOR_UNDEF;
  /* switch off the engine */
}

int game_server_move (ws_cli_conn_t client, const char * msg)
{
  if (msg [1] != '\0')
    server_error (client, "warning : unwanted trailing characters");
    
  if (BoardStack [0].status != GAME_CONTINUE)
  {
    ServerError(client, "warning : game over! (fixme)");
    return 0;
  }

  /* fixme : make the move suggested by engine*/
  _Move * moves = MOVES_AT (BoardStack);

  /* fixme : as of now, using a random move generator */
  srand ((unsigned) time (NULL));
  int nm = (int) BoardStack [0].totalMoves, n = nm, r = rand () % nm;
  while (n--)
  {
    if (moves [r].flags == MOVE_ILLEGAL)
    {
      r = (r+1) % nm;
      continue;
    }
    game_move (client, BoardStack, & moves [r]);
    return 1;
  }

  assert (0); /* couldn't find a legal move even though flag == GAME_CONTINUE?? */
  return 0;
}

int game_client_moved (ws_cli_conn_t client, const char * msg, uint64_t size)
{
  int msglen = (int) size - 1;
  assert (msglen == 4 || msglen == 5);
  assert (color != mycolor);

  uint8_t
    from = 8 * (8 - m[1] + '0' ) + m[0] - 'a',
    to   = 8 * (8 - m[3] + '0' ) + m[2] - 'a';
  assert (from < OUTSIDE && to < OUTSIDE);

  uint8_t prom =
    msglen ==  4  ? EMPTY :
    m [4]  == 'q' ? (BQUEEN  | color) :
    m [4]  == 'b' ? (BBISHOP | color) :
    m [4]  == 'r' ? (BROOK   | color) :
    m [4]  == 'n' ? (BKNIGHT | color) :
    INVALID;
  assert (prom != INVALID);

  _Move * move = MOVES_AT (BoardStack);
  for (uint8_t n = BoardStack[0].totalMoves; n; --n, ++move)
  {
    if
    (
      (move->flags == MOVE_ILLEGAL) ||
      (from != move->from.square)   ||
      (to   != move->to.square)     ||
      (prom != EMPTY && prom != move->promotion)
    )
      continue;
    game_move (move);
    return 1;
  }

  server_error ("error : server cannot find client's move");
  return 0;
}

/**
  main server function, that decodes the message ..
  .. friom client and send back appropriate responses ..
  .. or error/warnign messsages 
*/
  
    
int server_interpret (
  ws_cli_conn_t client,
  const unsigned char * msg,
  uint64_t size,
  int type
)
{
  /*
  .. types of recv messages are
  .. r: restart,
  .. f: fen
  .. p: server player,
  .. u: undo,
  .. M: make a move
  .. m: update clien't move
  .. d: debug flag (fixme)
  */

  char cmd = (char) msg[0];
  if (cmd == 'm')
    return game_client_moved (client, msg, size);
  if (cmd == 'M')
    return game_server_move (client, msg);
  if (cmd == 'r')
    return game_reset (client, msg);
  if (cmd == 'f')
    return game_set (client, msg);
  if (cmd == 's')
    return game_start (client, msg);
  if (cmd == 'p')
    return game_player (client, msg);
  if (cmd == 'u') 
    return game_undo (client, msg);
  return server_error ("error : unknown msg from client");
}

/**
 * @brief Called when a client connects to the server.
 *
 * @param client Client connection. The @p client parameter is used
 * in order to send messages and retrieve informations about the
 * client.
 */
void onopen (ws_cli_conn_t client)
{
	char *cli, *port;
	cli  = ws_getaddress (client);
	port = ws_getport (client);

  #ifndef DISABLE_VERBOSE
	printf ("connection opened, addr: %s, port: %s\n", cli, port);
  #endif
  /* fixme : make a handshake */
}

/**
 * @brief Called when a client disconnects to the server.
 *
 * @param client Client connection. The @p client parameter is used
 * in order to send messages and retrieve informations about the
 * client.
 */

void onclose(ws_cli_conn_t client)
{
	char *cli;
	cli = ws_getaddress(client);

  #ifndef DISABLE_VERBOSE
	printf("connection closed, addr: %s\n", cli);
  #endif
}

/**
 * @brief Called when a client connects to the server.
 *
 * @param client Client connection. The @p client parameter is used
 * in order to send messages and retrieve informations about the
 * client.
 *
 * @param msg Received message, this message can be a text
 * or binary message.
 *
 * @param size Message size (in bytes).
 *
 * @param type Message type.
 */
void onmessage (
  ws_cli_conn_t client,
	const unsigned char *msg,
  uint64_t size,
  int type
)
{
	char *cli;
	cli = ws_getaddress(client);

  if (type == WS_FR_OP_BIN)
  {
    server_error ("error : binary msg not expected");
    return;
  }

  server_interpret (client, msg, size, type);

	/**
	 * Mimicks the same frame type received and re-send it again
	 *
	 * Please note that we could just use a ws_sendframe_txt()
	 * or ws_sendframe_bin() here, but we're just being safe
	 * and re-sending the very same frame type and content
	 * again.
	 *
	 * Alternative functions:
	 *   ws_sendframe()
	 *   ws_sendframe_txt()
	 *   ws_sendframe_txt_bcast()
	 *   ws_sendframe_bin()
	 *   ws_sendframe_bin_bcast()
	 */
}

#ifndef PORT_START
#define PORT_START 8080
#endif
#ifndef PORT_END
#define PORT_END 8090
#endif

int is_port_available (int port) 
{
  int sock = socket (AF_INET, SOCK_STREAM, 0);
  if (sock < 0)
    return 0;
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(port);

  int result = bind (sock, (struct sockaddr*) & addr, sizeof(addr));
  close(sock);
  return result == 0;
}

/**
 * @brief Main routine.
 *
 * @note After invoking @ref ws_socket, this routine never returns,
 * unless if invoked from a different thread.
 */

int main ( void /* fixme : may pass address & port */ )
{

  /*
   *  See if 8080 is free in shell
   *  $  sudo lsof -i :8080
   *  $  netstat -tuln | grep 8080
   */

  int available = 0;
  for (uint32_t port = PORT_START; port <= PORT_END; ++port) 
  {
    if( !is_port_available(port) )
      continue;
    
    available = 1;
    fprintf(stdout, "server listening to 127.0.0.1:%d\n", port); 
  
  	ws_socket ( &(struct ws_server)
      {
	      /*
		     * Bind host:
		     * localhost -> localhost/127.0.0.1
		     * 0.0.0.0   -> global IPv4
		     * ::        -> global IPv4+IPv6 (DualStack)
		     */
  		  .host          = "0.0.0.0",
	   	  .port          = port,
		    .thread_loop   = 0,
		    .timeout_ms    = 1000,
  		  .evs.onopen    = & onopen,
	  	  .evs.onclose   = & onclose,
		    .evs.onmessage = & onmessage
	    } );
  }
  
  if (!available)  
    fprintf(stderr, "error: no port between %d and %d are available",
      PORT_START, PORT_END);

	/*
	 * If you want to execute code past ws_socket(), set
	 * .thread_loop to '1'.
	 */

	return 0;
}
