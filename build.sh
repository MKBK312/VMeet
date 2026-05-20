#!/bin/bash
# Build script for IMClient (Qt 5.15.2 MinGW 8.1.0 32-bit)
# Usage: bash build.sh

set -e
cd "$(dirname "$0")"

MINGW_BIN="E:/Qt/Tools/mingw810_32/bin"
MINGW_LIBEXEC="E:/Qt/Tools/mingw810_32/libexec/gcc/i686-w64-mingw32/8.1.0"
QMAKE="E:/Qt/5.15.2/mingw81_32/bin/qmake.exe"
MAKE="E:/Qt/Tools/mingw810_32/bin/mingw32-make"
CLEAN_PATH="$MINGW_BIN:$MINGW_LIBEXEC:/usr/bin:/bin"

echo "[1/3] Cleaning..."
rm -rf release debug
mkdir release

echo "[2/3] Running qmake..."
env -i PATH="$CLEAN_PATH" HOME="$HOME" TEMP="$TEMP" "$QMAKE" IMClient.pro

echo "[3/3] Building..."
# qmake puts literal "g++" in Makefile — patch to use full path
sed -i 's|^CXX .*|CXX           = E:/Qt/Tools/mingw810_32/bin/g++|' Makefile.Release
sed -i 's|^LINKER .*|LINKER      = E:/Qt/Tools/mingw810_32/bin/g++|' Makefile.Release
sed -i 's|^\tg++|\t$(CXX)|' Makefile.Release
env -i PATH="$CLEAN_PATH" HOME="$HOME" TEMP="$TEMP" "$MAKE" -j4 -f Makefile.Release

echo ""
echo "[4/4] Deploying Qt + MinGW runtime..."
"E:/Qt/5.15.2/mingw81_32/bin/windeployqt.exe" release/IMClient.exe --no-translations 2>/dev/null
# Replace SEH libgcc with DWARF2 (windeployqt sometimes copies wrong variant)
rm -f release/libgcc_s_seh-1.dll
cp "$MINGW_BIN/libgcc_s_dw2-1.dll" "$MINGW_BIN/libwinpthread-1.dll" "$MINGW_BIN/libstdc++-6.dll" "$MINGW_BIN/libgomp-1.dll" release/

echo ""
echo "Build complete: release/IMClient.exe"
ls -lh release/IMClient.exe
