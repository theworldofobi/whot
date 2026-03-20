# Whot — Implementation Manual

## 1. Overview

Whot is a production-grade online multiplayer card game server written in C++20. It implements the Nigerian Whot card game with real-time WebSocket gameplay, HTTP REST game management, four-level AI opponents, SQLite persistence, and a static HTML/JavaScript frontend.

The system is deployed as a single Docker container on Railway.app behind an Nginx reverse proxy that unifies HTTP and WebSocket traffic on one port.

---

## 2. Architecture

### 2.1 Layer diagram

```
┌─────────────────────────────────────────────────────┐
│  Web Frontend  (HTML / CSS / JS)                    │
│  WhotGameClient — WebSocket + REST over HTTP        │
└───────────────────────┬─────────────────────────────┘
                        │ WS / HTTP
┌───────────────────────▼─────────────────────────────┐
│  Application  (Application.cpp)                     │
│  Orchestrates: routing, game lifecycle, bot turns   │
├────────────────┬──────────────┬─────────────────────┤
│ Network layer  │  Game layer  │  Persistence layer  │
│  WebSocket     │  GameEngine  │  PlayerRepository   │
│  HTTP server   │  GameState   │  GameRepository     │
│  Session mgr   │  RuleEngine  │  SQLiteDatabase     │
│  Msg protocol  │  TurnManager │                     │
│                │  Scorer      │                     │
├────────────────┴──────────────┴─────────────────────┤
│  Core layer: Card, Deck, Hand, Player               │
├─────────────────────────────────────────────────────┤
│  AI layer: AIPlayer, DifficultyLevel, Strategy      │
├─────────────────────────────────────────────────────┤
│  Utils: Logger, Random, Validation, JSONSerializer  │
└─────────────────────────────────────────────────────┘
```

### 2.2 Key design decisions

- **MVC-style separation**: Core data (Card/Deck/Hand/Player) knows nothing about networking or persistence. GameEngine processes actions and emits events; Application translates those events into WebSocket messages.
- **Strategy pattern for AI**: Four strategies (Random, Aggressive, Defensive, Balanced) implement a common interface; `DifficultyLevel` selects the strategy and adds controlled randomness.
- **Repository pattern for persistence**: `PlayerRepository` and `GameRepository` depend on the abstract `Database` interface so the underlying storage can change without touching business logic. All queries use parameterised `sqlite3_prepare_v2` / `sqlite3_bind_*` statements.
- **Factory pattern for database backends**: `DatabaseFactory::create(config)` returns the concrete `SQLiteDatabase`; additional backends (PostgreSQL, MySQL) can be added by implementing `Database` and registering a new factory branch.
- **Event callbacks in GameEngine**: Callers register lambdas for named events (`card_played`, `round_ended`, etc.); Application translates these into typed `Message` objects and broadcasts them via `WebSocketServer::sendToGame()`.

---

## 3. Core module

### 3.1 Card

`Card` stores a `Suit` (BLOCK, CIRCLE, CROSS, STAR, TRIANGLE, WHOT) and `CardValue` (1–14, 20). `getSpecialAbility()` derives `SpecialAbility` from the value:

| Value | Ability |
|-------|---------|
| 1 | HOLD_ON — current player plays again |
| 2 | PICK_TWO — next player draws 2 |
| 5 | (rule-level: next player draws 3) |
| 8 | (rule-level: next player is suspended) |
| 14 | GENERAL_MARKET — all other players draw 1 |
| 20 | WHOT_CARD — player calls any suit |

`canPlayOn(callCard, demandedSuit)` returns true if the card matches the current top-of-pile by suit or value, or is a Whot card.

### 3.2 Deck

The full Whot deck is 54 cards across five suits with non-contiguous value ranges (some suits omit 6, 9, or other values). `Deck` supports multiple decks via the `numberOfDecks` constructor parameter. Shuffling uses `std::shuffle` with a seeded Mersenne Twister from `utils::Random`.

### 3.3 Hand

`Hand` is a `std::vector<std::unique_ptr<Card>>` with `addCard`, `playCard(index)` (removes and returns ownership), and `calculateTotalScore` (sum of face values). Size and card access are bounds-checked.

### 3.4 Player

