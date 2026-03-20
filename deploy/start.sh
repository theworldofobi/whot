#!/bin/sh
# Start Whot server in background, then Nginx on PORT (Railway sets PORT).
set -e
export PORT="${PORT:-8080}"
export WHOT_WS_PORT="${WHOT_WS_PORT:-8082}"
mkdir -p /app/logs

# Optional: inject API/WS URLs for frontend (Railway: set PUBLIC_URL or API_BASE/WS_URL)
if [ -n "$PUBLIC_URL" ]; then
  [ -z "$API_BASE" ] && export API_BASE="$(echo "$PUBLIC_URL" | sed 's#/$##')"
  [ -z "$WS_URL" ] && export WS_URL="wss://$(echo "$PUBLIC_URL" | sed -e 's#^https\?://##' -e 's#/$##')/ws"
fi
if [ -n "$API_BASE" ] || [ -n "$WS_URL" ]; then
  export API_BASE="${API_BASE:-}"
  export WS_URL="${WS_URL:-}"
  envsubst '${API_BASE} ${WS_URL}' < /app/runtime-config.injected.js.template > /app/web/js/runtime-config.js
fi

#region agent logs
dbg_log() {
  ts="$(date +%s%3N 2>/dev/null || date +%s000)"
  payload="$(printf '{"sessionId":"c414f0","runId":"pre-fix","hypothesisId":"%s","location":"%s","message":"%s","data":%s,"timestamp":%s}' "$1" "$2" "$3" "$4" "$ts")"
  curl -sS -X POST "http://localhost:7242/ingest/b9951c31-438d-4dc8-9d1e-9ac03c210347" \
    -H "Content-Type: application/json" \
    -H "X-Debug-Session-Id: c414f0" \
    --data "$payload" >/dev/null 2>&1 || true
}
#endregion

cd /app && /app/whot_server --http-port 8081 --ws-port "${WHOT_WS_PORT}" --static-path /app/web &
WHOT_PID=$!
#region agent log
dbg_log "H1" "deploy/start.sh:17" "started_whot_server" "{\"pid\":${WHOT_PID},\"httpPort\":8081,\"wsPort\":\"${WHOT_WS_PORT}\",\"portEnv\":\"${PORT}\"}"
#endregion

# Substitute PORT into Nginx config and run Nginx in foreground
envsubst '${PORT}' < /app/nginx.railway.conf.template > /tmp/nginx.conf
#region agent log
LISTEN_LINE="$(sed -n 's/^[[:space:]]*listen[[:space:]]\+\([0-9][0-9]*\).*/\1/p' /tmp/nginx.conf | head -n 1)"
WS_PROXY_LINE="$(sed -n 's#.*proxy_pass[[:space:]]\+http://127\.0\.0\.1:\([0-9][0-9]*\).*#\1#p' /tmp/nginx.conf | head -n 1)"
dbg_log "H2" "deploy/start.sh:23" "rendered_nginx_config" "{\"listenPort\":\"${LISTEN_LINE}\",\"firstProxyPort\":\"${WS_PROXY_LINE}\",\"portEnv\":\"${PORT}\"}"
dbg_log "H3" "deploy/start.sh:24" "about_to_start_nginx" "{\"nginxConfig\":\"/tmp/nginx.conf\",\"daemonMode\":\"off\"}"
#endregion
exec nginx -c /tmp/nginx.conf -g 'daemon off;'
