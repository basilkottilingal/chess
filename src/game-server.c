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

#include "_tree.h"
#include <ws.h>

static int server_send (ws_cli_conn_t client, const char * msg)
{
  /*
  .. types of messages send from server to client
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

  static char allowed [] = "sSfmxtgd";
  /* use server_error () for warnings and errors */
  assert (strchr (allowed, msg[0]) != NULL);
  fprintf (stdout, "\n{server >> client : %s}", msg);
  ws_sendframe_txt (client, msg);
  return 1;
}

static int server_error (ws_cli_conn_t client, const char * err)
{
  /* flush [to stderr] any error being sent to client */
  assert (err[0] == 'e' || err[0] == 'w');
  fprintf (stdout, "\n{server >> client : %s}", err);
  ws_sendframe_txt (client, err);
  GameError (err);
  GameErrorPrint ();
  return 0;
}

static
void server_handshake (ws_cli_conn_t client, const char * msg)
{
  /* fixme : not yet implemented. also, add a key */ /* h for handshake */
  assert (!strcmp (msg, "hVersion-1.0"));
  server_send (client, "hVersion-1.0");
}

static
int server_msg_unknown (ws_cli_conn_t client, const char * msg)
{
  char buff [200];
  snprintf (buff, sizeof (buff),
    "error: unknown command from client. \n msg : %s", msg); 
  server_error (client, buff);
  return 0;
}

static uint8_t mycolor = BLACK;  /* by default client takes WHITE */


static int game_start (ws_cli_conn_t client, const char * msg)
{
  /* fixme : 1. not yet implemented, 2. start clock */
  assert (msg [1] == '\0');
  return server_send (client, "S");
}

static void game_print ()
{
  char fen [100];
  BoardFEN (BoardStack, fen);
  fprintf (stdout, "\n%s\n", fen);
  BoardPrint (BoardStack);
}

static int server_send_fen (ws_cli_conn_t client)
{
  char fen [100]; fen [0] = 'f';
  BoardFEN (BoardStack, &fen[1]);
  return server_send (client, fen);
}

static int game_undo (ws_cli_conn_t client, const char * msg)
{
  assert (msg [1] == '\0');
  int undo = 0;
  if (BoardGameUnroll ())
  {
    if (color == mycolor)
      BoardGameUnroll ();
  }
  else
    return
      server_error (client, "warning : out of history. cannot undo");
  
  return server_send_fen (client);
}

static int game_reset (ws_cli_conn_t client, const char * msg)
{
  assert (msg [1] == '\0');
  while (BoardGameUnroll ()) {};
  return server_send_fen (client);
}

static int game_set (ws_cli_conn_t client, const char * msg)
{
  history.len = 0;
  _Board * b = BoardSetFromFEN (msg+1);
  if (BoardRoot (msg + 1) == NULL)
  {
    assert (BoardRoot ("") != NULL);
    return server_error (client, "error : wrong fen");
  }
  game_print ();
  return server_send_fen (client);
}

static int game_engine (ws_cli_conn_t client, const char * msg)
{
  char c = msg [1];
  if (c != 'w' && c != 'b')
    return server_error (client, "error : player should be 'w' or 'b'");
  mycolor = c == 'w' ? WHITE : BLACK;
  assert (msg [2] == '\0');
  /* fixme : engine  */
  return server_send (client, "S");
}

static int game_status (ws_cli_conn_t client, const char * msg)
{
  assert (msg [1] == '\0');
  uint8_t f = BoardStack[0].status;
  char reply [100];
  reply [0] = 'g';
  if (GAME_CONTINUES (f))
    return 0;
  char * status = reply + 1;
  if (f & GAME_IS_A_WIN)
  {
    sprintf (status, "%s wins by %s", 
      f & GAME_WHO_WINS ? "White" : "Black",
      (f & GAME_IS_WON_BY_TIME) ? "time" : 
      (f & GAME_IS_WON_BY_FORFEIT) ? "opponent's forfeit" :
      "checkmate");
    server_send (client, reply);
    return 1;
  }
  if (f & GAME_IS_A_DRAW)
  {
    Flag info = f & GAME_DRAW_INFO;
    sprintf(status, "draw : %s",
      (info == GAME_STALEMATE)  ? "Stalemate" :
      (info == GAME_INSUFFICIENT)  ? "Insufficient Material" :
      (info == GAME_FIFTY_MOVES)  ? "Fifty moves rule" :
      (info == GAME_THREE_FOLD)  ? "Three fold rule" :
      (info == GAME_WHITE_CANNOT)  ? 
        "Black ran of time and White cannot win" :
      (info == GAME_BLACK_CANNOT)  ? 
        "White ran of time and Black cannot win" :
      (info == GAME_AGREES)  ? "Players agree" :
        "ERROR: Unkown reason for a draw!! "); 
    server_send (client, reply);
    return 1;
  }
  return 0;
}

