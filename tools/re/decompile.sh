#!/usr/bin/env bash
# Decompiles re/inner_game.exe with Ghidra headless into re/ghidra-out/ (git-ignored).
# Ghidra is expected at $GHIDRA (default ~/.cache/hwd-re/ghidra_12.1.4_PUBLIC).
set -euo pipefail
REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
GHIDRA="${GHIDRA:-$HOME/.cache/hwd-re/ghidra_12.1.4_PUBLIC}"
podman run --rm --userns=keep-id --security-opt label=disable -e HOME=/tmp -e JAVA_TOOL_OPTIONS=-Duser.home=/tmp \
    -v "$REPO:/src" -v "$GHIDRA:/ghidra:ro" -w /src hwd-re \
    /ghidra/support/analyzeHeadless /src/re/ghidra-project hw \
        -import /src/re/inner_game.exe -overwrite \
        -scriptPath /src/tools/re -postScript ExportDecomp.java /src/re/ghidra-out \
        -max-cpu "$(nproc)"
