# PSM-3 (PsychoMatrix-3) — Collateral Effects

Emulazione dell'EMS VCS3 con tastiera DK1: plugin VST3 / AU / Standalone in C++17 con JUCE 8.
La guida per chi lo usa è in [LEGGIMI.md](LEGGIMI.md).

## Compilare su GitHub (non serve nulla sul Mac)

1. Crea un repository su GitHub (es. `PSM-3`) e carica **tutto** il contenuto di questa cartella sul
   ramo `main`, compresa la cartella nascosta `.github/workflows/`. Non caricare `docs/reference/`.
2. Apri **Actions → Build macOS**: parte da solo a ogni push su `main`, oppure con **Run workflow**.
3. A fine build scarica l'artifact **PSM-3-macOS** (contiene `PSM-3.vst3`, `PSM-3.component`,
   `PSM-3.app`, lo script di installazione e il LEGGIMI) e **gui-snapshot** (un'immagine
   dell'interfaccia resa dal runner, utile per controllare il layout).

Il workflow compila il binario universale, esegue i test del motore, il test del processore e
`auval`, firma ad-hoc e impacchetta con `ditto`. Un tag `v*` pubblica anche una release.

## Struttura

- `Source/DSP/` — motore, senza JUCE: un file per dispositivo e `PSM3Engine` che li collega alla matrice.
- `Source/Plugin/` — processore JUCE (parametri, MIDI Learn, stato, preset) e preset di fabbrica.
- `Source/GUI/` — pannelli disegnati in vettoriale.
- `Tests/` — test nativi del motore; `Tests/plugin/` — test che richiedono JUCE (solo in CI).
- `docs/manuale/` — manuale d'uso: `build_manual.py` genera l'HTML (legge i preset dal codice), Chrome lo converte in PDF.
- `docs/superpowers/` — specifica e piano di implementazione.

## Test del motore in locale

Basta il compilatore Apple a riga di comando:

```bash
scripts/test.sh
```

Per ascoltare i preset resi dal motore, senza plugin:

```bash
mkdir -p ~/Desktop/psm3-preset && PSM3_RENDER_DIR=~/Desktop/psm3-preset scripts/test.sh presets
```
