# PSM-3 (PsychoMatrix-3) — Collateral Effects

Strumento VST3 / AU / Standalone per macOS (Apple Silicon e Intel) che emula il sintetizzatore a
matrice **EMS VCS3 "Putney"** (1969) con la tastiera **DK1**. È ricavato dal manuale d'uso, dal
manuale di servizio (specifiche, schemi delle schede A, B, C e procedure di taratura) e dalla
letteratura sul filtro a diodi.

## Installazione

1. Apri la cartella `PSM-3` e fai doppio clic su **Installa PSM-3.command**
   (se macOS lo blocca: clic destro → Apri).
2. Riavvia la DAW e cerca **PSM-3** tra gli strumenti (produttore: Collateral Effects).
   La versione autonoma è **PSM-3.app** in Applicazioni.

## Come si suona

Il VCS3 non emette suono finché non ci sono **pin nella matrice**: ogni pin collega una sorgente
(riga, a sinistra) a una destinazione (colonna, in alto).

- **Clic su un foro**: inserisce un pin bianco; altri clic lo fanno diventare rosso, verde, poi lo tolgono.
  **Clic destro**: toglie il pin. Fermando il mouse su un foro compare che cosa collega.
- **Pin bianco**: 2,7 kΩ con tolleranza del 5 %. **Pin rosso**: 2,7 kΩ esatti, da usare per le
  tensioni di tastiera. **Pin verde**: 68 kΩ, collegamento attenuato (modulazioni leggere).
- **Manopole**: trascina in su/giù; **Maiusc** per la regolazione fine; **doppio clic** per il valore
  predefinito. I tre quadranti FREQUENCY sono a demoltiplica, quindi più lenti.
- **Targhetta "PSM-3"** sotto la matrice: apre il menu (preset, MIDI, qualità, informazioni).
- La finestra si ridimensiona trascinando l'angolo.

All'avvio è caricato il preset **Tastiera base**: suona subito dalla tastiera MIDI.

## Tastiera MIDI (es. Arturia KeyLab)

Le note arrivano alla **DK1**, che come nell'originale è monofonica e manda le sue tensioni agli
Input Ch. 1 e 2 (righe 8 e 9 della matrice):

- Interruttore **INP. CH.1** su *kbd voltage*: la riga 8 porta la tensione di tastiera. Con un pin
  rosso in **I8** e **INPUT LEVEL Channel 1 a 6,7** l'Oscillatore 1 è intonato cromaticamente
  (in **N8**, sul filtro, il valore è 5,8). Come nell'originale, l'intervallo fra i tasti si regola
  con quella manopola: valori diversi danno scale microtonali.