int game_end (ws_cli_conn_t client)
{
  Array * a [2] = {& history, & movesall};
  for (int i=0; i<2; ++i)
  {
    if (a [i]->p != NULL)
      free (a[i]->p);
    a [i]->p = NULL;
    a [i]->len = a [i]->max = 0;
  }
  /* fixme : switch off the engine */
}

int game_server_move (ws_cli_conn_t client, const char * msg)
{
  if (color != mycolor)
    return server_error (client, "error : it is not server's turn!");
    
  if (BoardStack [0].status != GAME_CONTINUE)
    return server_error (client, "warning : game over! (fixme)");

  /* fixme : make the move suggested by engine*/
  _Move * moves = MOVES_AT (BoardStack);

  #if 0
  /* random move generator */
  srand ((unsigned) time (NULL));
  int nm = (int) BoardStack [0].totalMoves, n = nm, r = rand () % nm;
  while (n--)
  {
    if (moves [r].flags == MOVE_ILLEGAL)
    {
      r = (r+1) % nm;
      continue;
    }
    _Move move = moves [r];
  }
  #endif

  _Move * move = BoardProbeAlphaBeta (5u);
  if (move == NULL)
    return server_error (client, "error : can't make a move. game over");
  const char reply [] =
    { 
      'm',  
      RANKFILE [move->from.square][0],
      RANKFILE [move->from.square][1],
      RANKFILE [move->to.square][0],
      RANKFILE [move->to.square][1],
      (move->flags & MOVE_PROMOTION) ?
        ASCII [move->promotion & ~ (uint8_t) 1] : '\0',
      '\0'
    };
  BoardGameRoll (move);
  game_print ();
  return server_send (client, reply); 
}

int game_client_moved (ws_cli_conn_t client, const char * msg, uint64_t size)
{
  if (color == mycolor)
    return server_error (client, "error : it is not client's turn!");

  int msglen = (int) size - 1;
  assert (msglen == 4 || msglen == 5);
  assert (color != mycolor);
  const char * m = msg + 1;
  uint8_t
    from = 8 * (8 - m[1] + '0') + m[0] - 'a',
    to   = 8 * (8 - m[3] + '0') + m[2] - 'a';
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
    BoardGameRoll (move);
    game_print ();
    return server_send (client, "S");
  }

  server_error (client, "error : server cannot find client's move");
  return 0;
}

/*
.. global variables (a) to see if a game is running, (b) which client
.. is actively communicating with this server
*/ 
static int game_running = 0;
static ws_cli_conn_t game_client = (ws_cli_conn_t) UINT64_MAX;

/*
.. main server function, that decodes the message friom the client and
.. send back appropriate responses or error/warnign messages 
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
  .. g: game status 
  .. d: debug flag (fixme)
  */

  if (type == WS_FR_OP_BIN)
    return server_error (client, "error : binary msg not expected");
  printf ("\n{client >> server : %s}", msg);
  if (client != game_client)
    return server_error (
      client, "error : game running in another tab. refresh this page"
    );
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
    return game_engine (client, msg);
  if (cmd == 'u') 
    return game_undo (client, msg);
  if (cmd == 'g') 
    return game_status (client, msg);
  return server_msg_unknown (client, msg);
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

  printf ("connection opened, addr: %s, port: %s\n", cli, port);

  if (game_running)
    server_send_fen (client);
  else  if (game_set (client, "f" /* default fen */))
    game_running = 1;

  game_client = client;
}

/**
 * @brief Called when a client disconnects to the server.
 *
 * @param client Client connection. The @p client parameter is used
 * in order to send messages and retrieve informations about the
 * client.
 */

void onclose (ws_cli_conn_t client)
{
  char *cli;
  cli = ws_getaddress(client);

  printf("connection closed, addr: %s\n", cli);
  if (game_client == client)
    game_client = (ws_cli_conn_t) UINT64_MAX;
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

int is_port_available (uint32_t port) 
{
  int sock = socket (AF_INET, SOCK_STREAM, 0);
  if (sock < 0)
    return 0;
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl (INADDR_ANY);
  addr.sin_port = htons (port);

  int result = bind (sock, (struct sockaddr*) & addr, sizeof(addr));
  close (sock);
  return result == 0;
}

/**
 * @brief Main routine.
 *
 * @note After invoking @ref ws_socket, this routine never returns,
 * unless if invoked from a different thread.
 */

int main (int argc, char * argv[])
{

  uint32_t port = 8080;
  if (argc > 1)
  {
    const char * wsport = argv [1], * str;
    if ( (str = strchr (wsport, ':')) == NULL ||
         (str = strchr (str+1, ':')) == NULL )
    {
      fprintf (stdout, "wrong wsserver link");
      exit (-1); 
    }
    port = 0;
    while (*++str != '\0')
    {
      assert (*str >= '0' || *str <= '9');
      port = port * 10 + (*str - '0');
    }
  }
  if ( port < 1024 || ! is_port_available (port) )
  {
    fprintf (stderr, "port %u not available\n", port);
    exit (-1);
  }

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

  return 0;
}
