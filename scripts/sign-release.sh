#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
# PandaManager v1.0.0 — Script firma GPG release tarball
# Uso: ./scripts/sign-release.sh <tarball> [gpg-key-id]

set -euo pipefail

VERSION="1.0.0"
PKG="PandaManager-${VERSION}"
TARBALL="${1:-${PKG}.tar.gz}"
KEY="${2:-}"

if [[ ! -f "${TARBALL}" ]]; then
    echo "ERRORE: tarball '${TARBALL}' non trovato." >&2
    exit 1
fi

echo "Calcolo checksum SHA-256..."
sha256sum "${TARBALL}" > "${TARBALL}.sha256"
echo "   -> ${TARBALL}.sha256"

echo "Calcolo checksum BLAKE3 (se disponibile)..."
if command -v b3sum &>/dev/null; then
    b3sum "${TARBALL}" > "${TARBALL}.blake3"
    echo "   -> ${TARBALL}.blake3"
else
    echo "   ATTENZIONE: b3sum non trovato, BLAKE3 saltato."
fi

echo "Firma GPG detached (armored)..."
GPG_OPTS=(--armor --detach-sign)
[[ -n "${KEY}" ]] && GPG_OPTS+=(--local-user "${KEY}")
gpg "${GPG_OPTS[@]}" "${TARBALL}"
echo "   -> ${TARBALL}.asc"

echo "Verifica firma..."
gpg --verify "${TARBALL}.asc" "${TARBALL}"

echo ""
echo "File generati per la release '${VERSION}':"
ls -lh "${TARBALL}" "${TARBALL}.sha256" "${TARBALL}.asc" 2>/dev/null
[[ -f "${TARBALL}.blake3" ]] && ls -lh "${TARBALL}.blake3"
echo ""
echo "Per pubblicare su GitHub Release:"
echo "   gh release create v${VERSION} \\"
echo "       '${TARBALL}' '${TARBALL}.sha256' '${TARBALL}.asc' \\"
echo "       --title 'PandaManager v${VERSION}' \\"
echo "       --notes-file CHANGELOG.md"