- Su *signal*: la riga 8 porta l'oscillatore interno della tastiera.
- **INP. CH.2** su *dynamic v.*: la riga 9 porta una tensione proporzionale alla forza del tocco.
- Ogni tasto fa partire l'Envelope Shaper come il pulsante ATTACK, ma solo se il tasto precedente è
  stato rilasciato (suonando legato l'inviluppo non riparte). Con la tastiera tieni **OFF a 10**.

**Ogni comando è controllabile via MIDI**: clic destro su una manopola o un interruttore →
*Assegna MIDI*, poi muovi un controllo della KeyLab. Le assegnazioni si salvano con il progetto;
dal menu *MIDI* puoi salvarle come predefinite per le nuove istanze.
Assegnazioni di fabbrica: Mod Wheel → joystick verticale, Pitch Bend → joystick orizzontale,
pedale sustain → pulsante ATTACK. I pin della matrice si salvano con progetto e preset ma non
sono automatizzabili.

## I dispositivi

| Originale | Emulazione |
|---|---|
| **Matrice 16×16** | Rete resistiva vera: ogni pin è una resistenza verso un ingresso da 10 kΩ. Un pin attenua, più pin sulla stessa colonna si mescolano e si caricano a vicenda, più pin sulla stessa riga caricano la sorgente. Qualsiasi retroazione è permessa. |
| **Oscillatore 1** | 1 Hz – 10 kHz, 1,5 ottave per divisione, 0,32 V/ottava. Sinusoide con Shape (pura a 5) e rampa. |
| **Oscillatore 2** | Stessa gamma. Quadra/impulso e triangolo/dente di sega; Shape agisce su entrambe. Forme non simmetriche rispetto a 0 V, come nel circuito. |
| **Oscillatore 3** | 0,025 Hz – 500 Hz, 0,26 V/ottava. Lascia un leggero disturbo negli altri oscillatori, difetto documentato della scheda C. |
| **Noise Generator** | Rumore bianco 3 V p-p con Colour. L'attesa di 20–30 s all'accensione è disattivata (riattivabile dal menu). |
| **Filter/Oscillator** | Scala a diodi secondo il modello di Fontana e Civolani: 12 dB nella prima ottava e 18 dB/ottava dopo, 0,2 V/ottava, oscilla con Response sopra 5 circa (261 Hz con la manopola a 5). Satura con segnali forti: lavora meglio con livelli bassi, come avverte il manuale. |
| **Ring Modulator** | A transistor, ingressi in alternata: con una tensione costante su un ingresso tace ("differenzia"). Distorce oltre 1,5 V p-p; residuo degli ingressi a circa −55 dB. |
| **Envelope Shaper** | Attack 2 ms–1 s, On 0–2,5 s, Decay 3 ms–15 s (controllabile in tensione), Off 10 ms–5 s; oltre 7 non ricicla. Trapezoid da +4 V (riposo) a −3 V (On). |
| **Reverberation** | Due molle da 25 e 30 ms con dispersione, coda di circa 2 s, Mix controllabile in tensione. |
| **Output** | Filtro di tono, modulatore a FET (15 dB/V), Pan. Gli interruttori MUTE in su inseriscono l'emulazione dell'altoparlante interno; in giù (predefinito) si ascolta l'uscita di linea. |
| **Joystick, Meter** | Due tensioni ±2 V con Range; strumento a lancetta sulla colonna A nei due modi. |

Il motore lavora a 4× la frequenza di campionamento (2× dal menu *Qualità*, più leggero).

## Preset

Oltre a *Tastiera base* e *Tastiera con filtro* ci sono gli esperimenti del manuale d'uso EMS
(Coloured Noise, Wind, Bells, Bird Song, Bounces, Pizzicato, Chords, Scales, Bowing, Melodies with
Noise, Timbre Changes, Filtering, Reverberation Chop, Ring Modulation, Beats, Vibrato). Dove il
manuale non dà un valore è stato scelto quello che produce l'effetto descritto. In *Bowing* e
*Bird Song* muovi il joystick. I preset utente sono file `.psm3` in
`~/Library/Application Support/Collateral Effects/PSM-3/Presets`.

## Limiti noti

- Monofonico, come l'originale. Niente sequencer KS né tastiera duofonica DK2.
- L'ingresso audio verso Input Ch. 1/2 è disponibile solo nel VST3, negli host che permettono di
  mandare audio a uno strumento.
- Le curve di alcuni comandi (livelli, tono d'uscita, colore del rumore, Shape della sinusoide)
  sono ricavate dagli estremi indicati nei manuali, non misurate su uno strumento reale.
- Firma ad-hoc, senza notarizzazione Apple: per questo serve lo script di installazione.

## Fonti

Manuale d'uso EMS Synthi VCS3; manuale di servizio EMS Synthi A/VCS3; brochure VCS3 1969;
schemi ridisegnati da Graham Hinton (Hinton Instruments); F. Fontana, M. Civolani, *Modeling of the
EMS VCS3 voltage-controlled filter as a nonlinear filter network*, IEEE TASLP 18(4), 2010.
Carattere: TeX Gyre Adventor (GUST Font License).
