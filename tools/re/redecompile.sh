#!/usr/bin/env bash
# Re-applies tools/re/names.txt to the existing Ghidra project and re-exports the
# decompiled C (no re-analysis), then runs the annotator.
set -euo pipefail
REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
GHIDRA="${GHIDRA:-$HOME/.cache/hwd-re/ghidra_12.1.4_PUBLIC}"
podman run --rm --userns=keep-id --security-opt label=disable -e HOME=/tmp -e JAVA_TOOL_OPTIONS=-Duser.home=/tmp \
    -v "$REPO:/src" -v "$GHIDRA:/ghidra:ro" -w /src hwd-re \
    /ghidra/support/analyzeHeadless /src/re/ghidra-project hw \
        -process inner_game.exe -noanalysis \
        -scriptPath /src/tools/re -preScript ApplyNames.java \
        -postScript ExportDecomp.java /src/re/ghidra-out -max-cpu "$(nproc)" 2>&1 \
    | grep -E "ApplyNames|decompiled total|ERROR|Exception" || true
python3 "$REPO/tools/re/annotate.py"
