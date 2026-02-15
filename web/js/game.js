class WhotGameClient {
    constructor() {
        this.ws = null;
        this.gameId = null;
        this.playerId = null;
        this.gameState = null;
        if (typeof window !== 'undefined') window.whotClient = this;
        this.initializeEventListeners();
    }

    onEnterLobby() {
        this.loadGameList();
    }
    
    initializeEventListeners() {
        document.getElementById('create-game')?.addEventListener('click', () => this.createGame());
        document.getElementById('join-by-code-btn')?.addEventListener('click', () => this.joinByCode());
        document.getElementById('start-game-btn')?.addEventListener('click', () => this.startGame());
        document.getElementById('draw-card')?.addEventListener('click', () => this.drawCard());
        document.getElementById('declare-last')?.addEventListener('click', () => this.declareLast());
        document.getElementById('return-lobby')?.addEventListener('click', () => this.returnToLobby());
    }

    startGame() {
        this.sendMessage(109, {}); // START_GAME
    }

    getPlayerName() {
        try {
            return sessionStorage.getItem('whot_playerName') || 'Guest';
        } catch (e) {
            return 'Guest';
        }
    }
    
    connect() {
        const wsUrl = this.getWebSocketUrl();
        this.ws = new WebSocket(wsUrl);
        
        this.ws.onopen = () => {
            console.log('Connected to server');
            this.loadGameList();
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
            setTimeout(() => this.connect(), 5000); // Reconnect after 5 seconds
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
    
    createGame() {
        const apiBase = this.getApiBase();
        const botCountEl = document.getElementById('bot-count');
        const botCount = (botCountEl && parseInt(botCountEl.value, 10)) || 0;
        this.apiJson(`${apiBase}/api/games`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({
                playerName: this.getPlayerName(),
                maxPlayers: 4,
                startingCards: 6,
                botCount: Math.max(0, Math.min(3, botCount))
            })
        })
        .then(data => {
            this.gameId = data.gameId;
            this.playerId = data.playerId;
            this.gameCode = data.gameCode || null;
            this.sendJoinGameMessage();
            this.showGameScreen();
        })
        .catch(error => console.error('Error creating game:', error));
    }

    sendJoinGameMessage() {
        if (!this.ws || this.ws.readyState !== WebSocket.OPEN) return;
        this.sendMessage(100, { // JOIN_GAME
            playerName: this.getPlayerName(),
            gameId: this.gameId
        });
    }

    joinByCode() {
        const input = document.getElementById('game-code-input');
        const code = input && input.value ? input.value.trim().toUpperCase() : '';
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
            this.gameId = data.gameId;
            this.playerId = data.playerId;
            this.sendJoinGameMessage();
            this.showGameScreen();
            if (input) input.value = '';
        })
        .catch(error => {
            this.showErrorToast(error.message || 'Could not join game');
        });
    }

    joinGameFromList(gameId) {
        const apiBase = this.getApiBase();
        this.apiJson(`${apiBase}/api/games/${gameId}/join`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ playerName: this.getPlayerName() })
        })
        .then(data => {
            if (data.error) throw new Error(data.error);
            this.gameId = data.gameId;
            this.playerId = data.playerId;
            this.sendJoinGameMessage();
            this.showGameScreen();
        })
        .catch(error => console.error('Error joining game:', error));
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
        this.renderGame();
    }
    
    renderGame() {
        const phase = this.gameState && this.gameState.phase;
        if (phase === 4) {
            this.showGameOver();
            return;
        }
        const gameCode = (this.gameState && this.gameState.gameCode) || this.gameCode;
        const codeEl = document.getElementById('game-code-area');
        if (codeEl) {
            if (phase === 0 && gameCode) {
                codeEl.style.display = 'block';
                codeEl.innerHTML = `
                    <span class="game-code-label">Game code (share to invite):</span>
                    <span class="game-code-value" id="game-code-value">${gameCode}</span>
                    <button type="button" class="btn-copy-code" id="copy-game-code">Copy</button>
                `;
        document.getElementById('copy-game-code')?.addEventListener('click', () => {
            const val = document.getElementById('game-code-value');
            if (val && navigator.clipboard && navigator.clipboard.writeText)
                navigator.clipboard.writeText(val.textContent).then(() => this.showErrorToast('Copied!'));
        });
            } else {
                codeEl.style.display = 'none';
                codeEl.innerHTML = '';
            }
        }
        const startArea = document.getElementById('start-game-area');
        if (startArea) {
            const isCreator = this.gameState && (this.gameState.creatorPlayerId === this.playerId ||
                (this.gameState.players && this.gameState.players[0] && this.gameState.players[0].id === this.playerId));
            startArea.style.display = (phase === 0 && isCreator) ? 'block' : 'none';
        }
        const inProgress = phase === 2;
        const drawBtn = document.getElementById('draw-card');
        const declareBtn = document.getElementById('declare-last');
        if (drawBtn) drawBtn.disabled = !inProgress;
        if (declareBtn) declareBtn.disabled = !inProgress;
        this.renderCallCard();
        this.renderPlayerHand();
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
            cardEl.addEventListener('click', () => this.playCard(index));
            handElement.appendChild(cardEl);
        });
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
        const text = payload.message || payload.errorCode || 'Invalid action';
        console.error('Error:', text);
        this.showErrorToast(text);
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
        document.getElementById('lobby').classList.add('active');
        this.loadGameList();
    }
    
    loadGameList() {
        const apiBase = this.getApiBase();
        this.apiJson(`${apiBase}/api/games`)
            .then(games => {
                const listElement = document.getElementById('game-list');
                if (!listElement) return;
                const phaseLabel = (p) => (p === 0 ? 'Lobby' : p === 2 ? 'In progress' : p === 3 ? 'Round ended' : p === 4 ? 'Ended' : '');
                listElement.innerHTML = (games || []).map(game => {
                    const joinable = game.joinable === true;
                    const status = phaseLabel(game.phase) || `Phase ${game.phase}`;
                    return `
                    <div class="game-item ${joinable ? '' : 'game-item-disabled'}" data-game-id="${game.gameId}" data-joinable="${joinable}">
                        <span>Game ${(game.gameId || '').slice(-8)} – ${game.playerCount}/${game.maxPlayers || 8} players – ${status}</span>
                        ${joinable ? '<button type="button" class="btn-join">Join</button>' : ''}
                    </div>
                    `;
                }).join('');
                listElement.querySelectorAll('.game-item').forEach(el => {
                    if (el.dataset.joinable === 'true') {
                        el.querySelector('.btn-join')?.addEventListener('click', (e) => {
                            e.stopPropagation();
                            this.joinGameFromList(el.dataset.gameId);
                        });
                    }
                });
            })
            .catch(err => console.error('Load games:', err));
    }

    getApiBase() {
        if (typeof this._apiBase !== 'undefined') return this._apiBase;
        const script = document.querySelector('script[data-api-base]');
        const base = script && script.dataset.apiBase ? script.dataset.apiBase.trim() : null;
        this._apiBase = (base && base.length > 0) ? base.replace(/\/$/, '') : (typeof window !== 'undefined' && window.location && window.location.origin ? window.location.origin.replace(/\/$/, '') : '');
        return this._apiBase;
    }

    /** Fetch URL and parse JSON. Rejects with a clear error if response is not OK or not JSON (e.g. HTML 404 page). */
    async apiJson(url, options = {}) {
        const res = await fetch(url, options);
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
        const script = document.querySelector('script[data-ws-port]');
        const wsPort = script && script.dataset.wsPort ? script.dataset.wsPort.trim() : null;
        const scheme = (typeof window !== 'undefined' && window.location && window.location.protocol === 'https:') ? 'wss:' : 'ws:';
        const host = (typeof window !== 'undefined' && window.location) ? window.location.hostname : 'localhost';
        if (wsPort && wsPort.length > 0) {
            this._wsUrl = `${scheme}//${host}:${wsPort}`;
        } else {
            const origin = (typeof window !== 'undefined' && window.location && window.location.host) ? window.location.host : 'localhost:8081';
            this._wsUrl = `${scheme}//${origin}`;
        }
        return this._wsUrl;
    }

    showGameScreen() {
        document.getElementById('lobby').classList.remove('active');
        document.getElementById('game-screen').classList.add('active');
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
    }
});
