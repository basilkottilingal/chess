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
#endif

#ifndef PORT_START
#define PORT_START 8080
#endif
#ifndef PORT_END
#define PORT_END 8090
#endif

#define PORT_RESERVED 0

int is_port_available (int port) 
{
  int sock = socket (AF_INET, SOCK_STREAM, 0);
  if (sock < 0)
    return 0;
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl (INADDR_ANY);
  addr.sin_port = htons (port);

  int result = bind (sock, (struct sockaddr*) & addr, sizeof(addr));
  close(sock);
  return result == 0;
}

int main ()
{

  for (uint32_t port = PORT_START; port <= PORT_END; ++port) 
    if( is_port_available(port) )
    {
      fprintf (stdout, "ws://127.0.0.1:%u", port);
      return 0;
    }
  fprintf (stderr, "no port available in [%u, %u] \n", PORT_START, PORT_END);
  return 0;
}
