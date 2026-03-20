# Project Structure Reference

```
whot/
├── CMakeLists.txt              Build system entry point (CMake 3.15+, C++20)
├── Makefile                    Convenience targets: build, test, run, docs, format, lint
├── Dockerfile                  Multi-stage build: builder (Ubuntu 22.04) -> runtime + Nginx
├── railway.json                Railway.app deployment config (V2 runtime, 1 replica)
├── README.md                   Quick-start guide and architecture notes
│
├── include/                    Public C++ headers (27 files across 7 modules)
│   ├── Application.hpp         Top-level orchestrator: HTTP, WebSocket, AI, persistence
│   ├── AI/
│   │   ├── AIPlayer.hpp        Bot player: decideAction, chooseCard, chooseSuit, delays
│   │   ├── DifficultyLevel.hpp Enum + helper: EASY / MEDIUM / HARD / RANDOM
│   │   └── Strategy.hpp        Abstract Strategy base + Random/Aggressive/Defensive/Balanced
│   ├── Core/
│   │   ├── Card.hpp            Card value, suit, special ability; JSON serialisation
│   │   ├── Deck.hpp            Full Whot deck; shuffle, draw, reshuffle from discard
│   │   ├── GameConstants.hpp   Magic numbers: starting cards, max players, score limits
│   │   ├── Hand.hpp            Card collection; play by index; hand score calculation
│   │   └── Player.hpp          Player identity, hand, stats, turn flags, timers
│   ├── Game/
│   │   ├── ActionTypes.hpp     GameAction, ActionResult, ActionType enum
│   │   ├── GameEngine.hpp      processAction dispatcher; event callback registry
│   │   ├── GameState.hpp       All mutable game state; JSON serialisation (full + per-player)
│   │   ├── RuleEngine.hpp      canPlayCard, mustDrawCard, calculateDrawCount, etc.
│   │   ├── ScoreCalculator.hpp Hand score, round winner, game winner, elimination
│   │   └── TurnManager.hpp     Turn lifecycle, skip queue, multi-action, timer
│   ├── Network/
│   │   ├── HTTPServer.hpp      Embedded HTTP server; addRoute, addPatternRoute, static files
│   │   ├── MessageProtocol.hpp Message struct; 41-variant MessageType enum; serialize/parse
│   │   ├── SessionManager.hpp  Session CRUD; activity tracking; expired-session cleanup
│   │   └── WebSocketServer.hpp websocketpp wrapper; heartbeat; connect/disconnect hooks
│   ├── Persistence/
│   │   ├── Database.hpp        Abstract DB interface + SqlParam variant; DatabaseFactory
│   │   ├── GameRepository.hpp  CRUD for GameState in games and game_players tables
│   │   └── PlayerRepository.hpp CRUD for Player stats in players and player_stats tables
│   ├── Rules/
│   │   └── NigerianRules.hpp   Nigerian Whot rule variant interface
│   └── Utils/
│       ├── JSONSerializer.hpp  Helpers for composing JSON without a full parse cycle
│       ├── Logger.hpp          5-level thread-safe logger with file + console sinks
│       ├── Random.hpp          Thread-safe RNG; UUID/ID generation
│       └── Validation.hpp      Input sanitisation helpers
│
├── src/                        C++ implementation (30 files)
│   ├── Application.cpp         HTTP routes, WS handlers, game lifecycle, bot execution
│   ├── AI/
│   │   ├── AIPlayer.cpp        decideAction: draw or play, with human-like delay
│   │   ├── DifficultyLevel.cpp maps difficulty -> strategy class + randomness factor
│   │   └── Strategy.cpp        evaluateCardValue; selectCard / selectSuit per strategy
│   ├── Core/
│   │   ├── Card.cpp            getSpecialAbility; canPlayOn; JSON round-trip
│   │   ├── Deck.cpp            Full 54-card Whot deck; supports multi-deck games
│   │   ├── GameConstants.cpp   Named constant definitions
│   │   ├── Hand.cpp            addCard, playCard (by index), calculateTotalScore
│   │   └── Player.cpp          Stat mutations; turn flags; JSON round-trip
│   ├── Game/
│   │   ├── GameEngine.cpp      processAction → handlePlayCard/DrawCard/Declaration/SuitChoice
│   │   │                       executeSpecialCard: HOLD_ON / PICK_TWO / FIVE / EIGHT /
│   │   │                       GENERAL_MARKET / WHOT_CARD effects all inline here
│   │   ├── GameState.cpp       initialize, startRound, endRound, checkRoundEnd;
│   │   │                       toJson (full) + toJsonForPlayer (hides other hands)
│   │   ├── RuleEngine.cpp      NigerianRules delegation; draw-count chain logic
│   │   ├── ScoreCalculator.cpp hand score = sum of card face values; elimination threshold
│   │   └── TurnManager.cpp     startTurn / endTurn; skip queue; canPlayAgain; timer
│   ├── Network/
│   │   ├── HTTPServer.cpp      HTTP/1.1 server; route table; static file serving
│   │   ├── MessageProtocol.cpp Message::serialize / deserialize (JSON text frames)
│   │   ├── SessionManager.cpp  UUID session IDs; activity timestamps; expired removal
│   │   └── WebSocketServer.cpp websocketpp WsServerImpl; asio heartbeat + timeout timers;
│   │                           connect / disconnect hook dispatch
│   ├── Persistence/
│   │   ├── Database.cpp        SQLiteDatabase: connect, execute, executeBound (parameterised),
│   │   │                       queryOneBound, queryManyBound, initializeSchema (4 tables)
│   │   ├── GameRepository.cpp  saveGame / loadGame / deleteGame / getActiveGames
│   │   └── PlayerRepository.cpp savePlayer / loadPlayer / getPlayerStats / getLeaderboard
│   ├── Rules/
│   │   ├── BaseRule.cpp        Default implementations shared across variants
│   │   ├── EnglishRules.cpp    English Whot rule overrides
│   │   ├── NigerianRules.cpp   Nigerian variant: 2s defend 2s, 5→pick3, 8→suspend, etc.
│   │   └── RuleVariant.cpp     Factory / registry for rule variants
│   └── Utils/
│       ├── JSONSerializer.cpp  Append-style JSON builder for performance-sensitive paths
│       ├── Logger.cpp          Thread-safe file + console output; configurable format
│       ├── Random.cpp          Mersenne Twister RNG; generateId using hex alphabet
│       └── Validation.cpp      Sanitise player names, game codes, card indices
│
├── tests/                      33 test files using Google Test
│   ├── TestMain.cpp            Google Test main entry
│   ├── TestHelpers.hpp/.cpp    In-memory DB config and zero-port server helpers
│   ├── TestIntegration.cpp     End-to-end: create game, join, play, leave, reconnect
│   ├── TestBots.cpp            AI bot turn execution in a full game loop
│   ├── TestGameCode.cpp        Game code normalisation and uniqueness
│   ├── TestGameplayFlows.cpp   Full round flows: special cards, win conditions
│   ├── TestStartGame.cpp       Lobby-to-active-game transitions
│   ├── AI/                     TestAIPlayer, TestDifficultyLevel, TestStrategy
│   ├── Core/                   TestCard, TestDeck, TestGameConstants, TestHand, TestPlayer
│   ├── Game/                   TestGameEngine, TestGameState, TestRuleEngine,
│   │                           TestScoreCalculator, TestTurnManager
│   ├── Network/                TestHTTPServer, TestMessageProtocol,
│   │                           TestSessionManager, TestWebSocketServer
│   ├── Persistence/            TestDatabase, TestGameRepository,
│   │                           TestNameRepository, TestPlayerRepository
│   ├── Rules/                  TestNigerianRules
│   └── Utils/                  TestJSONSerializer, TestLogger, TestRandom, TestValidation
│
├── web/                        Static web frontend
│   ├── index.html              Single-page app shell; modal dialogs for join/bot options
│   ├── css/style.css           Game board layout; card styles; drag-and-drop states;
│   │                           responsive breakpoints at 768px and 480px
│   ├── js/
│   │   ├── game.js             WhotGameClient class: WebSocket lifecycle, game rendering,
│   │   │                       click and drag-and-drop card play, turn timer, reconnection
│   │   └── runtime-config.js   window.WHOT_CONFIG injection point for API/WS URLs
│   └── assets/images/          88 SVG card images (BLOCK/CIRCLE/CROSS/STAR/TRIANGLE suits
│                               + WHOT_20, back, blank, favicon)
│
├── deploy/
│   ├── start.sh                Container entrypoint: start whot_server then Nginx
│   ├── nginx.railway.conf.template  Nginx reverse-proxy template; substitutes ${PORT}
│   └── runtime-config.injected.js.template  Generates window.WHOT_CONFIG from env vars
│
├── scripts/
│   ├── clean_rebuild.sh        Remove build/ and rebuild from scratch
│   └── cmake-wrapper.sh        Thin wrapper for IDEs that need an explicit cmake call
│
└── .github/workflows/
    ├── ci.yml                  Build + test on ubuntu-latest for every push/PR
    └── deploy-pages.yml        Build frontend with production URLs; deploy to GitHub Pages
```

## Database schema (4 normalised tables)

| Table | Primary key | Purpose |
|-------|-------------|---------|
| `schema_version` | version | Migration tracking |
| `games` | game_id | Serialised GameState JSON + status + timestamps |
| `players` | player_id | Player name + creation time |
| `player_stats` | player_id (FK) | total_games, games_won, total_score, last_played |
| `game_players` | (game_id, player_id) | Many-to-many game membership |

All queries that incorporate external input use `sqlite3_prepare_v2` + `sqlite3_bind_*` (no string concatenation).