`Player` aggregates identity (id, name, type), a `Hand`, score counters (`currentScore_`, `cumulativeScore_`), lifetime stats (`gamesPlayed_`, `gamesWon_`), turn-declaration flags (`saidLastCard_`, `saidCheckUp_`), and a `lastActionTime_` for turn-timer enforcement. JSON round-trip is provided by `toJson()` / `fromJson()`. Persistence restoration uses `setGamesPlayed`, `setGamesWon`, `setCumulativeScore` — the only setters in this module, intentionally restricted to the persistence layer.

---

## 4. Game module

### 4.1 GameState

`GameState` holds the full mutable state of one game: active player list, deck, discard pile, call card, demanded suit, direction (clockwise/counter-clockwise), active pick count, and game phase (`WAITING`, `STARTING`, `IN_PROGRESS`, `ROUND_ENDED`, `GAME_ENDED`).

`toJsonForPlayer(playerId)` serialises the state with other players' hand sizes rather than card lists, so the server never leaks opponents' cards.

Key state transitions:

```
WAITING  →[startGame]→  STARTING  →[startNewRound]→  IN_PROGRESS
IN_PROGRESS  →[checkRoundEnd]→  ROUND_ENDED  →[startNewRound]→  IN_PROGRESS
IN_PROGRESS  →[endGame]→  GAME_ENDED
```

### 4.2 GameEngine

`GameEngine` owns a `GameState` and a `RuleEngine`. `processAction(action)` dispatches to one of four private handlers:

| Handler | Triggered by |
|---------|-------------|
| `handlePlayCard` | `PLAY_CARD` action |
| `handleDrawCard` | `DRAW_CARD` action |
| `handleDeclaration` | `DECLARE_LAST_CARD`, `DECLARE_CHECK_UP` |
| `handleSuitChoice` | `CHOOSE_SUIT` |

After a card is played, `executeSpecialCard` applies in-place effects to `GameState` (skip next player, increment pick count, distribute cards, clear demanded suit). Round-end detection and score accumulation happen at the end of `handlePlayCard`.

### 4.3 RuleEngine

`RuleEngine` delegates to `NigerianRules` (default). Key methods:

- `canPlayCard(state, player, card)` — validates suit/value match, pick-chain defense rules, and demanded-suit override.
- `mustDrawCard(state, player)` — true when the active pick count > 0 and the player cannot defend.
- `calculateDrawCount(state, player)` — returns the accumulated pick count.
- `requiresLastCardDeclaration(player)` — true when the player has exactly one card.
- `requiresCheckUpDeclaration(player)` — true when an opponent is at last card.

### 4.4 TurnManager

`TurnManager` tracks whose turn it is (via `GameState::getCurrentPlayer`), manages a skip queue, and records a `startTime_` for turn-timer enforcement. `enableMultipleActions()` (used by HOLD_ON) allows the current player to play again before `endTurn()` is called. Action history is capped at 100 entries.

### 4.5 ScoreCalculator

Hand scores are the sum of remaining card face values. Players are eliminated when their cumulative score reaches the configured threshold. The round winner is the player with the lowest score (or the first to empty their hand). The game winner is the player with the most rounds won.

---

## 5. Rules module

Three concrete rule classes implement a shared virtual interface:

- **NigerianRules**: Full Nigerian Whot — 2s defend 2s, 5s pick 3, 8s suspend, 14 general market, Whot card with suit choice and optional direction reversal.
- **EnglishRules**: Simplified English variant without some special cards.
- **BaseRule**: Default implementations; concrete classes override only what they change.

`RuleVariant` provides a factory that constructs the appropriate class by name string.

---

## 6. Network module

### 6.1 WebSocketServer

`WebSocketServer` wraps `websocketpp::server<websocketpp::config::asio>` inside a private `WsServerImpl` struct. The server runs on its own thread. Two `boost::asio::steady_timer` instances (created after `init_asio()`) drive:

- **Heartbeat**: fires every `heartbeatInterval_` seconds (default 30), sends WebSocket PING frames to all open connections. PING keeps NAT tables and proxy keepalive alive without application-level polling.
- **Timeout check**: fires at half the `timeout_` interval (default every 30 s), calls `checkTimeouts()`, which removes expired sessions from `SessionManager` and sends WebSocket close frames to their connections.

