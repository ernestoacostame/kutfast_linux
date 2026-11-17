#!/usr/bin/env bash
set -euo pipefail

echo "Desinstalando KutFast..."

sudo rm -f /usr/local/bin/kutfast-bin
sudo rm -f /usr/local/bin/kutfast
rm -f "${HOME}/.local/share/applications/kutfast.desktop"
rm -f "${HOME}/.local/share/icons/hicolor/scalable/apps/kutfast.svg"
update-desktop-database "${HOME}/.local/share/applications" 2>/dev/null || true

echo "✓ KutFast desinstalado."
echo ""
echo "Los datos de usuario NO se han eliminado. Si quieres borrarlos:"
echo "  rm -rf ~/.config/kutfast ~/.local/share/kutfast"
