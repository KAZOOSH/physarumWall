#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$SCRIPT_DIR/wallProjection"
OF_ROOT="$(cd "$PROJECT_DIR/../../.." && pwd)"
ZED_DIR="$PROJECT_DIR/.zed"

echo "=== physarumWall setup ==="
echo "Project:  $PROJECT_DIR"
echo "OF root:  $OF_ROOT"
echo ""

# ── 1. Verify openFrameworks installation ────────────────────────────────────
MK="$OF_ROOT/libs/openFrameworksCompiled/project/makefileCommon/compile.project.mk"
if [[ ! -f "$MK" ]]; then
    echo "ERROR: openFrameworks not found at $OF_ROOT"
    echo "       The project must live inside <OF_ROOT>/apps/<category>/<project>/"
    exit 1
fi
echo "[OK] openFrameworks found"

# ── 2. Fix config.make OF_ROOT ───────────────────────────────────────────────
CONFIG="$PROJECT_DIR/config.make"
CURRENT_OF=$(grep -E '^OF_ROOT\s*=' "$CONFIG" | sed 's/.*=\s*//' | tr -d ' ')
if [[ "$CURRENT_OF" != "../../.." ]]; then
    sed -i "s|^OF_ROOT\s*=.*|OF_ROOT = ../../..|" "$CONFIG"
    echo "[FIXED] config.make: OF_ROOT set to ../../.."
else
    echo "[OK] config.make OF_ROOT correct"
fi

# ── 3. Switch git remote from HTTPS to SSH ───────────────────────────────────
REMOTE_URL=$(git -C "$SCRIPT_DIR" remote get-url origin 2>/dev/null || true)
if [[ "$REMOTE_URL" == https://github.com/* ]]; then
    SSH_URL=$(echo "$REMOTE_URL" | sed 's|https://github.com/|git@github.com:|')
    git -C "$SCRIPT_DIR" remote set-url origin "$SSH_URL"
    echo "[FIXED] git remote switched to SSH: $SSH_URL"
else
    echo "[OK] git remote: $REMOTE_URL"
fi

# ── 4. Write Zed config files ────────────────────────────────────────────────
mkdir -p "$ZED_DIR"

cat > "$ZED_DIR/settings.json" << 'EOF'
{
  "languages": {
    "C++": {
      "language_servers": ["clangd"],
      "formatter": "language_server"
    },
    "C": {
      "language_servers": ["clangd"],
      "formatter": "language_server"
    }
  },
  "lsp": {
    "clangd": {
      "binary": {
        "path": "/usr/bin/clangd"
      },
      "initialization_options": {
        "clangdFileStatus": true,
        "fallbackFlags": ["-std=c++17"]
      }
    }
  },
  "file_types": {
    "C++": ["cpp", "h", "hpp", "cc", "cxx"]
  },
  "tab_size": 4,
  "hard_tabs": false
}
EOF

cat > "$ZED_DIR/tasks.json" << EOF
[
  {
    "label": "Build Debug",
    "command": "make Debug -j\$(nproc) OF_ROOT=$OF_ROOT 2>&1",
    "cwd": "\$ZED_WORKTREE_ROOT/wallProjection"
  },
  {
    "label": "Build Release",
    "command": "make -j\$(nproc) OF_ROOT=$OF_ROOT 2>&1",
    "cwd": "\$ZED_WORKTREE_ROOT/wallProjection"
  },
  {
    "label": "Build and Run",
    "command": "make -j\$(nproc) OF_ROOT=$OF_ROOT 2>&1 && make RunRelease",
    "cwd": "\$ZED_WORKTREE_ROOT/wallProjection"
  },
  {
    "label": "Clean Debug",
    "command": "make CleanDebug OF_ROOT=$OF_ROOT",
    "cwd": "\$ZED_WORKTREE_ROOT/wallProjection"
  },
  {
    "label": "Clean Release",
    "command": "make CleanRelease OF_ROOT=$OF_ROOT",
    "cwd": "\$ZED_WORKTREE_ROOT/wallProjection"
  },
  {
    "label": "Clean All",
    "command": "make clean OF_ROOT=$OF_ROOT",
    "cwd": "\$ZED_WORKTREE_ROOT/wallProjection"
  }
]
EOF

cat > "$ZED_DIR/launch.json" << EOF
[
  {
    "label": "Debug wallProjection",
    "adapter": "GDB",
    "request": "launch",
    "program": "$PROJECT_DIR/bin/wallProjection_debug",
    "cwd": "$PROJECT_DIR",
    "args": []
  }
]
EOF

echo "[OK] .zed/ config files written (settings, tasks, launch)"

# ── 5. Generate compile_commands.json for clangd ─────────────────────────────
if command -v bear &>/dev/null; then
    echo "Generating compile_commands.json (bear + make)..."
    make -C "$PROJECT_DIR" clean OF_ROOT="$OF_ROOT" -s 2>/dev/null || true
    bear --output "$PROJECT_DIR/compile_commands.json" \
        -- make -C "$PROJECT_DIR" -j"$(nproc)" OF_ROOT="$OF_ROOT" 2>&1 \
        | grep -v "^make\[" || true
    echo "[OK] compile_commands.json generated"
else
    echo "[SKIP] 'bear' not found — compile_commands.json not generated"
    echo "       Install with: sudo pacman -S bear  (or your distro's equivalent)"
    echo "       Then re-run this script for full LSP support in Zed"
fi

echo ""
echo "Setup complete. Open the project in Zed:"
echo "  zed $PROJECT_DIR"
