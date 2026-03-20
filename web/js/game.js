class WhotGameClient {
    constructor() {
        this.ws = null;
        this.gameId = null;
        this.playerId = null;
        this.gameState = null;
        this.gameCode = null;
        this.pendingJoinPayload = null;
        this.pendingAutoStartBots = false;
        this.autoStartSent = false;
        this.pendingBotGameView = false;
        this.sessionMode = null; // "human-host" | "human-join" | "bots"
        this.reconnectAttempts = 0;
        this.maxReconnectDelayMs = 30000;
        if (typeof window !== 'undefined') window.whotClient = this;
        this.initializeEventListeners();
    }

    onEnterLobby() {
        this.showScreen('lobby');
    }
    
    initializeEventListeners() {
        document.getElementById('start-human-game-btn')?.addEventListener('click', () => this.createHumanGame());
        document.getElementById('start-bot-game-btn')?.addEventListener('click', () => this.openBotOptionsModal());
        document.getElementById('open-join-modal-btn')?.addEventListener('click', () => this.openJoinModal());
        document.getElementById('join-code-submit-btn')?.addEventListener('click', () => this.joinByCodeFromModal());
        document.getElementById('join-code-cancel-btn')?.addEventListener('click', () => this.closeJoinModal());
        document.getElementById('bot-options-submit-btn')?.addEventListener('click', () => this.createBotGameFromModal());
        document.getElementById('bot-options-cancel-btn')?.addEventListener('click', () => this.closeBotOptionsModal());
        document.getElementById('join-code-input')?.addEventListener('keydown', (event) => {
            if (event.key === 'Enter') {
                event.preventDefault();
                this.joinByCodeFromModal();
            }
            if (event.key === 'Escape') {
                event.preventDefault();
                this.closeJoinModal();
            }
        });
        document.getElementById('join-code-modal')?.addEventListener('click', (event) => {
            if (event.target && event.target.id === 'join-code-modal') this.closeJoinModal();
        });
        document.getElementById('bot-count-modal')?.addEventListener('keydown', (event) => {
            if (event.key === 'Enter') {
                event.preventDefault();
                this.createBotGameFromModal();
            }
            if (event.key === 'Escape') {
                event.preventDefault();
                this.closeBotOptionsModal();
            }
        });
        document.getElementById('bot-options-modal')?.addEventListener('click', (event) => {
            if (event.target && event.target.id === 'bot-options-modal') this.closeBotOptionsModal();
        });
        document.getElementById('start-game-btn')?.addEventListener('click', () => this.startGame());
        document.getElementById('draw-card')?.addEventListener('click', () => this.drawCard());
        document.getElementById('declare-last')?.addEventListener('click', () => this.declareLast());
        document.getElementById('return-lobby')?.addEventListener('click', () => this.returnToLobby());
    }

    startGame() {
        if (!this.ws || this.ws.readyState !== WebSocket.OPEN) {
            this.showErrorToast('Connecting to game server. Please try Start again in a moment.');
            return;
        }
        this.flushPendingJoinMessage();
        this.sendMessage(109, {}); // START_GAME
    }

    getPlayerName() {
        try {
            const name = sessionStorage.getItem('whot_playerName') || 'Guest';
            return this.normalizePlayerName(name);
        } catch (e) {
            return 'Guest';
        }
    }

    normalizePlayerName(name) {
        const trimmed = (name || '').trim();
        if (!trimmed) return 'Guest';
        return trimmed.slice(0, 30);
    }

    normalizeGameCode(code) {
        const cleaned = (code || '').trim().toUpperCase();
        return cleaned.replace(/[^A-Z0-9]/g, '').slice(0, 12);
    }
    
    connect() {
        if (this.ws && (this.ws.readyState === WebSocket.OPEN || this.ws.readyState === WebSocket.CONNECTING)) {
            return;
        }
        const wsUrl = this.getWebSocketUrl();
        this.ws = new WebSocket(wsUrl);
        
        this.ws.onopen = () => {
            console.log('Connected to server');
            this.reconnectAttempts = 0;
            this.flushPendingJoinMessage();
            // Rejoin existing game after reconnect (same gameId + playerId).
            if (!this.pendingJoinPayload && this.gameId && this.playerId) {
                this.sendMessage(100, {
                    playerName: this.getPlayerName(),
                    gameId: this.gameId
                });
            }
            // If a bot game reconnects while still in lobby phase, allow one
            // fresh auto-start attempt after socket recovery.
            if (this.sessionMode === 'bots' && this.pendingAutoStartBots &&
                this.gameState && this.gameState.phase === 0) {
                this.autoStartSent = false;
            }
        };
        
        this.ws.onmessage = (event) => {
            const message = JSON.parse(event.data);
            this.handleMessage(message);
        };
        
        this.ws.onerror = (error) => {
            console.error('WebSocket error:', error);
        };
        
        this.ws.onclose = () => {
            console.log('Disconnected from server');
            const baseDelay = Math.min(this.maxReconnectDelayMs, 1000 * Math.pow(2, this.reconnectAttempts));
            const jitter = Math.floor(Math.random() * 500);
            const reconnectDelay = baseDelay + jitter;
            this.reconnectAttempts += 1;
            setTimeout(() => this.connect(), reconnectDelay);
        };
    }
    
    handleMessage(message) {
        switch(message.type) {
            case 200: // GAME_STATE_UPDATE
                this.updateGameState(message);
                break;
            case 201: // PLAYER_JOINED
                this.handlePlayerJoined(message);
                break;
            case 204: // CARD_PLAYED
                this.handleCardPlayed(message);
                break;
            case 206: // ROUND_ENDED
                this.handleRoundEnded(message);
                break;
            case 207: // GAME_ENDED
                this.handleGameEnded(message);
                break;
            case 208: // ERROR
                this.handleError(message);
                break;
        }
    }
    
    sendMessage(type, payload) {
        if (this.ws && this.ws.readyState === WebSocket.OPEN) {
            const message = {
                type: type,
                playerId: this.playerId,
                gameId: this.gameId,
                payload: JSON.stringify(payload),
                timestamp: Date.now()
            };
            this.ws.send(JSON.stringify(message));
        }
    }
    
    createHumanGame() {
        this.sessionMode = 'human-host';
        this.pendingAutoStartBots = false;
        this.autoStartSent = false;
        this.pendingBotGameView = false;
        this.createGameWithOptions({ botCount: 0 });
    }

    createBotGame(botCount = 0) {
        this.sessionMode = 'bots';
        this.pendingAutoStartBots = true;
        this.autoStartSent = false;
        this.pendingBotGameView = true;
        this.closeBotOptionsModal();
        this.createGameWithOptions({
            botCount: Math.max(0, Math.min(3, botCount))
        });
    }

    openBotOptionsModal() {
        const modal = document.getElementById('bot-options-modal');
        const select = document.getElementById('bot-count-modal');
        if (!modal) return;
        modal.style.display = 'flex';
        if (select) select.focus();
    }

    closeBotOptionsModal() {
        const modal = document.getElementById('bot-options-modal');
        if (!modal) return;
        modal.style.display = 'none';
    }

    createBotGameFromModal() {
        const botCountEl = document.getElementById('bot-count-modal');
        const botCount = (botCountEl && parseInt(botCountEl.value, 10)) || 0;
        this.createBotGame(botCount);
    }

    createGameWithOptions(options = {}) {
        const apiBase = this.getApiBase();
        const botCount = typeof options.botCount === 'number' ? options.botCount : 0;
        const isHumanGame = botCount === 0;
        const body = isHumanGame
            ? {
                playerName: this.getPlayerName(),
                minPlayers: 3,
                maxPlayers: 6,
                startingCards: 6,
                botCount: 0
            }
            : {
                playerName: this.getPlayerName(),
                maxPlayers: 4,
                startingCards: 6,
                botCount: Math.max(0, Math.min(3, botCount))
            };
        this.apiJson(`${apiBase}/api/games`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(body)
        })
        .then(data => {
            this.gameId = data.gameId;
            this.playerId = data.playerId;
            this.gameCode = data.gameCode || null;
            this.sendJoinGameMessage();
            if (this.sessionMode === 'human-host') {
                this.showWaitingRoom();
            } else if (this.sessionMode === 'bots') {
                this.showErrorToast('Starting bot game...');
            }
        })
        .catch(error => {
            console.error('Error creating game:', error);
            this.showErrorToast(error.message || 'Could not create game');
        });
    }

    sendJoinGameMessage() {
        const payload = {
            playerName: this.getPlayerName(),
            gameId: this.gameId
        };
        if (!this.ws || this.ws.readyState !== WebSocket.OPEN) {
            this.pendingJoinPayload = payload;
            return;
        }
        this.sendMessage(100, payload); // JOIN_GAME
        this.pendingJoinPayload = null;
    }

    flushPendingJoinMessage() {
        if (!this.pendingJoinPayload) return;
        if (!this.ws || this.ws.readyState !== WebSocket.OPEN) return;
        this.sendMessage(100, this.pendingJoinPayload); // JOIN_GAME
        this.pendingJoinPayload = null;
    }

    joinByCode(codeValue) {
        const code = this.normalizeGameCode(codeValue || '');
        if (!code) {
            this.showErrorToast('Enter a game code');
            return;
        }
        const apiBase = this.getApiBase();
        this.apiJson(`${apiBase}/api/games/join`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ gameCode: code, playerName: this.getPlayerName() })
        })
        .then(data => {
            this.sessionMode = 'human-join';
            this.pendingAutoStartBots = false;
            this.autoStartSent = false;
            this.pendingBotGameView = false;
            this.gameId = data.gameId;
            this.playerId = data.playerId;
            this.gameCode = data.gameCode || code;
            this.sendJoinGameMessage();
            this.showWaitingRoom();
            this.closeJoinModal();
        })
        .catch(error => {
            this.showErrorToast(error.message || 'Could not join game');
        });
    }

    openJoinModal() {
        const modal = document.getElementById('join-code-modal');
        const input = document.getElementById('join-code-input');
        if (!modal) return;
        modal.style.display = 'flex';
        if (input) {
            input.value = '';
            input.focus();
        }
    }

    closeJoinModal() {
        const modal = document.getElementById('join-code-modal');
        if (!modal) return;
        modal.style.display = 'none';
    }

    joinByCodeFromModal() {
        const input = document.getElementById('join-code-input');
        const code = this.normalizeGameCode(input && input.value ? input.value : '');
        if (input) input.value = code;
        this.joinByCode(code);
    }

    playCard(cardIndex) {
        if (!this.gameState || !this.gameState.players) return;
        if (this.gameState.phase !== 2) return; // only in IN_PROGRESS
        const me = this.gameState.players.find(p => p.id === this.playerId);
        if (!me || !me.hand) return;
        const hand = Array.isArray(me.hand) ? me.hand : (me.hand.cards || []);
        const card = hand[cardIndex];
        const cardValue = card && (typeof card === 'object' ? card.value : card);
        const isWhot = cardValue === 20 || cardValue === '20';
        if (isWhot) {
            this.pendingWhotCardIndex = cardIndex;
            this.showSuitPicker();
        } else {
            this.sendPlayCard(cardIndex);
        }
    }

    showSuitPicker() {
        const modal = document.getElementById('suit-picker-modal');
        if (!modal) return;
        modal.style.display = 'flex';
        modal.querySelectorAll('.suit-buttons button').forEach(btn => {
            btn.onclick = () => {
                const suit = btn.dataset.suit;
                this.playCardWithSuit(this.pendingWhotCardIndex, suit);
                modal.style.display = 'none';
            };
        });
    }

    playCardWithSuit(cardIndex, chosenSuit) {
        this.sendPlayCard(cardIndex, chosenSuit);
        this.pendingWhotCardIndex = null;
    }

    sendPlayCard(cardIndex, chosenSuit) {
        const payload = { cardIndex };
        if (chosenSuit) payload.chosenSuit = chosenSuit;
        this.sendMessage(102, payload);
    }
    
    drawCard() {
        this.sendMessage(103, {}); // DRAW_CARD
    }
    
    declareLast() {
        this.sendMessage(104, {}); // DECLARE_LAST_CARD
    }
    
    updateGameState(message) {
        const payload = typeof message.payload === 'string' ? JSON.parse(message.payload) : message.payload;
        const jsonStr = payload.gameStateJson || payload.gameState;
        this.gameState = typeof jsonStr === 'string' ? JSON.parse(jsonStr) : jsonStr;
        if (this.gameState && this.gameState.gameCode) {
            this.gameCode = this.gameState.gameCode;
        }
        if (this.pendingAutoStartBots && !this.autoStartSent && this.gameState && this.gameState.phase === 0) {
            const players = Array.isArray(this.gameState.players) ? this.gameState.players : [];
            const attached = players.some((p) => p && p.id === this.playerId);
            const isCreator = this.gameState.creatorPlayerId === this.playerId ||
                (players[0] && players[0].id === this.playerId);
            // Only auto-start after our WS session is actually attached to the room.
            if (attached && isCreator) {
                this.startGame();
                this.autoStartSent = true;
            }
        }
        if (this.pendingBotGameView && this.gameState && this.gameState.phase === 2) {
            this.showGameScreen();
            this.pendingBotGameView = false;
            this.pendingAutoStartBots = false;
        }
        this.renderGame();
    }
    
    renderGame() {
        const phase = this.gameState && this.gameState.phase;
        if (phase === 4) {
            this.showGameOver();
            return;
        }
        const gameCode = (this.gameState && this.gameState.gameCode) || this.gameCode;
        const headerCodeEl = document.getElementById('player-game-code');
        if (headerCodeEl) {
            if (this.gameId && gameCode) {
                headerCodeEl.style.display = 'inline-flex';
                headerCodeEl.textContent = `Code: ${gameCode}`;
            } else {
                headerCodeEl.style.display = 'none';
                headerCodeEl.textContent = 'Code: --';
            }
        }

        const isLobbyPhase = phase === 0;
        const inProgress = phase === 2;
        if (inProgress) {
            this.showGameScreen();
        } else if (isLobbyPhase && this.sessionMode !== 'bots' && this.gameId) {
            this.showWaitingRoom();
        }

        if (isLobbyPhase && this.sessionMode !== 'bots') {
            this.renderWaitingRoom();
        }

        const gameTable = document.getElementById('game-table');
        const handArea = document.getElementById('player-hand');
        const controls = document.getElementById('controls');
        if (gameTable) gameTable.style.display = inProgress ? 'block' : 'none';
        if (handArea) handArea.style.display = inProgress ? 'flex' : 'none';
        if (controls) controls.style.display = inProgress ? 'flex' : 'none';

        const drawBtn = document.getElementById('draw-card');
        const declareBtn = document.getElementById('declare-last');
        if (drawBtn) drawBtn.disabled = !inProgress;
        if (declareBtn) declareBtn.disabled = !inProgress;

        this.updateTurnPopup(inProgress);
        this.updatePhaseLabel(phase);

        if (inProgress) {
            this.renderCallCard();
            this.renderPlayerHand();
        } else {
            this.clearTablePlaceholders();
        }
        this.renderOpponents();
        this.updateGameInfo();
        if (phase === 3) {
            this.showRoundOverBanner();
        } else {
            this.hideRoundOverBanner();
        }
    }

    showRoundOverBanner() {
        let el = document.getElementById('round-over-banner');
        if (!el) {
            el = document.createElement('div');
            el.id = 'round-over-banner';
            el.className = 'round-over-banner';
            document.getElementById('game-info')?.appendChild(el);
        }
        el.textContent = 'Round over. Next round starting…';
        el.style.display = 'block';
    }

    hideRoundOverBanner() {
        const el = document.getElementById('round-over-banner');
        if (el) el.style.display = 'none';
    }

    clearTablePlaceholders() {
        const callCardElement = document.getElementById('call-card');
        if (callCardElement) {
            callCardElement.innerHTML = '';
        }
        const handElement = document.getElementById('player-hand');
        if (handElement) {
            handElement.innerHTML = '';
        }
    }

    renderWaitingRoom() {
        const playersEl = document.getElementById('waiting-room-players');
        const isCreator = this.gameState && (
            this.gameState.creatorPlayerId === this.playerId ||
            (this.gameState.players && this.gameState.players[0] && this.gameState.players[0].id === this.playerId)
        );
        const actionsEl = document.getElementById('waiting-room-actions');
        if (actionsEl) actionsEl.style.display = isCreator ? 'block' : 'none';

        const titleEl = document.getElementById('waiting-room-title');
        if (titleEl) {
            const code = (this.gameState && this.gameState.gameCode) || this.gameCode;
            titleEl.textContent = isCreator && code
                ? `Waiting Room (CODE: ${code})`
                : 'Waiting Room';
        }

        const metaEl = document.getElementById('waiting-room-meta');
        const gameIdEl = document.getElementById('waiting-room-game-id');
        const gameCodeEl = document.getElementById('waiting-room-game-code');
        if (isCreator) {
            if (metaEl) metaEl.style.display = 'block';
            if (gameIdEl) gameIdEl.textContent = this.gameId || '--';
            if (gameCodeEl) gameCodeEl.textContent = (this.gameState && this.gameState.gameCode) || this.gameCode || '--';
        } else if (metaEl) {
            metaEl.style.display = 'none';
        }

        const helpEl = document.getElementById('waiting-room-help');
        if (helpEl) {
            if (isCreator) {
                helpEl.innerHTML = 'Waiting for players to join<span class="loading"></span>';
            } else {
                helpEl.innerHTML = 'Waiting for the game creator to start<span class="loading"></span>';
            }
        }

        const players = (this.gameState && Array.isArray(this.gameState.players)) ? this.gameState.players : [];
        const playerCount = players.length;

        const toastEl = document.getElementById('waiting-room-toast');
        if (toastEl) {
            toastEl.style.display = playerCount > 6 ? 'block' : 'none';
        }

        const startBtn = document.getElementById('start-game-btn');
        if (startBtn && isCreator) {
            const canStart = playerCount >= 3 && playerCount <= 6;
            startBtn.disabled = !canStart;
            startBtn.textContent = 'Start Game';
            if (canStart) {
                startBtn.classList.add('start-game-btn--active');
            } else {
                startBtn.classList.remove('start-game-btn--active');
            }
        }

        if (!playersEl) return;
        if (!players.length) {
            playersEl.innerHTML = '<div class="waiting-room-player">Connecting to room...</div>';
            return;
        }
        playersEl.innerHTML = players.map((p, index) => {
            const me = p.id === this.playerId ? ' (You)' : '';
            const host = index === 0 ? ' • Host' : '';
            return `<div class="waiting-room-player">${p.name || p.id || 'Player'}${me}${host}</div>`;
        }).join('');
    }

    updateTurnPopup(inProgress) {
        const popup = document.getElementById('turn-popup');
        if (!popup) return;
        if (!inProgress || !this.gameState || !Array.isArray(this.gameState.players) || !this.gameState.players.length) {
            popup.style.display = 'none';
            popup.textContent = '';
            return;
        }
        const idx = Math.max(0, this.gameState.currentPlayerIndex ?? 0);
        const current = this.gameState.players[idx];
        const name = current ? (current.name || current.id || 'Player') : 'Player';
        const isMe = current && current.id === this.playerId;
        popup.textContent = isMe ? 'Your turn' : `Turn: ${name}`;
        popup.style.display = 'block';
    }

    updatePhaseLabel(phase) {
        const el = document.getElementById('phase-label');
        if (!el) return;
        const labels = { 0: 'Lobby', 1: 'Starting', 2: 'In progress', 3: 'Round over', 4: 'Game over' };
        const label = phase !== undefined && phase !== null ? (labels[phase] || '') : '';
        el.textContent = label;
        el.style.display = label ? 'block' : 'none';
    }
    
    renderCallCard() {
        const callCardElement = document.getElementById('call-card');
        if (!callCardElement) return;
        if (this.gameState && this.gameState.callCard) {
            const c = this.gameState.callCard;
            callCardElement.innerHTML = '';
            callCardElement.appendChild(this.createCardElement(
                typeof c === 'object' ? c : { suit: 'CIRCLE', value: 1 }));
        } else {
            callCardElement.innerHTML = '<div class="card card-placeholder">Call</div>';
        }
    }
    
    renderPlayerHand() {
        const handElement = document.getElementById('player-hand');
        if (!handElement) return;
        handElement.innerHTML = '';
        if (!this.gameState || !this.gameState.players) return;
        const me = this.gameState.players.find(p => p.id === this.playerId);
        if (!me || !me.hand) return;
        const hand = Array.isArray(me.hand) ? me.hand : (me.hand.cards || []);
        hand.forEach((card, index) => {
            const cardObj = typeof card === 'object' ? card : { suit: 'CIRCLE', value: card };
            const cardEl = this.createCardElement(cardObj, true);

            // Click to play (desktop and mobile tap)
            cardEl.addEventListener('click', () => this.playCard(index));

            // HTML5 drag-and-drop for desktop
            cardEl.draggable = true;
            cardEl.addEventListener('dragstart', (e) => {
                e.dataTransfer.setData('text/plain', String(index));
                e.dataTransfer.effectAllowed = 'move';
                cardEl.classList.add('dragging');
            });
            cardEl.addEventListener('dragend', () => {
                cardEl.classList.remove('dragging');
            });

            // Touch drag for mobile: drag to play-area or tap
            let touchStartX = 0;
            let touchStartY = 0;
            cardEl.addEventListener('touchstart', (e) => {
                touchStartX = e.touches[0].clientX;
                touchStartY = e.touches[0].clientY;
            }, { passive: true });
            cardEl.addEventListener('touchend', (e) => {
                const tx = e.changedTouches[0].clientX;
                const ty = e.changedTouches[0].clientY;
                const dx = Math.abs(tx - touchStartX);
                const dy = Math.abs(ty - touchStartY);
                const playAreaEl = document.getElementById('play-area');
                if (playAreaEl && (dx > 20 || dy > 20)) {
                    const rect = playAreaEl.getBoundingClientRect();
                    if (tx >= rect.left && tx <= rect.right &&
                        ty >= rect.top  && ty <= rect.bottom) {
                        this.playCard(index);
                    }
                    return;
                }
                // Tap without movement falls through to the click handler above
            }, { passive: true });

            handElement.appendChild(cardEl);
        });

        // Wire play-area as a drag-and-drop target
        const playAreaEl = document.getElementById('play-area');
        if (playAreaEl) {
            playAreaEl.addEventListener('dragover', (e) => {
                e.preventDefault();
                e.dataTransfer.dropEffect = 'move';
                playAreaEl.classList.add('drag-over');
            });
            playAreaEl.addEventListener('dragleave', () => {
                playAreaEl.classList.remove('drag-over');
            });
            playAreaEl.addEventListener('drop', (e) => {
                e.preventDefault();
                playAreaEl.classList.remove('drag-over');
                const idx = parseInt(e.dataTransfer.getData('text/plain'), 10);
                if (!isNaN(idx)) this.playCard(idx);
            });
        }
    }
    
    createCardElement(card, interactive = false) {
        const CARD_IMAGES_BASE = 'assets/images';
        const div = document.createElement('div');
        div.className = 'card';
        if (interactive) {
            // Check if playable and add class
        }

        const suit = card.suit || 'CIRCLE';
        const value = card.value !== undefined && card.value !== null ? card.value : 1;
        const src = `${CARD_IMAGES_BASE}/${suit}_${value}.svg`;
        const alt = `${suit} ${value}`;

        const img = document.createElement('img');
        img.className = 'card-img';
        img.src = src;
        img.alt = alt;
        img.setAttribute('loading', 'lazy');

        const fallback = document.createElement('span');
        fallback.className = 'card-fallback';
        fallback.textContent = `${suit} ${value}`;
        fallback.setAttribute('aria-hidden', 'true');

        img.onerror = () => {
            img.style.display = 'none';
            fallback.style.display = 'block';
        };

        div.appendChild(img);
        div.appendChild(fallback);

        return div;
    }
    
    renderOpponents() {
        const container = document.getElementById('opponent-hands');
        if (!container || !this.gameState || !this.gameState.players) return;
        const opponents = this.gameState.players.filter(p => p.id !== this.playerId);
        const idx = this.gameState.currentPlayerIndex ?? 0;
        const currentId = this.gameState.players[idx] ? this.gameState.players[idx].id : null;
        container.innerHTML = opponents.map((p, i) => {
            const handCount = (p.hand && typeof p.hand === 'object' && 'count' in p.hand) ? p.hand.count : (Array.isArray(p.hand) ? p.hand.length : 0);
            const name = p.name || p.id || 'Player';
            const initial = name.charAt(0).toUpperCase();
            const isCurrent = p.id === currentId;
            return `<div class="opponent-circle ${isCurrent ? 'opponent-current-turn' : ''}" title="${name} – ${handCount} cards">
                <div class="opponent-avatar">${initial}</div>
                <span class="opponent-name">${name}</span>
                <span class="opponent-cards">${handCount}</span>
            </div>`;
        }).join('');
    }

    updateGameInfo() {
        const currentEl = document.getElementById('current-player');
        const timerEl = document.getElementById('turn-timer');
        const deckEl = document.getElementById('deck-count');
        if (!this.gameState) return;
        if (currentEl && this.gameState.players && this.gameState.players.length) {
            const idx = this.gameState.currentPlayerIndex ?? 0;
            const cur = this.gameState.players[idx];
            currentEl.textContent = cur ? `Current turn: ${cur.name || cur.id || 'Player'}` : '';
        }
        if (deckEl !== null && deckEl !== undefined) {
            deckEl.textContent = this.gameState.deckSize ?? 0;
        }
    }
    
    handlePlayerJoined(message) {
        console.log('Player joined:', message);
    }
    
    handleCardPlayed(message) {
        console.log('Card played:', message);
    }
    
    handleRoundEnded(message) {
        console.log('Round ended:', message);
    }
    
    handleGameEnded(message) {
        console.log('Game ended:', message);
        this.showGameOver(message);
    }
    
    handleError(message) {
        let payload;
        try {
            payload = typeof message.payload === 'string' ? JSON.parse(message.payload) : message.payload;
        } catch (e) {
            payload = { message: 'Something went wrong' };
        }
        const raw = payload.message || payload.errorCode || 'Invalid action';
        const text = this.getFriendlyErrorMessage(payload.errorCode, raw);
        if (this.sessionMode === 'bots' && payload && payload.errorCode === 'START_GAME' &&
            this.gameState && this.gameState.phase === 0) {
            // Allow another auto-start attempt when server rejects while still in lobby.
            this.autoStartSent = false;
        }
        console.error('Error:', text);
        this.showErrorToast(text);
    }

    getFriendlyErrorMessage(errorCode, serverMessage) {
        const map = {
            'START_GAME': {
                'Not enough players to start': 'Need at least 3 players to start.',
                'Only the creator can start the game': 'Only the host can start the game.',
                'Game already started': 'Game has already started.',
                'Game not found or not in session': 'Session lost. Please return to lobby and rejoin.'
            },
            'invalid_action': serverMessage,
            'JOIN_GAME': {
                'Game not found': 'Invalid or expired game code.',
                'Cannot join game': 'Could not join. Game may be full or already started.'
            }
        };
        if (errorCode && map[errorCode] && typeof map[errorCode] === 'object' && map[errorCode][serverMessage])
            return map[errorCode][serverMessage];
        return serverMessage;
    }

    showErrorToast(text) {
        const el = document.getElementById('error-message');
        if (!el) return;
        el.textContent = text;
        el.style.display = 'block';
        clearTimeout(this._errorToastTimer);
        this._errorToastTimer = setTimeout(() => {
            el.style.display = 'none';
        }, 4000);
    }
    
    showGameOver(message) {
        document.getElementById('game-screen').classList.remove('active');
        document.getElementById('game-over').classList.add('active');
        const winnerInfo = document.getElementById('winner-info');
        const finalScores = document.getElementById('final-scores');
        if (winnerInfo && this.gameState && this.gameState.players) {
            const winner = this.gameState.players.find(p => p.id === (this.gameState.winnerId || (this.gameState.players[0] && this.gameState.players[0].id)));
            winnerInfo.textContent = winner ? `Winner: ${winner.name || winner.id}` : 'Game over';
        }
        if (finalScores && this.gameState && this.gameState.players) {
            finalScores.innerHTML = this.gameState.players.map(p => `${p.name || p.id}: ${p.cumulativeScore ?? p.currentScore ?? 0}`).join('<br>');
        }
    }
    
    returnToLobby() {
        document.getElementById('game-over').classList.remove('active');
        this.showScreen('lobby');
        this.gameId = null;
        this.playerId = null;
        this.gameState = null;
        this.gameCode = null;
        this.sessionMode = null;
        this.pendingAutoStartBots = false;
        this.autoStartSent = false;
        this.pendingBotGameView = false;
        const headerCodeEl = document.getElementById('player-game-code');
        if (headerCodeEl) {
            headerCodeEl.style.display = 'none';
            headerCodeEl.textContent = 'Code: --';
        }
    }
    
    getApiBase() {
        if (typeof this._apiBase !== 'undefined') return this._apiBase;
        const runtimeConfig = (typeof window !== 'undefined' && window.WHOT_CONFIG) ? window.WHOT_CONFIG : null;
        const runtimeBase = runtimeConfig && runtimeConfig.apiBase ? String(runtimeConfig.apiBase).trim() : null;
        if (runtimeBase) {
            this._apiBase = runtimeBase.replace(/\/$/, '');
            return this._apiBase;
        }
        const script = document.querySelector('script[data-api-base]');
        const base = script && script.dataset.apiBase ? script.dataset.apiBase.trim() : null;
        this._apiBase = (base && base.length > 0) ? base.replace(/\/$/, '') : (typeof window !== 'undefined' && window.location && window.location.origin ? window.location.origin.replace(/\/$/, '') : '');
        return this._apiBase;
    }

    /** Fetch URL and parse JSON. Rejects with a clear error if response is not OK or not JSON (e.g. HTML 404 page). */
    async apiJson(url, options = {}) {
        let res;
        try {
            res = await fetch(url, options);
        } catch (err) {
            throw new Error(`Could not reach API at ${url}`);
        }
        const ct = (res.headers.get('Content-Type') || '').toLowerCase();
        if (!ct.includes('application/json')) {
            const text = await res.text();
            const preview = (text || '').trim().slice(0, 80);
            throw new Error(`API returned ${res.status} (expected JSON). Response: ${preview}${text.length > 80 ? '...' : ''}`);
        }
        const data = await res.json();
        if (!res.ok) throw new Error(data.error || data.message || `HTTP ${res.status}`);
        return data;
    }

    getWebSocketUrl() {
        if (typeof this._wsUrl !== 'undefined') return this._wsUrl;
        const runtimeConfig = (typeof window !== 'undefined' && window.WHOT_CONFIG) ? window.WHOT_CONFIG : null;
        const runtimeWsUrl = runtimeConfig && runtimeConfig.wsUrl ? String(runtimeConfig.wsUrl).trim() : null;
        if (runtimeWsUrl) {
            this._wsUrl = runtimeWsUrl;
            return this._wsUrl;
        }
        const script = document.querySelector('script[src*="game.js"]') || document.querySelector('script[data-ws-url], script[data-api-base], script[data-ws-port]');
        const wsUrl = script && script.dataset.wsUrl ? script.dataset.wsUrl.trim() : null;
        if (wsUrl && wsUrl.length > 0) {
            this._wsUrl = wsUrl;
            return this._wsUrl;
        }
        const apiBase = script && script.dataset.apiBase ? script.dataset.apiBase.trim() : null;
        if (apiBase && apiBase.length > 0) {
            const base = apiBase.replace(/\/$/, '');
            try {
                const url = new URL(base);
                const scheme = url.protocol === 'https:' ? 'wss:' : 'ws:';
                this._wsUrl = `${scheme}//${url.host}/ws`;
                return this._wsUrl;
            } catch (e) {
                // fall through to port/host logic
            }
        }
        const wsPort = script && script.dataset.wsPort ? script.dataset.wsPort.trim() : null;
        const scheme = (typeof window !== 'undefined' && window.location && window.location.protocol === 'https:') ? 'wss:' : 'ws:';
        const host = (typeof window !== 'undefined' && window.location) ? window.location.hostname : 'localhost';
        if (wsPort && wsPort.length > 0) {
            this._wsUrl = `${scheme}//${host}:${wsPort}`;
        } else {
            const origin = (typeof window !== 'undefined' && window.location && window.location.host) ? window.location.host : 'localhost:8081';
            this._wsUrl = `${scheme}//${origin}/ws`;
        }
        return this._wsUrl;
    }

    showScreen(screenId) {
        ['lobby', 'waiting-room', 'game-screen', 'game-over'].forEach((id) => {
            const el = document.getElementById(id);
            if (!el) return;
            if (id === screenId) el.classList.add('active');
            else el.classList.remove('active');
        });
    }

    showWaitingRoom() {
        this.showScreen('waiting-room');
    }

    showGameScreen() {
        this.showScreen('game-screen');
    }
}

// Initialize client when page loads; connection starts when user enters lobby (onEnterLobby).
const client = new WhotGameClient();
window.addEventListener('load', () => {
    // If welcome is already hidden (e.g. returning user), go straight to lobby.
    const welcome = document.getElementById('welcome');
    if (welcome && welcome.style.display === 'none') {
        document.getElementById('main-header').style.display = 'flex';
        document.getElementById('main-main').style.display = 'block';
        client.onEnterLobby();
        client.connect();
    }
});
