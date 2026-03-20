/**
 * Central config for API and WebSocket base URLs.
 * - Local dev: leave unset; game.js uses current origin and /ws.
 * - Production (Railway/GitHub Pages): inject via runtime-config.injected.js
 *   or set window.WHOT_CONFIG before loading game.js.
 * Expected shape: { apiBase?: string, wsUrl?: string }
 * - apiBase: e.g. "https://your-app.railway.app" (no trailing slash)
 * - wsUrl: e.g. "wss://your-app.railway.app/ws"
 */
window.WHOT_CONFIG = window.WHOT_CONFIG || {};
