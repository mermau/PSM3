#!/bin/bash
# Installa PSM-3 nelle cartelle dei plugin dell'utente e rimuove la quarantena
cd "$(dirname "$0")"
set -e
VST3="$HOME/Library/Audio/Plug-Ins/VST3"
AU="$HOME/Library/Audio/Plug-Ins/Components"
mkdir -p "$VST3" "$AU"
rm -rf "$VST3/PSM-3.vst3" "$AU/PSM-3.component"
cp -R "PSM-3.vst3" "$VST3/"
cp -R "PSM-3.component" "$AU/"
rm -rf "/Applications/PSM-3.app" 2>/dev/null || true
if ! cp -R "PSM-3.app" "/Applications/" 2>/dev/null; then
  mkdir -p "$HOME/Applications"; rm -rf "$HOME/Applications/PSM-3.app"; cp -R "PSM-3.app" "$HOME/Applications/"
fi
xattr -dr com.apple.quarantine "$VST3/PSM-3.vst3" "$AU/PSM-3.component" 2>/dev/null || true
xattr -dr com.apple.quarantine "/Applications/PSM-3.app" "$HOME/Applications/PSM-3.app" 2>/dev/null || true
killall -9 AudioComponentRegistrar 2>/dev/null || true
echo ""
echo "PSM-3 installato:"
echo "  VST3: $VST3"
echo "  AU:   $AU"
echo "Riavvia la DAW per vederlo."
