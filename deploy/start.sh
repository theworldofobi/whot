#!/bin/sh
# Start Whot server in background, then Nginx on PORT (Railway sets PORT).
set -e
export PORT="${PORT:-8080}"
mkdir -p /app/logs

cd /app && /app/whot_server --http-port 8081 --ws-port 8080 --static-path /app/web &
WHOT_PID=$!

# Substitute PORT into Nginx config and run Nginx in foreground
envsubst '${PORT}' < /app/nginx.railway.conf.template > /tmp/nginx.conf
exec nginx -c /tmp/nginx.conf -g 'daemon off;'
