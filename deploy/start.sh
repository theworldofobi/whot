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

cd /app && /app/whot_server --http-port 8081 --ws-port "${WHOT_WS_PORT}" --static-path /app/web &
WHOT_PID=$!

# Substitute PORT into Nginx config and run Nginx in foreground
envsubst '${PORT}' < /app/nginx.railway.conf.template > /tmp/nginx.conf
exec nginx -c /tmp/nginx.conf -g 'daemon off;'
