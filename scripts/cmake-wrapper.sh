#!/usr/bin/env bash
# Wrapper around CMake that removes the "toolchains" File API request from the
# client query so older CMake (< 3.20) does not report "unknown request kind".
# Use via: "cmake.cmakePath": "${workspaceFolder}/scripts/cmake-wrapper.sh"

set -e
REAL_CMAKE="${REAL_CMAKE:-/usr/bin/cmake}"

# Find build directory from -B <dir> so we can patch the File API query there
BUILD_DIR=""
ARGV=()
while [[ $# -gt 0 ]]; do
  case "$1" in
    -B)
      BUILD_DIR="${2:-.}"
      ARGV+=("$1" "$2")
      shift 2
      ;;
    *)
      ARGV+=("$1")
      shift
      ;;
  esac
done

# If no -B, assume cwd might be build dir when query exists there
strip_toolchains_from_query() {
  local q="$1"
  [[ ! -f "$q" ]] && return 0
  # Remove {"kind":"toolchains","version":1} from requests array (Python for portability)
  python3 -c "
import json, sys
p = '$q'
with open(p) as f:
  d = json.load(f)
if 'requests' in d:
  d['requests'] = [r for r in d['requests'] if r.get('kind') != 'toolchains']
  with open(p, 'w') as f:
    json.dump(d, f, separators=(',', ':'))
"
}

# Try build dir from -B, then cwd
for base in "$BUILD_DIR" .; do
  [[ -z "$base" ]] && continue
  Q="${base}/.cmake/api/v1/query/client-vscode/query.json"
  if [[ -f "$Q" ]]; then
    strip_toolchains_from_query "$Q"
    break
  fi
done

exec "$REAL_CMAKE" "${ARGV[@]}"
