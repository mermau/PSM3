#!/usr/bin/env bash
# Uso interno: scripts/commit.sh "<messaggio>" — esegue tutti i test e, se verdi, registra.
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."
scripts/test.sh > /tmp/psm3-test.log 2>&1 || { tail -30 /tmp/psm3-test.log; echo "TEST ROSSI: nessun commit"; exit 1; }
git add -A
git -c user.name="Collateral Effects" -c user.email="noreply@localhost" commit -q -m "$1

Co-Authored-By: Claude Fable 5.2 <noreply@anthropic.com>"
echo "$(grep -c '^OK' /tmp/psm3-test.log) test verdi — $(git log --oneline -1)"
