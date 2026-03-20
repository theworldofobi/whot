# Multi-stage build for Whot game server (Railway / single PORT)
# Build stage: compile C++ server
FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    software-properties-common \
    && add-apt-repository universe \
    && apt-get update \
    && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    libboost-all-dev \
    nlohmann-json3-dev \
    libsqlite3-dev \
    git \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .

RUN mkdir build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF && \
    cmake --build . --target whot_server -j$(nproc) && \
    cp bin/whot_server /whot_server && \
    cp -r ../web /web

# Run stage: Nginx + Whot server (single PORT via proxy)
FROM ubuntu:22.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    nginx \
    libboost-system1.74.0 \
    libsqlite3-0 \
    gettext-base \
    curl \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Whot binary and static web files
COPY --from=builder /whot_server /app/whot_server
COPY --from=builder /web /app/web

# Nginx config template (PORT substituted at startup), runtime config template, and start script
COPY deploy/nginx.railway.conf.template /app/nginx.railway.conf.template
COPY deploy/runtime-config.injected.js.template /app/runtime-config.injected.js.template
COPY deploy/start.sh /app/start.sh

RUN chmod +x /app/start.sh /app/whot_server

# Whot server listens on 8081 (HTTP) and 8082 (WS) internally; Nginx listens on PORT
ENV PORT=8080
EXPOSE 8080
HEALTHCHECK --interval=30s --timeout=5s --start-period=10s --retries=3 \
  CMD curl -fsS "http://127.0.0.1:${PORT}/api/health" >/dev/null || exit 1

CMD ["/app/start.sh"]