Every incoming message refreshes the session's `lastActivity` timestamp via `SessionManager::updateActivity`.

`WebSocketServer` exposes `setConnectionHandler` and `setDisconnectionHandler` so `Application` can react to transport events. The disconnection handler fires before `destroySession`, so the session game/player IDs are still readable at callback time.

### 6.2 MessageProtocol

`Message` carries a `MessageType` (41-variant enum), a `payload` JSON string, and optional `sessionId`. `serialize()` produces a JSON text frame; `deserialize()` parses one. The enum covers the complete game lifecycle:

```
JOIN_GAME, LEAVE_GAME, START_GAME, GAME_STATE_UPDATE,
PLAY_CARD, DRAW_CARD, CHOOSE_SUIT, CARD_PLAYED, CARD_DRAWN,
ROUND_ENDED, GAME_ENDED, PLAYER_JOINED, PLAYER_LEFT,
PLAYER_DISCONNECTED, ERROR, PING, PONG, ...
```

### 6.3 SessionManager

Each WebSocket connection gets a `Session` with a 24-character random hex ID (`utils::Random::generateId`), IP address, associated `playerId` and `gameId`, and `lastActivity` timestamp. `removeExpiredSessions(timeoutSeconds)` returns the removed session IDs so the caller can close the corresponding WS handles.

### 6.4 HTTPServer

A lightweight embedded HTTP/1.1 server with a route table supporting exact-match and `:param` pattern routes. Static file serving from a configured root directory. CORS headers are added by the WebSocket server's HTTP handler for preflight requests.

---

## 7. AI module

### 7.1 AIPlayer

`AIPlayer::decideAction(state)` returns the `GameAction` the bot will take. It:

