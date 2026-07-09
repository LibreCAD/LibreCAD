#!/bin/bash
set -euxo pipefail

# Build a minimal, static, universal (arm64 + x86_64) Freetype for macOS.
#
# ttf2lff links Freetype, but Homebrew ships a thin (single-arch) Freetype, so a
# universal LibreCAD build cannot link the x86_64 slice of ttf2lff. ttf2lff only
# needs Freetype's core TrueType outline reading, so we build a self-contained
# static library with the optional dependencies disabled (no libpng, harfbuzz,
# brotli, bzip2 or external zlib) - this avoids pulling in further thin dylibs.
#
# Usage: build-universal-freetype-macos.sh <install-prefix>
# Then point the build at it with FREETYPE_DIR=<install-prefix>.

PREFIX="${1:?usage: $0 <install-prefix>}"
FT_VER="${FREETYPE_VERSION:-2.13.3}"
# This code is linked into ttf2lff, which ships inside the released DMG, and the
# download URL redirects to volunteer mirrors - so verify the bytes, not just the
# version. Update both together when bumping FT_VER.
FT_SHA256="${FREETYPE_SHA256:-5c3a8e78f7b24c20b25b54ee575d6daa40007a5f4eea2845861c3409b3021747}"

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
cd "$WORK"
# -f: fail on HTTP errors instead of saving the error page as the tarball.
# --proto/--proto-redir '=https': never follow a redirect down to plain http.
curl -fsSL --proto '=https' --proto-redir '=https' --retry 3 -o freetype.tar.gz \
    "https://download.savannah.gnu.org/releases/freetype/freetype-${FT_VER}.tar.gz"
echo "${FT_SHA256}  freetype.tar.gz" | shasum -a 256 -c -
tar xf freetype.tar.gz
cd "freetype-${FT_VER}"

./configure --prefix="$PREFIX" \
    --enable-static --disable-shared \
    --without-harfbuzz --without-png --without-brotli --without-bzip2 --without-zlib \
    CFLAGS="-arch arm64 -arch x86_64" \
    LDFLAGS="-arch arm64 -arch x86_64"
make -j"$(sysctl -n hw.ncpu)"
make install

echo "Built universal Freetype at $PREFIX"
lipo -info "$PREFIX/lib/libfreetype.a"
