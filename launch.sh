#!/bin/bash

# find an available port for the WebSocket server
WS_PORT=$(cd src && make port >/dev/null && ./port)
if [ -z "$WS_PORT" ]; then
    echo "Failed to obtain WebSocket port" >&2
    exit 1
fi
# generate JavaScript configuration
echo "const WS_PORT = \"$WS_PORT\";" > ./client/config.js

# launch the WebSocket server
(cd src && make server && ./server $WS_PORT) &
SERVER_PID=$!

echo "WebSocket port: $WS_PORT"
echo "WebSocket server PID: $SERVER_PID"

# open an http server to serve the pages
python3 -m http.server 8000 &
HTTP_PID=$!

# now, you can kill both the processes by ctrl+c
cleanup()
  {
    kill "$SERVER_PID" "$HTTP_PID" 2>/dev/null
  }
trap cleanup EXIT INT TERM

# once  the "./server" is launched let's wait a bit before the webpage is opened
sleep 0.2
xdg-open http://127.0.0.1:8000/client/client.html

wait "$SERVER_PID"
