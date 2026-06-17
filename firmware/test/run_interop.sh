#!/usr/bin/env bash
# Live interop: the host-built firmware (device_interop) against the real Python
# phone server. Proves the C protocol port talks to the existing Python stack
# with no ESP32 hardware, AND that the firmware consumes its configuration
# (provisioned here with device_config) instead of hardcoded values.
#
# Usage: firmware/test/run_interop.sh
# Assumes the firmware host build exists at firmware/build (see firmware/README).

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$REPO_ROOT"

DEMO="firmware/build/device_interop"
CONFIG_CLI="firmware/build/device_config"
if [[ ! -x "$DEMO" || ! -x "$CONFIG_CLI" ]]; then
    echo "build first: cmake -S firmware -B firmware/build && cmake --build firmware/build" >&2
    exit 1
fi

PORT="${1:-8765}"

# Isolate config in a temp file so we don't touch any real config.
export SUDONIT_CONFIG_PATH
SUDONIT_CONFIG_PATH="$(mktemp -t sudonit_interop_config.XXXXXX)"
trap 'rm -f "$SUDONIT_CONFIG_PATH"' EXIT

echo "[interop] provisioning config via device_config"
"$CONFIG_CLI" set device_name interop-glasses
"$CONFIG_CLI" set server_host 127.0.0.1
"$CONFIG_CLI" set server_port "$PORT"
"$CONFIG_CLI" show

echo "[interop] starting phone server (stub provider) on port $PORT"
SUDONIT_AI_PROVIDER=stub python3 -m phone.server --port "$PORT" >/tmp/sudonit_interop_server.log 2>&1 &
SERVER_PID=$!
trap 'kill "$SERVER_PID" 2>/dev/null || true; rm -f "$SUDONIT_CONFIG_PATH"' EXIT

for _ in $(seq 1 30); do
    grep -q "listening on" /tmp/sudonit_interop_server.log 2>/dev/null && break
    sleep 0.1
done

echo "[interop] running firmware device_interop (no args — reads server from config)"
"$DEMO"
RC=$?

echo "[interop] --- server log ---"
cat /tmp/sudonit_interop_server.log
exit $RC