1. Collects playable cards using `RuleEngine::canPlayCard`.
2. If no card is playable, returns a `DRAW_CARD` action.
3. Otherwise, delegates card selection to the configured `Strategy`.
4. If the selected card is a Whot card, calls `chooseSuitForWhotCard` (picks the suit most represented in the bot's remaining hand).
5. Inserts a configurable thinking delay (0–1500 ms, scaled by difficulty) to avoid instant mechanical play.

### 7.2 DifficultyLevel

| Level | Strategy class | Randomness |
|-------|---------------|------------|
| EASY | Random | High |
| MEDIUM | Balanced | Medium |
| HARD | Aggressive or Defensive (alternates) | Low |
| RANDOM | Random | Full random |

### 7.3 Strategy

| Strategy | selectCard behaviour |
|----------|---------------------|
| `RandomStrategy` | Uniform random from playable cards |
| `AggressiveStrategy` | Highest `evaluateCardValue` (GENERAL_MARKET=14, WHOT=12, PICK_TWO=10, …) |
| `DefensiveStrategy` | Lowest `evaluateCardValue` to minimise score if the round ends early |
| `BalancedStrategy` | Aggressive when hand ≤ 3 cards (trying to empty), defensive otherwise |

`evaluateCardValue` assigns integer weights to special abilities; regular cards are worth their face value.

---

## 8. Persistence module

### 8.1 Database abstraction

`Database` declares a pure-virtual interface with three query categories:

- **Schema-safe queries** (`execute`, `queryOne`, `queryMany`): for DDL and literal-value queries with no external input.
- **Parameterised queries** (`executeBound`, `queryOneBound`, `queryManyBound`): accept `std::vector<SqlParam>` where `SqlParam = std::variant<std::string, int64_t>`. The `SQLiteDatabase` implementation binds each variant element using `sqlite3_bind_text` or `sqlite3_bind_int64`.

All repository methods that accept player IDs, game IDs, names, or search strings use the parameterised path. Integer parameters (LIMIT, timestamps) are bound as `int64_t`.

### 8.2 Schema

```sql
CREATE TABLE IF NOT EXISTS games (
    game_id   TEXT PRIMARY KEY,
    game_state TEXT,          -- full GameState JSON
    rule_variant TEXT,
    created_at INTEGER,
    updated_at INTEGER,
    status TEXT               -- 'active', 'round_ended', 'ended', 'archived'
);

CREATE TABLE IF NOT EXISTS players (
    player_id   TEXT PRIMARY KEY,
    player_name TEXT,
    created_at  INTEGER
);

CREATE TABLE IF NOT EXISTS player_stats (
    player_id  TEXT PRIMARY KEY REFERENCES players(player_id),
    total_games INTEGER DEFAULT 0,
    games_won   INTEGER DEFAULT 0,
    total_score INTEGER DEFAULT 0,
    last_played INTEGER
);

CREATE TABLE IF NOT EXISTS game_players (
    game_id   TEXT REFERENCES games(game_id),
    player_id TEXT REFERENCES players(player_id),
    PRIMARY KEY(game_id, player_id)
);
```

### 8.3 Game state serialisation

`GameState::toJson()` serialises the entire game — all players with their full hands — to a JSON string. This is what gets stored in `games.game_state`. On `loadGame`, `GameState::fromJson` reconstructs the state tree including all `Player` and `Card` objects, so a crash-restarted server can resume any active game.

---

## 9. Application module

`Application` is the top-level coordinator. Its lifecycle:

```
Application::initialize()
  setupDatabase()          — open SQLite, run initializeSchema()
  setupWebSocketHandlers() — create WS server, register message + disconnect handlers
  setupHttpRoutes()        — register REST routes on HTTP server
  loadExistingGames()      — restore active games from DB after restart

Application::run()
  wsServer_->start()       — WS server thread begins (heartbeat + timeout timers start)
  httpServer_->start()     — HTTP server thread begins
  (blocks until shutdown)
```

### 9.1 Game lifecycle via REST

| Step | Endpoint | Handler |
|------|----------|---------|
| Create | `POST /api/games` | `handleCreateGame` |
| Join by code | `POST /api/games/join` | `handleJoinByCode` |
| Get state | `GET /api/games/:id` | `handleGetGame` |
| Leaderboard | `GET /api/leaderboard` | `handleLeaderboard` |

### 9.2 Game lifecycle via WebSocket

After connecting, the client sends typed JSON messages:

```
→ JOIN_GAME   {gameId, playerName}
← GAME_STATE_UPDATE + PLAYER_JOINED broadcasts

→ START_GAME  {gameId}
← GAME_STATE_UPDATE to all players

→ PLAY_CARD   {gameId, cardIndex, [chosenSuit]}
← CARD_PLAYED + GAME_STATE_UPDATE broadcasts
← ROUND_ENDED / GAME_ENDED if applicable

→ DRAW_CARD   {gameId}
← CARD_DRAWN + GAME_STATE_UPDATE

→ CHOOSE_SUIT {gameId, suit}
← GAME_STATE_UPDATE

→ LEAVE_GAME  {gameId}
← PLAYER_LEFT broadcasts; game deleted if empty
```

### 9.3 Bot execution

After every human action, `Application::runBotTurnsIfNeeded()` loops (up to 50 iterations) and executes `AIPlayer::decideAction` for any bot whose turn it is. This runs synchronously within the WS message handler thread, blocking further incoming messages during bot computation. The 50-iteration cap prevents an infinite loop if the game state is inconsistent.

### 9.4 Disconnect handling

When a WebSocket connection drops, the `DisconnectionHandler` registered in `setupWebSocketHandlers` calls `leaveGame(gameId, playerId)`. This removes the player, broadcasts `PLAYER_LEFT`, and deletes the game if no humans remain. Because the handler fires before `destroySession`, the session's gameId and playerId are still readable.

---

## 10. Frontend

`WhotGameClient` (in `web/js/game.js`) manages the WebSocket connection to the server and renders the game board.

### 10.1 Connection and reconnection

On `WebSocket.onclose`, the client waits an exponentially increasing interval (500 ms, 1 s, 2 s, 4 s, … up to 30 s) before reconnecting. Session state (player name, game code) is preserved in the client object so a reconnect seamlessly re-joins the same game.

### 10.2 Card interaction

Cards in the player's hand support:

- **Click**: immediate play attempt via `playCard(index)`.
- **HTML5 drag-and-drop**: `draggable=true` + `dragstart`/`dragend` on the card; `dragover`/`drop` on `#play-area`. The card index is stored in `dataTransfer` and read on drop.
- **Touch drag**: `touchstart` records finger position; `touchend` checks whether the finger was released inside `#play-area` after horizontal movement, and calls `playCard` if so.

### 10.3 Whot card suit selection

When a Whot card (value 20) is played, the UI shows a suit-choice modal. The player selects a suit; the client sends `CHOOSE_SUIT` with the selection. The game engine's `handleSuitChoice` sets `demandedSuit` on the game state, which is then broadcast to all players.

### 10.4 Turn timer

`document.getElementById('turn-timer')` is updated from `gameState.turnTimeRemaining` in the `GAME_STATE_UPDATE` handler. The server's `TurnManager` calculates `getRemainingTime()` based on `steady_clock` elapsed since `startTurn()`.

### 10.5 Responsive layout

Two CSS breakpoints adapt the layout:

- **768px**: header stacks, cards reduce to 64×96 px, play area goes vertical, opponents wrap.
- **480px**: cards reduce to 52×78 px, text and buttons scale for thumb reach.

---

## 11. Build system

`CMakeLists.txt` targets C++20 with strict compiler flags (`-Wall -Wextra -Wpedantic -Werror`). WebSocketPP is fetched at 0.8.2 via FetchContent if not found on the system; nlohmann/json and Google Test are similarly managed. The WebSocket translation unit is compiled at C++17 due to websocketpp template compatibility, all other units use C++20.

```bash
# Debug build with tests
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
make -j$(nproc)
ctest --output-on-failure

# Release build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc) whot_server
```

Optional targets: `make docs` (Doxygen), `make format` (clang-format), `make lint` (cppcheck).

---

## 12. Deployment

### 12.1 Local (without Docker)

See `README.md` quick-start section.

### 12.2 Docker

The multi-stage `Dockerfile` builds the server in an `ubuntu:22.04` builder stage (no build artifacts in the final image), then copies the binary, web assets, Nginx config, and start script into a minimal runtime stage. `deploy/start.sh` starts `whot_server` as a background process, injects `window.WHOT_CONFIG` from environment variables via the runtime-config template, then starts Nginx in the foreground so the container lives until Nginx exits.

### 12.3 Railway

`railway.json` specifies the Dockerfile builder, one replica in US-West-2, restart on failure (max 10 retries), and `sleepApplication: false`. Railway injects `PORT`; the Nginx config template substitutes it so Nginx binds to the correct port. `PUBLIC_URL`, `API_BASE`, and `WS_URL` environment variables control the frontend endpoint configuration.

---

## 13. Testing

33 test files use Google Test. Tests are organised to mirror the source tree:

- **Unit tests** cover Card, Deck, Hand, Player, GameState, GameEngine, RuleEngine, ScoreCalculator, TurnManager, AIPlayer, Strategy, Logger, Random, Validation, JSONSerializer, SessionManager, MessageProtocol, Database, PlayerRepository, GameRepository, NigerianRules.
- **Integration tests** (TestIntegration, TestGameplayFlows, TestBots, TestStartGame, TestGameCode) run full game flows through `Application` with an in-memory SQLite database and zero-bound port servers.

All tests that touch the database use `DatabaseConfig{.type = MEMORY}` so they leave no files on disk and run in parallel without conflict.

---

## 14. Logging

`Logger` writes to console (stdout) and optionally a file, protected by a `std::mutex`. Five severity levels: DEBUG, INFO, WARNING, ERROR, CRITICAL. Each log line optionally includes an ISO-8601 timestamp and the calling thread ID. The log level threshold, output targets, and timestamp format are configurable at runtime.

---

## 15. Known limitations and roadmap

| Area | Current state | Possible improvement |
|------|---------------|---------------------|
| Auth | Guest names only | OAuth 2.0 / JWT session tokens |
| PostgreSQL | Interface designed, SQLite only wired | Add `libpq`-backed `PostgreSQLDatabase` class |
| Rate limiting | None | Token bucket per IP at Application or Nginx layer |
| Spectator mode | Not implemented | Read-only WebSocket session with `toJsonForSpectator` |
| Declaration penalties | Engine supports, UI does not expose | Add timer-based auto-penalty |
| Chat | Not implemented | Dedicated `CHAT_MESSAGE` message type |
| Horizontal scaling | Single-process | Shared Redis session / game store |
