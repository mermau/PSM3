#!/usr/bin/env bash
# Compila ed esegue i test nativi del motore (senza JUCE).
# Uso: scripts/test.sh            tutti i test
#      scripts/test.sh <nome>     solo Tests/test_<nome>.cpp
set -uo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT="${PSM3_TEST_BUILD:-${TMPDIR:-/tmp}/psm3-tests}"
mkdir -p "$OUT/obj"
cd "$ROOT"
CXX="${CXX:-clang++}"
FLAGS=(-std=c++17 -O2 -Wall -Wextra -ISource -ITests)

# Oggetti del motore, ricompilati solo se cambiati.
OBJS=()
shopt -s nullglob
for src in Source/DSP/*.cpp; do
    obj="$OUT/obj/$(basename "${src%.cpp}").o"
    newest=$(ls -t "$src" Source/DSP/*.h | head -n 1)
    if [[ ! -f "$obj" || "$newest" -nt "$obj" ]]; then
        "$CXX" "${FLAGS[@]}" -c "$src" -o "$obj" || { echo "ERRORE di compilazione: $src"; exit 1; }
    fi
    OBJS+=("$obj")
done

if [[ $# -gt 0 ]]; then TESTS=("Tests/test_$1.cpp"); else TESTS=(Tests/test_*.cpp); fi
fail=0
for t in "${TESTS[@]}"; do
    name="$(basename "${t%.cpp}")"
    if ! "$CXX" "${FLAGS[@]}" "$t" ${OBJS[@]+"${OBJS[@]}"} -o "$OUT/$name"; then
        echo "FALLITO (compilazione): $name"; fail=1; continue
    fi
    if "$OUT/$name"; then echo "OK   $name"; else echo "FALLITO: $name"; fail=1; fi
done
exit $fail
