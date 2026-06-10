#!/bin/bash
# Launcher wrapper for Netrek.app — sets CWD to Resources/ so all
# relative paths (pixmaps/, sdl2/fonts/, sounds/) resolve against
# the bundle instead of wherever the user double-clicked from.
BUNDLE_DIR="$(dirname "$0")/.."
cd "$BUNDLE_DIR/Resources" || exit 1
exec "$BUNDLE_DIR/MacOS/netrek-client-cow" "$@"
