# Whot Card Game

Online multiplayer Whot card game: C++20 backend with HTTP + WebSocket API and a static web frontend. Supports hosting rooms with a join code, playing with AI bots, reconnection, persistence, and an optional leaderboard.

## Quick start (local)

### Build and run the server

```bash
# Dependencies (Debian/Ubuntu)
sudo apt-get install -y build-essential cmake libboost-system-dev \
    libsqlite3-dev nlohmann-json3-dev libwebsocketpp-dev

# Dependencies (macOS with Homebrew)
brew install cmake boost sqlite3 nlohmann-json websocketpp

# Dependencies (Windows with vcpkg)
vcpkg install boost-system sqlite3 nlohmann-json websocketpp

# Build (CMake 3.15+)
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --target whot_server

# Run (HTTP on 8081, WebSocket on 8082 by default)
./bin/whot_server --http-port 8081 --ws-port 8082 --static-path ../web
```

### Serve the frontend

- **Option A**: Point your browser at the HTTP server (same origin for API and WS):
  - Open `http://localhost:8081` — the server serves the `web/` static files and the API. WebSocket connects to `ws://localhost:8081/ws` only if you run a single-port setup (see below).
- **Option B**: For separate ports (default build), the app expects HTTP on 8081 and WS on 8082. Either:
  - Open `http://localhost:8081` and ensure the frontend is configured to use `apiBase: 'http://localhost:8081'` and `wsUrl: 'ws://localhost:8082'` (e.g. via `data-api-base` / `data-ws-url` on the script tag, or `window.WHOT_CONFIG` in `web/js/runtime-config.js`), or
  - Use a local reverse proxy (e.g. Nginx or Caddy) that serves the app on one port and proxies `/api` and `/ws` to the backend.

Example for same-machine dev with default ports — set in `web/js/runtime-config.js` before loading game.js:

```js
window.WHOT_CONFIG = { apiBase: 'http://localhost:8081', wsUrl: 'ws://localhost:8082' };
```

Then open `web/index.html` from the file system or serve `web/` on any port; the client will use the above URLs.

## Docker

Build and run (single port via Nginx proxy):

```bash
docker build -t whot .
docker run -p 8080:8080 whot
```

Open `http://localhost:8080`. The container runs the Whot server (HTTP 8081, WS 8082) and Nginx on `PORT` (default 8080), proxying `/api` and `/` to HTTP and `/ws` to the WebSocket server.

## Railway deployment

- Set **PORT** (Railway sets this automatically).
- Optionally set **PUBLIC_URL** (e.g. `https://your-app.railway.app`). The start script will set API and WS URLs for the frontend so the same build works behind the proxy.
- Or set **API_BASE** and **WS_URL** explicitly (e.g. `https://your-app.railway.app` and `wss://your-app.railway.app/ws`).

The Docker image uses `deploy/start.sh`: it starts the Whot server, then Nginx with `daemon off` on `PORT`. Logs from `whot_server` go to the process stdout (visible in Railway logs).

## Runtime config (API / WebSocket URLs)

- **Local**: Rely on defaults in `web/js/game.js` (current origin + `/ws`) or set `window.WHOT_CONFIG = { apiBase, wsUrl }` in `web/js/runtime-config.js` (loaded before `game.js`).
- **Docker/Railway**: Set `PUBLIC_URL` or `API_BASE`/`WS_URL`; `start.sh` writes `web/js/runtime-config.js` from `deploy/runtime-config.injected.js.template` so the same static assets work in all environments.

## Smoke checklist

1. Create a game (Create room) → note the join code.
2. Join by code from another browser/incognito → see waiting room.
3. Start game (host) → play a round (play/draw, last card, drag-and-drop or click cards).
4. Disconnect (close tab) and reopen → rejoin with same code; state should restore.
5. Play with bots → create bot game, auto-start, play to game over.
6. Game over → winner and scores; leaderboard at `GET /api/leaderboard`.

## API overview

- `POST /api/games` — create game (body: `playerName`, optional `botCount`, `minPlayers`, `maxPlayers`).
- `POST /api/games/join` — join by code (body: `gameCode`, `playerName`).
- `GET /api/games` — list active games (no join codes).
- `GET /api/games/:id` — game state (JSON).
- `GET /api/leaderboard?limit=50` — top players by wins / score.
- `GET /api/health` — health check.
- WebSocket `/ws` — connect then send typed JSON messages: `JOIN_GAME`, `START_GAME`, `PLAY_CARD`, `DRAW_CARD`, `CHOOSE_SUIT`, `LEAVE_GAME`, and others (41 message types defined in `include/Network/MessageProtocol.hpp`).

## Architecture notes

- **Language**: C++20 (GCC 10+ or Clang 11+).
- **Message protocol**: JSON-serialised typed messages over WebSocket text frames.  The 41-variant `MessageType` enum covers the full game lifecycle without resorting to free-form string parsing.
- **AI opponents**: Four configurable difficulty levels backed by four strategy classes (Random, Aggressive, Defensive, Balanced) using heuristic card-value scoring.  Difficulty adjusts hand-tracking accuracy and random noise rather than strategy class alone.
- **Database**: SQLite backend (default) via a repository-pattern abstraction.  All queries that incorporate external input use parameterised `sqlite3_prepare_v2` / `sqlite3_bind_*` statements; there is no string concatenation of user data into SQL.  The `DatabaseFactory` interface is designed to accept alternative backends.
- **Session management**: Sessions are created on WebSocket connect and destroyed on disconnect or timeout (default 60 s idle).  A 30 s PING heartbeat keeps connections alive through NAT tables.

## Known limitations and future work

- **Auth**: Guest names only; no accounts or sign-in.
- **Chat**: Not implemented.
- **Advanced rules**: Declaration penalties and turn timers exist in the engine but are not fully exposed in the UI.
- **Rate limiting**: Consider adding for `/api/games` and `/api/games/join` (or at the proxy).

## License

See repository license file.
