#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$SCRIPT_DIR/wallProjection"
BIN="$PROJECT_DIR/bin/wallProjection"
APP_NAME="${1:-physarumWall}"
APPDIR="$SCRIPT_DIR/AppDir"
TOOLS_DIR="$SCRIPT_DIR/.appimage-tools"

# ── helpers ──────────────────────────────────────────────────────────────────
require_tool() {
    local name="$1" url="$2"
    local dest="$TOOLS_DIR/$name"
    if command -v "$name" &>/dev/null; then
        echo "[OK] $name found in PATH" >&2
        echo "$name"
        return
    fi
    if [[ -x "$dest" ]]; then
        echo "[OK] $name found in $TOOLS_DIR" >&2
        echo "$dest"
        return
    fi
    echo "Downloading $name..." >&2
    mkdir -p "$TOOLS_DIR"
    curl -fsSL -o "$dest" "$url"
    chmod +x "$dest"
    echo "[OK] $name downloaded" >&2
    echo "$dest"
}

# ── check binary exists ───────────────────────────────────────────────────────
if [[ ! -f "$BIN" ]]; then
    echo "ERROR: binary not found at $BIN"
    echo "       Run 'make -j\$(nproc)' in wallProjection/ first"
    exit 1
fi
echo "[OK] Binary found: $BIN"

# ── download tools if needed ─────────────────────────────────────────────────
LINUXDEPLOY=$(require_tool linuxdeploy \
    "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage")
APPIMAGETOOL=$(require_tool appimagetool \
    "https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage")

# ── clean and create AppDir ───────────────────────────────────────────────────
rm -rf "$APPDIR"
mkdir -p "$APPDIR/usr/bin"

# ── copy binary ───────────────────────────────────────────────────────────────
cp "$BIN" "$APPDIR/usr/bin/$APP_NAME"

# ── copy fmod (shipped with OF, not in system libs) ───────────────────────────
if [[ -f "$PROJECT_DIR/bin/libfmod.so" ]]; then
    mkdir -p "$APPDIR/usr/lib"
    cp "$PROJECT_DIR/bin/libfmod.so"* "$APPDIR/usr/lib/" 2>/dev/null || true
    echo "[OK] libfmod.so copied"
fi

# ── copy data directory ───────────────────────────────────────────────────────
# OF looks for 'data/' next to the binary, so place it next to the binary
cp -r "$PROJECT_DIR/bin/data" "$APPDIR/usr/bin/data"
echo "[OK] data/ directory copied"

# ── desktop entry ─────────────────────────────────────────────────────────────
cat > "$APPDIR/$APP_NAME.desktop" << EOF
[Desktop Entry]
Name=physarumWall
Exec=physarumWall
Icon=physarumWall
Type=Application
Categories=Graphics;
EOF

# ── icon (use a placeholder if none exists) ───────────────────────────────────
ICON_SRC=$(find "$PROJECT_DIR" -name "*.png" | head -1 || true)
if [[ -n "$ICON_SRC" ]]; then
    cp "$ICON_SRC" "$APPDIR/$APP_NAME.png"
else
    # create a minimal 64x64 black PNG via Python
    python3 - << 'PYEOF'
import struct, zlib, os

def make_png(path, size=64):
    def chunk(tag, data):
        c = zlib.crc32(tag + data) & 0xffffffff
        return struct.pack('>I', len(data)) + tag + data + struct.pack('>I', c)
    ihdr = struct.pack('>IIBBBBB', size, size, 8, 2, 0, 0, 0)
    raw  = b''.join(b'\x00' + b'\x20\x20\x60' * size for _ in range(size))
    idat = zlib.compress(raw)
    with open(path, 'wb') as f:
        f.write(b'\x89PNG\r\n\x1a\n')
        f.write(chunk(b'IHDR', ihdr))
        f.write(chunk(b'IDAT', idat))
        f.write(chunk(b'IEND', b''))

make_png(os.environ['ICON_PATH'])
PYEOF
    echo "[OK] Placeholder icon created"
fi
export ICON_PATH="$APPDIR/$APP_NAME.png"

# ── AppRun: set working dir so 'data/' is found next to binary ────────────────
cat > "$APPDIR/AppRun" << 'EOF'
#!/usr/bin/env bash
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export LD_LIBRARY_PATH="$HERE/usr/lib:${LD_LIBRARY_PATH:-}"

# APPIMAGE env var is set by the AppImage runtime to the .AppImage file path.
# Fall back to a sibling path of $HERE for extracted/dev runs.
APPIMAGE_DIR="$(cd "$(dirname "${APPIMAGE:-$HERE/../physarumWall-x86_64.AppImage}")" && pwd)"
DATA_DIR="$APPIMAGE_DIR/data"

# On first run, copy bundled data/ next to the AppImage so it can be edited.
if [[ ! -d "$DATA_DIR" ]]; then
    echo "First run: extracting data/ to $DATA_DIR"
    cp -r "$HERE/usr/bin/data" "$DATA_DIR"
fi

# Tell OF to load data from the editable directory instead of next to the binary.
export OF_DATA_PATH="$DATA_DIR"
exec "$HERE/usr/bin/physarumWall" "$@"
EOF
chmod +x "$APPDIR/AppRun"

# ── bundle shared libraries with linuxdeploy ─────────────────────────────────
echo "Bundling shared libraries..."
ARCH=x86_64 "$LINUXDEPLOY" \
    --appdir "$APPDIR" \
    --executable "$APPDIR/usr/bin/$APP_NAME" \
    --desktop-file "$APPDIR/$APP_NAME.desktop" \
    --icon-file "$APPDIR/$APP_NAME.png" \
    2>&1 | grep -v "^$" || true

# ── force X11-only GLFW (overwrite whatever linuxdeploy bundled) ─────────────
# System has libglfw.so.3.4 (Wayland) and libglfw.so.3.3 (X11-only, custom).
# We need the X11-only build; copy it and fix symlinks explicitly.
GLFW_X11="/usr/lib/libglfw.so.3.3"
if [[ -f "$GLFW_X11" ]]; then
    cp "$GLFW_X11" "$APPDIR/usr/lib/libglfw.so.3.3"
    ln -sf libglfw.so.3.3 "$APPDIR/usr/lib/libglfw.so.3"
    ln -sf libglfw.so.3.3 "$APPDIR/usr/lib/libglfw.so"
    echo "[OK] Forced X11-only GLFW 3.3.10 into AppDir"
else
    echo "[WARN] $GLFW_X11 not found — AppImage may crash on Wayland/X11 mismatch"
fi

# ── build AppImage ────────────────────────────────────────────────────────────
OUTPUT="$SCRIPT_DIR/${APP_NAME}-x86_64.AppImage"
ARCH=x86_64 "$APPIMAGETOOL" "$APPDIR" "$OUTPUT" 2>&1
chmod +x "$OUTPUT"

echo ""
echo "Done: $OUTPUT"
echo "Run:  $OUTPUT"
