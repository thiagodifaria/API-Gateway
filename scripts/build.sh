#!/usr/bin/env bash
set -euo pipefail

CONFIGURATION="${1:-Release}"
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$ROOT_DIR/build"
SOURCE_DIR="$ROOT_DIR/service-api/service-cpp"

case "$CONFIGURATION" in
  Release|Debug) ;;
  *)
    echo "Uso: ./build.sh [Release|Debug]" >&2
    exit 2
    ;;
esac

if [[ "${OSTYPE:-}" == msys* || "${OSTYPE:-}" == win32* || "${OS:-}" == Windows_NT ]]; then
  local_app_data="${LOCALAPPDATA:-}"
  if command -v cygpath >/dev/null 2>&1 && [[ -n "$local_app_data" ]]; then
    local_app_data="$(cygpath -u "$local_app_data")"
  fi
  for candidate in \
    "/c/Program Files/CMake/bin" \
    "$local_app_data/Microsoft/WinGet/Packages/BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe/mingw64/bin" \
    "/c/Program Files/OpenSSL-Win64/bin"; do
    if [[ -d "$candidate" ]]; then
      export PATH="$candidate:$PATH"
    fi
  done
fi

if ! command -v cmake >/dev/null 2>&1; then
  echo "ERRO: cmake nao encontrado no PATH." >&2
  exit 1
fi

echo "Limpeza: removendo build antigo..."
rm -rf "$BUILD_DIR"

cmake_args=(
  -S "$SOURCE_DIR"
  -B "$BUILD_DIR"
  -DCMAKE_BUILD_TYPE="$CONFIGURATION"
)

OPENSSL_WIN_ROOT="/c/Program Files/OpenSSL-Win64"
if [[ -d "$OPENSSL_WIN_ROOT" ]] && command -v dlltool >/dev/null 2>&1; then
  openssl_import_dir="$BUILD_DIR/openssl-mingw"
  mkdir -p "$openssl_import_dir"
  crypto_def="$OPENSSL_WIN_ROOT/lib/VC/x64/MD/libcrypto.def"
  ssl_def="$OPENSSL_WIN_ROOT/lib/VC/x64/MD/libssl.def"
  crypto_import="$openssl_import_dir/libcrypto.dll.a"
  ssl_import="$openssl_import_dir/libssl.dll.a"

  if [[ -f "$crypto_def" && -f "$ssl_def" ]]; then
    echo "OpenSSL: gerando import libraries MinGW..."
    dlltool -d "$crypto_def" -l "$crypto_import" -D libcrypto-4-x64.dll
    dlltool -d "$ssl_def" -l "$ssl_import" -D libssl-4-x64.dll
    cmake_args+=(
      -DOPENSSL_ROOT_DIR="$OPENSSL_WIN_ROOT"
      -DOPENSSL_INCLUDE_DIR="$OPENSSL_WIN_ROOT/include"
      -DLIB_EAY="$crypto_import"
      -DSSL_EAY="$ssl_import"
    )
  fi
fi

if command -v ninja >/dev/null 2>&1; then
  cmake_args+=(-G Ninja)
fi

echo "Configuracao: CMake $CONFIGURATION..."
cmake "${cmake_args[@]}"

echo "Compilacao: construindo projeto..."
cmake --build "$BUILD_DIR" --config "$CONFIGURATION"

echo "SUCESSO: build concluido em $BUILD_DIR"
