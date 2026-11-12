#!/usr/bin/env python3
"""Genera docs/manuale/manuale.html (poi convertito in PDF con Chrome senza interfaccia).
Legge i preset da Source/Plugin/FactoryPresets.h per disegnare le matrici."""
import html, os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
HERE = os.path.join(ROOT, "docs", "manuale")

INK = "#161616"; PANEL = "#dcdcd8"; BLUE = "#2d6fd2"; GREEN = "#2aa860"; RED = "#d3302b"; YELLOW = "#e7c32f"; WOOD = "#7a4a2a"

ROWS = ["Osc 1 sinusoide", "Osc 1 rampa", "Osc 2 quadra", "Osc 2 triangolo", "Osc 3 quadra", "Osc 3 triangolo", "Noise",
        "Input Ch. 1", "Input Ch. 2", "Filter", "Trapezoid", "Envelope Signal", "Ring Modulator", "Reverberation",
        "Stick orizzontale", "Stick verticale"]
COLS = ["Meter", "Output Ch. 1", "Output Ch. 2", "Envelope", "Ring Mod. A", "Ring Mod. B", "Reverb", "Filter",
        "Frequenza Osc 1", "Frequenza Osc 2", "Frequenza Osc 3", "Decay", "Reverb Mix", "Filter Frequency",
        "Output Ch. 1 Level", "Output Ch. 2 Level"]

PARAM_NAMES = {
    "osc1Freq": "Osc 1 Frequency", "osc1Shape": "Osc 1 Shape", "osc1Sine": "Osc 1 Level sinusoide", "osc1Ramp": "Osc 1 Level rampa",
    "osc2Freq": "Osc 2 Frequency", "osc2Shape": "Osc 2 Shape", "osc2Square": "Osc 2 Level quadra", "osc2Tri": "Osc 2 Level triangolo",
    "osc3Freq": "Osc 3 Frequency", "osc3Shape": "Osc 3 Shape", "osc3Square": "Osc 3 Level quadra", "osc3Tri": "Osc 3 Level triangolo",
    "noiseColour": "Noise Colour", "noiseLevel": "Noise Level", "in1Level": "Input Level Ch. 1", "in2Level": "Input Level Ch. 2",
    "ringLevel": "Ring Modulator Level", "filtFreq": "Filter Frequency", "filtResponse": "Filter Response", "filtLevel": "Filter Level",
    "envAttack": "Attack", "envOn": "On", "envDecay": "Decay", "envOff": "Off", "envTrapezoid": "Trapezoid", "envSignal": "Envelope Signal",
    "revMix": "Reverb Mix", "revLevel": "Reverb Level", "joyX": "Joystick orizzontale", "joyY": "Joystick verticale",
    "rangeX": "Range orizzontale", "rangeY": "Range verticale", "out1Level": "Output Ch. 1 Level", "out2Level": "Output Ch. 2 Level",
    "kbLevel": "DK1 Level", "kbCh1Mode": "DK1 Inp. Ch. 1", "kbCh2Mode": "DK1 Inp. Ch. 2",
}

def parse_presets():
    src = open(os.path.join(ROOT, "Source", "Plugin", "FactoryPresets.h"), encoding="utf-8").read()
    pat = re.compile(r'\{\s*"([^"]+)",\s*(true|false),\s*"([^"]*)",\s*"([^"]*)",\s*"((?:[^"\\]|\\.)*)"\s*\}', re.S)
    out = []
    for name, kb, pins, params, note in pat.findall(src):
        note = note.replace("\\'", "'")
        for a_, b_ in ((" e' ", " è "), ("puo'", "può"), ("'wah-wah'", "«wah-wah»"), ("'trita'", "«trita»"), ("'differenzia'", "«differenzia»")):
            note = note.replace(a_, b_)
        out.append(dict(name=name, keyboard=kb == "true", pins=pins.split(), params=[p.split("=") for p in params.split()], note=note))
    return out

def pin_xy(code):
    col = ord(code[0]) - 65
    m = re.match(r"(\d+)([rg]?)", code[1:])
    return int(m.group(1)) - 1, col, {"": "w", "r": "r", "g": "g"}[m.group(2)]

def matrix_svg(pins, size=150, labels=True):
    """Matrice 16x16 con i pin inseriti. pins: sigle tipo 'B1', 'I8r'."""
    pad_l, pad_t, pad_r, pad_b = (4, 4, 13, 13) if labels else (3, 3, 3, 3)
    cell = (size - pad_l - pad_r) / 16.0
    w = size; h = pad_t + cell * 16 + pad_b
    o = [f'<svg class="mx" viewBox="0 0 {w:.1f} {h:.1f}" width="{w:.1f}" height="{h:.1f}" xmlns="http://www.w3.org/2000/svg">']
    o.append(f'<rect x="{pad_l-3}" y="{pad_t-3}" width="{cell*16+6:.1f}" height="{cell*16+6:.1f}" rx="3" fill="{INK}"/>')
    placed = {}
    for code in pins:
        r, c, k = pin_xy(code); placed[(r, c)] = k
    for r in range(16):
        for c in range(16):
            cx = pad_l + (c + .5) * cell; cy = pad_t + (r + .5) * cell
            if (r, c) in placed:
                fill = {"w": "#f4f2ea", "r": RED, "g": GREEN}[placed[(r, c)]]
                o.append(f'<circle cx="{cx:.1f}" cy="{cy:.1f}" r="{cell*.42:.2f}" fill="{fill}"/>')
            else:
                o.append(f'<circle cx="{cx:.1f}" cy="{cy:.1f}" r="{cell*.13:.2f}" fill="#6f6f6b"/>')
    if labels:
        fs = cell * .78
        for i in range(16):
            o.append(f'<text x="{pad_l + (i+.5)*cell:.1f}" y="{pad_t + 16*cell + 10:.1f}" font-size="{fs:.1f}" text-anchor="middle" class="mxl">{chr(65+i)}</text>')
            o.append(f'<text x="{pad_l + 16*cell + 8:.1f}" y="{pad_t + (i+.5)*cell + fs*.36:.1f}" font-size="{fs:.1f}" text-anchor="middle" class="mxl">{i+1}</text>')
    o.append("</svg>")
    return "".join(o)

def crop(x, y, w, h, width_mm, cls=""):
    """Ritaglio dell'immagine dell'interfaccia (coordinate in pixel sull'immagine 1240x1700)."""
    s = width_mm / w
    return (f'<div class="crop {cls}" style="width:{width_mm:.1f}mm;height:{h*s:.1f}mm">'
            f'<img src="img/gui.png" style="width:{1240*s:.2f}mm;margin-left:{-x*s:.2f}mm;margin-top:{-y*s:.2f}mm" alt=""></div>')

def tag(text, colour=None):
    style = f' style="background:{colour}"' if colour else ""
    return f'<span class="tag"{style}>{html.escape(text)}</span>'

def pins_inline(codes):
    return " ".join(tag(c.rstrip("rg")) + ("<span class='pinc r'></span>" if c.endswith("r") else "<span class='pinc g'></span>" if c.endswith("g") else "") for c in codes)

def chapter(num, title, colour, lead):
    return (f'<section class="chapter"><header class="ch"><span class="cap" style="background:{colour}"></span>'
            f'<span class="chn">{num}</span><h1>{title}</h1></header><p class="lead">{lead}</p>')

def device(title, cropspec, body, controls):
    x, y, w, h, mm = cropspec
    rows = "".join(f"<tr><th>{html.escape(n)}</th><td>{d}</td></tr>" for n, d in controls)
    return (f'<article class="device"><h2 class="bar">{title}</h2><div class="dv">'
            f'<div class="dvimg">{crop(x, y, w, h, mm)}</div><div class="dvtxt">{body}<table class="ctl">{rows}</table></div></div></article>')

def fmt(v):
    f = float(v); s = f"{f:.2f}".rstrip("0").rstrip(".")
    return s.replace(".", ",")

def preset_card(p):
    special = {"kbCh1Mode": lambda v: "kbd voltage" if float(v) >= .5 else "signal", "kbCh2Mode": lambda v: "dynamic v." if float(v) >= .5 else "kbd voltage"}
    rows = []
    for k, v in p["params"]:
        if k in ("kbLevel",) and False: continue
        val = special[k](v) if k in special else fmt(v)
        rows.append(f"<tr><th>{PARAM_NAMES.get(k, k)}</th><td>{val}</td></tr>")
    half = (len(rows) + 1) // 2
    how = "Si suona dalla tastiera." if p["keyboard"] else "Suona da solo: muovi manopole e joystick."
    return (f'<article class="preset"><div class="pmx">{matrix_svg(p["pins"], 142)}</div><div class="ptx"><h3>{html.escape(p["name"])}</h3>'
            f'<p>{html.escape(p["note"])} <em>{how}</em></p><p class="pins">Pin: {pins_inline(p["pins"])}</p>'
            f'<div class="pv"><table>{"".join(rows[:half])}</table><table>{"".join(rows[half:])}</table></div></div></article>')

# ----------------------------------------------------------------------------- contenuti
def build():
    presets = parse_presets()
    P = []
    a = P.append

    # copertina
    a(f'''<section class="cover">
<div class="cv-top"><div class="cv-brand">Collateral Effects</div><div class="cv-kind">Manuale d'uso</div></div>
<div class="cv-title"><h1>PSM-3</h1><p class="cv-sub">PsychoMatrix-3</p>
<p class="cv-desc">Sintetizzatore a matrice per VST3, Audio Unit e Standalone.<br>Emulazione dell'EMS VCS3 con tastiera DK1.</p></div>
<div class="cv-mx">{matrix_svg(["H2","H3","D10","B12","C12","I8r","J8r","N9","G12","C14","E1","F4","B13","L6","M5","O15","P16","A11"], 300, False)}</div>
<div class="cv-foot"><span>Versione 1.0</span><span>macOS, Apple Silicon e Intel</span></div>
</section>''')

    # indice
    toc = [("1", "Uno studio in una valigia di legno", "La storia del VCS3 e come veniva usato"),
           ("2", "Primi passi", "Installazione e primo suono"),
           ("3", "La matrice", "Collegare i dispositivi con i pin"),
           ("4", "I dispositivi", "Che cosa fa ogni sezione del pannello"),
           ("5", "Tastiera e MIDI", "La DK1, la tua tastiera controller, MIDI Learn"),
           ("6", "Preset", "Gli esperimenti del manuale EMS, con le loro matrici"),
           ("7", "Tecniche", "Idee per costruire i tuoi suoni"),
           ("8", "Riferimento", "Menu, scorciatoie, problemi comuni")]
    a('<section class="toc"><h1 class="plain">Contenuti</h1><ol>' +
      "".join(f'<li><span class="tn">{n}</span><span class="tt">{t}</span><span class="td">{d}</span></li>' for n, t, d in toc) +
      '</ol><p class="note">Questo manuale è scritto per chi suona. Descrive che cosa fa ogni comando e come usarlo; '
      'non entra nei dettagli dei circuiti che il plugin riproduce.</p></section>')

    # 1 storia
    a(chapter("1", "Uno studio in una valigia di legno", WOOD,
              "Nel novembre 1969 una piccola ditta di Londra mise in vendita un sintetizzatore che stava su un tavolo, "
              "costava quanto un buon organo elettrico e non aveva la tastiera. Si chiamava VCS3."))
    a('''<aside class="timeline"><dl><dt>1969</dt><dd>Nasce EMS a Londra. A novembre esce il VCS3.</dd>
<dt>1970</dt><dd>Arriva la tastiera DK1, venduta a parte.</dd>
<dt>1971</dt><dd>Il Synthi A mette la stessa elettronica in una valigetta.</dd>
<dt>1972</dt><dd>Il Synthi AKS aggiunge tastiera a sfioramento e sequencer.</dd></dl></aside>
<div class="cols">
<p>La ditta era la <strong>Electronic Music Studios</strong>, fondata da Peter Zinovieff con il compositore Tristram Cary e
l'ingegnere David Cockerell. Lavoravano nella casa di Zinovieff a Putney, sul Tamigi, dove da anni funzionava uno studio
privato di musica elettronica controllato da calcolatori. Il VCS3 nacque per finanziare quello studio: Cockerell ne
disegnò l'elettronica, Cary il mobile in legno a forma di leggio. Il nome sta per <em>Voltage Controlled Studio</em>,
terza versione; negli Stati Uniti lo chiamarono semplicemente «the Putney».</p>
<p>All'epoca un sintetizzatore era un armadio di moduli collegati da decine di cavi, costoso e legato allo studio in cui
era installato. Il VCS3 costava circa 330 sterline, si portava sotto il braccio e sostituiva i cavi con una
<strong>matrice a spinotti</strong>: una piastra di sedici righe per sedici colonne in cui un piccolo pin collega una
sorgente a una destinazione. È l'idea che lo rende ancora oggi riconoscibile, e che dà il nome a questo plugin.</p>
<h2>Uno strumento per trattare il suono</h2>
<p>Il VCS3 non fu pensato per suonare melodie. EMS lo presentava come un piccolo studio completo: tre oscillatori, un
generatore di rumore, un filtro, un modulatore ad anello, un riverbero a molla, un generatore di inviluppo, due ingressi
per microfoni e strumenti, due uscite con altoparlanti incorporati. Il manuale originale elenca tre impieghi:
strumento da concerto per generare suoni e <em>modificare quelli di microfoni e chitarre</em>, cuore di uno studio di
musica elettronica con due registratori a nastro, e strumento didattico per mostrare i fenomeni acustici.</p>
<p>È proprio così che fu usato. Il BBC Radiophonic Workshop ne comprò diversi per gli effetti sonori di radio e
televisione. Brian Eno, nei Roxy Music, ci faceva passare le voci e gli strumenti degli altri. Gli Who lo usarono per
far pulsare l'organo di <em>Won't Get Fooled Again</em>. I Pink Floyd lo portarono in studio per
<em>The Dark Side of the Moon</em>, e lo stesso fecero, ciascuno a suo modo, Tangerine Dream, Hawkwind, King Crimson,
Jean-Michel Jarre e, in Italia, Franco Battiato nei suoi primi dischi.</p>
<p>La tastiera arrivò dopo, come accessorio: la <strong>DK1</strong>, monofonica e sensibile al tocco, che si collega
agli ingressi dello strumento. Anche con la tastiera il VCS3 resta volubile: gli oscillatori si spostano con la
temperatura e l'intervallo fra i tasti si regola con una manopola. Chi lo ama lo ama per questo: non è uno strumento
che esegue, è uno strumento con cui si cerca.</p>
<h2>Che cosa riproduce PSM-3</h2>
<p>PSM-3 riproduce il VCS3 della prima serie insieme alla DK1, seguendo il manuale d'uso e il manuale di servizio
originali. Ogni comando del pannello è presente e si comporta come nello strumento: le gamme delle manopole, i livelli,
il modo in cui i pin si influenzano a vicenda, la deriva lenta degli oscillatori. In più ci sono le comodità di un
plugin: i patch si salvano, ogni manopola si comanda via MIDI e si può automatizzare.</p>
</div>

</section>''')

    # 2 primi passi
    a(chapter("2", "Primi passi", BLUE, "Dall'installazione alla prima nota servono un paio di minuti."))
    a(f'''<h2>Installare</h2>
<ol class="steps"><li>Apri la cartella <strong>PSM-3</strong> che hai scaricato e fai doppio clic su
<strong>Installa PSM-3.command</strong>. Se macOS lo blocca, fai clic destro sul file e scegli <em>Apri</em>.</li>
<li>Riavvia la DAW. PSM-3 compare tra gli strumenti, sotto il produttore <strong>Collateral Effects</strong>, come VST3 e
come Audio Unit.</li>
<li>Per usarlo senza DAW apri <strong>PSM-3.app</strong> dalla cartella Applicazioni e scegli scheda audio e tastiera MIDI
dal menu <em>Options</em>.</li></ol>
<h2>La finestra</h2>
<div class="fig">{crop(0, 0, 1240, 1700, 92)}<div class="figtxt">
<p>La finestra ripete la forma dello strumento.</p>
<p><strong>In alto</strong>, il pannello verticale con i dispositivi: i tre oscillatori a sinistra, i trattamenti a
destra, lo strumento a lancetta in basso.</p>
<p><strong>Al centro</strong>, il pannello orizzontale: la matrice, il joystick, il pulsante Attack e i comandi delle
due uscite. La targhetta <em>PSM-3</em> sotto la matrice apre il menu.</p>
<p><strong>In basso</strong>, la tastiera DK1 con i suoi comandi.</p>
<p>Trascina l'angolo della finestra per ridimensionarla: le proporzioni restano fisse e la misura scelta viene
ricordata.</p></div></div>
<h2>Il primo suono</h2>
<p>All'apertura è caricato il preset <strong>Tastiera base</strong>. Suona un tasto della tua tastiera MIDI, o fai clic
sulla tastiera a schermo: senti l'Oscillatore 1 passare attraverso l'inviluppo.</p>
<p>Ora prova a costruire un suono da zero, come si fa sullo strumento vero.</p>
<ol class="steps"><li>Apri il menu dalla targhetta e scegli <strong>Togli tutti i pin</strong>. Lo strumento tace: senza
pin nulla è collegato a nulla.</li>
<li>Fai clic sul foro all'incrocio tra la colonna {tag("B")} e la riga {tag("1")}. Compare un pin bianco: hai
collegato la sinusoide dell'Oscillatore 1 all'uscita di sinistra.</li>
<li>Alza la manopola {tag("1")} dell'Oscillatore 1 verso 6. Ora senti una nota continua.</li>
<li>Gira il quadrante blu <strong>Frequency</strong> per cambiare altezza e la manopola verde <strong>Shape</strong> per
cambiare timbro.</li>
<li>Metti un altro pin in {tag("C")}{tag("1")} per sentirla anche a destra.</li></ol>
<h2>Usare i comandi</h2>
<table class="keys"><tr><th>Trascinare su e giù</th><td>Gira la manopola.</td></tr>
<tr><th>Maiusc mentre trascini</th><td>Regolazione fine. I tre quadranti blu sono già più lenti delle altre manopole.</td></tr>
<tr><th>Doppio clic</th><td>Riporta il comando al valore predefinito.</td></tr>
<tr><th>Rotella del mouse</th><td>Gira la manopola sotto il puntatore.</td></tr>
<tr><th>Clic destro su un comando</th><td>Apre il menu per assegnarlo a un controllo MIDI.</td></tr>
<tr><th>Clic su un foro</th><td>Inserisce un pin; altri clic ne cambiano il colore e poi lo tolgono.</td></tr>
<tr><th>Clic destro su un foro</th><td>Toglie il pin.</td></tr></table>
</section>''')

    # 3 matrice
    a(chapter("3", "La matrice", GREEN,
              "La matrice è lo strumento. Ogni suono del VCS3 è una disposizione di pin, e imparare a leggerla è la "
              "cosa più utile che puoi fare."))
    rowt = "".join(f"<tr><td>{tag(str(i+1))}</td><td>{r}</td></tr>" for i, r in enumerate(ROWS))
    colt = "".join(f"<tr><td>{tag(chr(65+i))}</td><td>{c}</td></tr>" for i, c in enumerate(COLS))
    a(f'''<div class="mxref"><div>{crop(34, 800, 730, 600, 96)}</div>
<div><p>Le <strong>righe</strong> sono ciò che esce dai dispositivi: i segnali degli oscillatori, il rumore, gli
ingressi, il filtro, l'inviluppo, il modulatore ad anello, il riverbero e le due tensioni del joystick.</p>
<p>Le <strong>colonne</strong> sono ciò che entra. Le prime otto, da A a H, sono ingressi di segnale: lo strumento a
lancetta, le due uscite e i trattamenti. Le altre otto, da I a P, sono ingressi di controllo: una tensione che arriva
lì ha lo stesso effetto di girare la manopola segnata con quella lettera.</p>
<p>Un pin all'incrocio collega la riga alla colonna. Pensa al segnale che corre lungo la riga da sinistra a destra, e
al pin come al punto in cui lo prelevi per mandarlo in alto.</p></div></div>
<div class="twocol"><table class="ref"><caption>Righe: le sorgenti</caption>{rowt}</table>
<table class="ref"><caption>Colonne: le destinazioni</caption>{colt}</table></div>
<h2>Leggere un patch</h2>
<div class="patchread"><div>{matrix_svg(["H7","D10","B12","C12"], 150)}</div><div>
<p>In questo manuale un patch è scritto con le sigle dei fori, lettera della colonna e numero della riga, e disegnato
con una piccola matrice.</p>
<p>{pins_inline(["H7","D10","B12","C12"])} si legge così: il rumore (riga 7) va nel filtro (colonna H); il filtro
(riga 10) va nell'inviluppo (colonna D); l'inviluppo (riga 12) va nelle due uscite (colonne B e C).</p>
<p>Fermando il puntatore su un foro, il plugin ti dice che cosa collega.</p></div></div>
<h2>I tre colori dei pin</h2>
<table class="pinsT">
<tr><td><span class="pinc w big"></span></td><th>Bianco</th><td>Il pin normale. Come nello strumento vero, i pin bianchi non sono tutti identici: ognuno
lascia passare un po' più o un po' meno segnale. Per i suoni non conta; per le intonazioni sì.</td></tr>
<tr><td><span class="pinc r big"></span></td><th>Rosso</th><td>Il pin di precisione. Usalo quando la tensione della tastiera comanda la frequenza di un
oscillatore o del filtro, così l'intonazione è esatta e uguale in ogni progetto.</td></tr>
<tr><td><span class="pinc g big"></span></td><th>Verde</th><td>Un pin attenuato: lascia passare molto meno. Serve per modulazioni leggere, per esempio un
vibrato appena accennato, senza dover abbassare la manopola di livello della sorgente.</td></tr></table>
<h2>Tre cose da sapere</h2>
<p><strong>I pin si influenzano.</strong> Più pin sulla stessa colonna si mescolano, ma ciascuno arriva un po' più
debole. Più pin sulla stessa riga dividono il segnale di quella sorgente. Aggiungere un pin può quindi cambiare
leggermente il livello di ciò che già suonava: è il comportamento dell'originale.</p>
<p><strong>Ogni manopola numerata regola una riga.</strong> Il numero sotto una manopola di livello è il numero della
riga che esce da lì. Se una riga non suona, controlla prima quella manopola.</p>
<p><strong>Tutto si può collegare a tutto.</strong> Un segnale audio può comandare una frequenza, una tensione lenta
può entrare in un ingresso di segnale, l'uscita di un dispositivo può tornare al proprio ingresso. Lo strumento non
si rompe; al massimo tace o urla.</p>
</section>''')

    # 4 dispositivi
    a(chapter("4", "I dispositivi", RED,
              "Il VCS3 ha sorgenti, che producono un segnale, e trattamenti, che ne modificano uno. In questo capitolo "
              "trovi che cosa fa ciascuno e quali righe e colonne della matrice gli appartengono."))
    a(device("Oscillator 1", (34, 20, 600, 192, 78),
             f"<p>La sorgente principale. Copre tutto lo spettro udibile e scende abbastanza da servire anche come "
             f"modulazione lenta. Ha due uscite con livello separato: una sinusoide ({tag('1')}) e una rampa "
             f"({tag('2')}), più ricca di armonici. La colonna {tag('I')} ne comanda la frequenza.</p>",
             [("Frequency", "Da circa 1 Hz a oltre 10 kHz. Ogni divisione vale un'ottava e mezza; a 6 suona il do centrale."),
              ("Shape", "Agisce solo sulla sinusoide: pura a 5, sempre più ronzante verso 0 e verso 10."),
              ("Level", "Il livello delle due uscite alla matrice.")]))
    a(device("Oscillator 2", (34, 212, 600, 192, 78),
             f"<p>Stessa estensione dell'Oscillatore 1, forme d'onda diverse: quadra ({tag('3')}) e triangolo "
             f"({tag('4')}). La colonna {tag('J')} ne comanda la frequenza. Accordato vicino all'Oscillatore 1 dà "
             f"battimenti; i due non restano mai perfettamente insieme, come nello strumento.</p>",
             [("Shape", "Cambia entrambe le uscite: a 5 quadra simmetrica e triangolo; verso gli estremi la quadra diventa un impulso stretto e il triangolo un dente di sega."),
              ("Level", "Livelli di quadra e triangolo.")]))
    a(device("Oscillator 3", (34, 404, 600, 192, 78),
             f"<p>L'oscillatore lento, da un ciclo ogni quaranta secondi a 500 Hz. È soprattutto una sorgente di "
             f"controllo: vibrati, tremoli, aperture del filtro, ritmi. Si sente come nota solo con Frequency oltre 8. "
             f"Uscite: quadra ({tag('5')}) e triangolo ({tag('6')}); la colonna {tag('K')} ne comanda la frequenza.</p>",
             [("Shape", "Come nell'Oscillatore 2. A 0 e a 10 il triangolo diventa una rampa che scende o che sale: utile per scale e glissandi."),
              ("Level", "Decide quanto è profonda la modulazione che mandi altrove.")]))
    a(device("Noise Generator", (34, 596, 301, 192, 50),
             f"<p>Rumore: vento, mare, vapore, piatti e rullanti. Esce sulla riga {tag('7')}. Di solito lo si fa passare "
             f"nel filtro e nell'inviluppo.</p>",
             [("Colour", "Al centro rumore bianco; a sinistra più cupo, a destra più chiaro e sibilante."),
              ("Level", "Livello alla matrice.")]))
    a(device("Filter / Oscillator", (800, 20, 406, 192, 78),
             f"<p>Il filtro è il carattere del VCS3. Riceve il segnale dalla colonna {tag('H')} e lo restituisce sulla "
             f"riga {tag('10')}; la colonna {tag('N')} ne comanda la frequenza. Fa tre cose, secondo la posizione di "
             f"Response.</p><p>Preferisce segnali bassi: se l'ingresso è forte satura e ingrossa il suono. Per un "
             f"filtraggio pulito abbassa il livello della sorgente e alza quello del filtro.</p>",
             [("Frequency", "Il punto in cui il filtro taglia, o la nota che emette quando oscilla."),
              ("Response", "A 0 passa-basso: toglie gli acuti. Verso 5 esalta una banda, come un wah-wah. Oltre 6 circa il filtro oscilla da solo e diventa una sorgente di sinusoide molto pura."),
              ("Level", "Livello alla matrice.")]))
    a(device("Ring Modulator", (634, 20, 166, 192, 36),
             f"<p>Moltiplica due segnali, che entrano dalle colonne {tag('E')} e {tag('F')}; il risultato esce sulla "
             f"riga {tag('13')}. Con due note produce la loro somma e la loro differenza: campane, metalli, voci "
             f"robotiche. Servono entrambi gli ingressi: senza uno dei due non esce nulla.</p>"
             f"<p>Reagisce ai cambiamenti: se a un ingresso arriva una tensione ferma, tace finché quella non si "
             f"muove. È il trucco dei preset Pizzicato e Bowing.</p>",
             [("Level", "Livello alla matrice. Tieni bassi i segnali in ingresso: se sono forti, distorce.")]))
    a(device("Envelope Shaper", (634, 212, 572, 192, 78),
             f"<p>Dà forma al suono nel tempo. Il segnale entra dalla colonna {tag('D')} ed esce, modellato, sulla riga "
             f"{tag('12')}. Il ciclo ha quattro tempi: il suono cresce (Attack), resta (On), svanisce (Decay), tace "
             f"(Off), e poi ricomincia da solo. La spia rossa è accesa durante Attack e On.</p>"
             f"<p>Lo stesso andamento esce anche come tensione sulla riga {tag('11')}, Trapezoid, per comandare altri "
             f"dispositivi: il filtro che si apre a ogni nota, per esempio. La colonna {tag('L')} allunga o accorcia "
             f"il Decay.</p>",
             [("Attack", "Da un colpo secco a una salita di un secondo."),
              ("On", "Quanto resta al massimo, fino a due secondi e mezzo."),
              ("Decay", "Da pochi millesimi a quindici secondi."),
              ("Off", "La pausa prima di ripartire. Oltre 7 il ciclo non riparte da solo: serve un tasto o il pulsante Attack. Con la tastiera tienilo a 10."),
              ("Trapezoid, Signal", "I livelli delle righe 11 e 12.")]))
    a(device("Reverberation", (634, 404, 278, 192, 46),
             f"<p>Un riverbero a molla: entra dalla colonna {tag('G')}, esce sulla riga {tag('14')}. Scuro e un po' "
             f"metallico, come le molle vere. La colonna {tag('M')} comanda Mix, cosa rara per un riverbero: puoi farlo "
             f"aprire e chiudere a ritmo.</p>",
             [("Mix", "A 0 solo il suono diretto, a 10 quasi solo le molle."),
              ("Level", "Livello alla matrice.")]))
    a(device("Input Level", (335, 596, 299, 192, 50),
             f"<p>I due ingressi dello strumento, sulle righe {tag('8')} e {tag('9')}. Qui arrivano le tensioni e "
             f"l'oscillatore della tastiera DK1 e, nella versione VST3, l'audio che la DAW manda al plugin.</p>",
             [("Channel 1, 2", "Il livello di ciò che entra. Con la tastiera, Channel 1 regola anche l'intervallo fra i tasti: vedi il capitolo 5.")]))
    a(device("Output Filter e uscite", (634, 596, 306, 192, 50),
             f"<p>Le colonne {tag('B')} e {tag('C')} sono le due uscite. Ogni patch deve avere almeno un pin in una "
             f"delle due. Le colonne {tag('O')} e {tag('P')} ne comandano il livello: un oscillatore lento lì crea "
             f"un tremolo, il joystick una dissolvenza.</p>",
             [("Output Filter", "Un controllo di tono: piatto a 5, più scuro verso 0, più sottile verso 10."),
              ("Mute", "In giù senti l'uscita di linea. In su senti il suono attraverso i piccoli altoparlanti interni dello strumento: stretto e un po' distorto."),
              ("Level, Pan", "Nel pannello orizzontale: volume e posizione stereo di ciascuna uscita.")]))
    a(device("Joystick, Attack e Meter", (780, 800, 426, 380, 60),
             f"<p>Il <strong>joystick</strong> produce due tensioni, sulle righe {tag('15')} e {tag('16')}. È il modo "
             f"più rapido per esplorare un patch e per suonarlo dal vivo. Resta dove lo lasci; il doppio clic lo "
             f"riporta al centro. Le manopole Range ne limitano l'escursione.</p>"
             f"<p>Il pulsante <strong>Attack</strong> fa partire l'inviluppo e lo tiene aperto finché resta premuto.</p>"
             f"<p>Lo <strong>strumento a lancetta</strong> mostra ciò che arriva alla colonna {tag('A')}. Con "
             f"l'interruttore in su legge le tensioni di controllo, con lo zero al centro; in giù legge il livello "
             f"dei segnali. Serve a seguire un patch senza ascoltarlo.</p>"
             f"<p><strong>Mains</strong> accende e spegne lo strumento.</p>", []))
    a("</section>")

    # 5 tastiera e midi
    a(chapter("5", "Tastiera e MIDI", YELLOW,
              "Le note della tua tastiera arrivano alla DK1, e la DK1 le porta nella matrice come faceva l'originale: "
              "sotto forma di tensioni sugli ingressi."))
    a(f'''<div class="fullcrop">{crop(34, 1420, 1172, 270, 178)}</div>
<h2>Come la tastiera entra nel patch</h2>
<p>La DK1 è monofonica: suona una nota alla volta, l'ultima premuta. Manda due cose allo strumento, una per ingresso,
e due interruttori decidono quali.</p>
<table class="ctl wide">
<tr><th>Inp. Ch. 1 su <em>kbd voltage</em></th><td>La riga {tag("8")} porta una tensione che sale con l'altezza del tasto. Mandala alla
frequenza di un oscillatore, o del filtro, e la tastiera lo suona.</td></tr>
<tr><th>Inp. Ch. 1 su <em>signal</em></th><td>La riga {tag("8")} porta il suono dell'oscillatore interno della tastiera, che puoi
trattare come qualsiasi altra sorgente.</td></tr>
<tr><th>Inp. Ch. 2 su <em>dynamic v.</em></th><td>La riga {tag("9")} porta una tensione che dipende dalla forza con cui batti il tasto.
Mandala al filtro, al livello di un'uscita, al Decay.</td></tr>
<tr><th>Inp. Ch. 2 su <em>kbd voltage</em></th><td>La riga {tag("9")} porta la stessa tensione di altezza, per comandare una seconda cosa.</td></tr>
</table>
<p>Ogni tasto fa anche partire l'inviluppo, come il pulsante Attack, e lo tiene aperto finché resta premuto. Se suoni
legato, senza staccare, l'inviluppo non riparte: è il modo di fare frasi legate.</p>
<h2>Intonare la tastiera</h2>
<div class="patchread"><div>{matrix_svg(["D1","B12","C12","I8r"], 150)}</div><div>
<p>Sul VCS3 l'intervallo fra due tasti vicini non è fisso: lo decidi tu. Per avere la scala cromatica:</p>
<ol class="steps"><li>metti un pin <strong>rosso</strong> in {tag("I")}{tag("8")};</li>
<li>porta <strong>Input Level Channel 1 a 6,7</strong>;</li>
<li>accorda con il quadrante Frequency dell'Oscillatore 1: a 6,33 il do centrale suona come do.</li></ol>
<p>Per il filtro, pin rosso in {tag("N")}{tag("8")} e Input Level a 5,8. Il preset <em>Tastiera base</em> è già
regolato così. Valori diversi di Input Level stringono o allargano la scala: a metà corsa ottieni quarti di tono.</p></div></div>
<p>Gli oscillatori del VCS3 si spostano lentamente di qualche centesimo di tono, e l'Oscillatore 2 non segue mai l'1
alla perfezione. PSM-3 conserva questo comportamento perché fa parte del suono.</p>
<h2>I comandi della DK1</h2>
<table class="ctl wide"><tr><th>Frequency</th><td>L'altezza dell'oscillatore interno della tastiera.</td></tr>
<tr><th>Tuning spread</th><td>L'intervallo fra i tasti per l'oscillatore interno: a 5 è cromatico.</td></tr>
<tr><th>Level</th><td>Il volume dell'oscillatore interno, che dipende anche dalla forza del tocco.</td></tr>
<tr><th>Dynamic range</th><td>Quanto pesa la forza del tocco sulla tensione dinamica.</td></tr></table>
<h2>Comandare tutto dalla tua tastiera controller</h2>
<p>Ogni manopola, ogni interruttore, il pulsante Attack e i due assi del joystick si possono assegnare a un
controllo fisico, per esempio le manopole e i cursori di una Arturia KeyLab.</p>
<ol class="steps"><li>Fai <strong>clic destro</strong> sul comando e scegli <em>Assegna MIDI</em>. Il comando si
cerchia di arancione.</li>
<li>Muovi la manopola, il cursore o il pedale che vuoi usare. L'assegnazione è fatta.</li>
<li>Per toglierla, clic destro e <em>Rimuovi assegnazione</em>.</li></ol>
<p>Un controllo comanda un solo parametro alla volta. Le assegnazioni si salvano con il progetto; dal menu
<em>MIDI</em> puoi salvarle come predefinite, così le ritrovi in ogni nuova istanza.</p>
<table class="keys"><caption>Assegnazioni di fabbrica</caption>
<tr><th>Mod Wheel</th><td>Joystick verticale</td></tr><tr><th>Pitch Bend</th><td>Joystick orizzontale</td></tr>
<tr><th>Pedale sustain</th><td>Pulsante Attack</td></tr></table>
<p>Tutti i comandi sono anche parametri della DAW e si possono automatizzare. I pin della matrice si salvano con il
progetto e con i preset, ma non si automatizzano.</p>
</section>''')

    # 6 preset
    a(chapter("6", "Preset", BLUE,
              "I preset di fabbrica riprendono gli esperimenti del manuale EMS. Ognuno è un punto di partenza: carica, "
              "ascolta, poi sposta un pin."))
    a('<p>Apri il menu dalla targhetta e scegli <em>Preset di fabbrica</em>. <em>Salva preset</em> conserva il tuo patch, '
      'manopole e pin, tra i <em>Preset utente</em>. Dove il manuale originale non indicava un valore è stato scelto '
      'quello che produce l\'effetto descritto.</p>')
    for p in presets:
        a(preset_card(p))
    a("</section>")

    # 7 tecniche
    a(chapter("7", "Tecniche", GREEN, "Qualche idea da cui partire quando la matrice è vuota."))
    def tech(title, pins, text):
        return (f'<article class="tech"><div>{matrix_svg(pins, 118)}</div><div><h3>{title}</h3>{text}'
                f'<p class="pins">Pin: {pins_inline(pins)}</p></div></article>')
    a(tech("Vibrato e modulazione di frequenza", ["B1", "I6"],
           "<p>Manda il triangolo dell'Oscillatore 3 alla frequenza dell'Oscillatore 1. Con l'Oscillatore 3 lento e il "
           "suo livello basso ottieni un vibrato; alzando frequenza e livello il suono si rompe in timbri metallici. "
           "Un pin verde al posto del bianco rende più facile dosare modulazioni leggere.</p>"))
    a(tech("Il filtro che si apre a ogni nota", ["H2", "D10", "B12", "C12", "I8r", "N11"],
           "<p>La tensione Trapezoid segue l'inviluppo: mandala alla frequenza del filtro. È una tensione rovesciata, "
           "alta a riposo e bassa durante la nota, quindi il filtro si chiude quando suoni e si riapre nel Decay. "
           "Regola la manopola Trapezoid per la profondità e Filter Frequency per il punto di partenza.</p>"))
    a(tech("Tremolo e dissolvenze", ["B1", "O6", "P16"],
           "<p>Le colonne O e P comandano il volume delle uscite. L'Oscillatore 3 lì crea un tremolo; il joystick una "
           "dissolvenza manuale. Con una tensione molto negativa l'uscita si chiude del tutto: se un patch tace, "
           "controlla anche questi pin.</p>"))
    a(tech("Campane e suoni metallici", ["E1", "F4", "B13", "C13"],
           "<p>Due oscillatori nel modulatore ad anello, accordati a un intervallo non consonante. Tieni i loro "
           "livelli intorno a 4 o 5. Aggiungi inviluppo e riverbero per la coda, come nel preset Bells.</p>"))
    a(tech("Il filtro come oscillatore", ["B10", "N8r"],
           "<p>Con Response oltre 6 il filtro emette una sinusoide pura, e la colonna N la intona. Con un pin rosso in "
           "N8 e Input Level a 5,8 la suoni dalla tastiera. Alle note più basse impiega un momento a partire.</p>"))
    a(tech("Retroazione", ["H2", "B10", "H14", "G10"],
           "<p>Manda l'uscita di un trattamento nel proprio ingresso passando da un altro: qui il filtro entra nel "
           "riverbero e il riverbero rientra nel filtro. Parti con i livelli bassi e alzali piano. Il risultato va "
           "dall'alone all'ululato; lo strumento regge qualsiasi collegamento.</p>"))
    a(tech("Trattare un suono esterno", ["H8", "B10", "C10", "N6"],
           "<p>Nella versione VST3, se la tua DAW permette di mandare audio a uno strumento, quel segnale arriva sulle "
           "righe 8 e 9. Da lì puoi filtrarlo, modularlo ad anello, mandarlo nelle molle o nell'inviluppo, come si "
           "faceva con chitarre, organi e voci. Senza ingresso audio, metti Inp. Ch. 1 su <em>signal</em> e usa "
           "l'oscillatore della tastiera come sorgente.</p>"))
    a("</section>")

    # 8 riferimento
    a(chapter("8", "Riferimento", WOOD, "Il menu, le opzioni e che cosa controllare quando qualcosa non va."))
    a(f'''<h2>Il menu</h2>
<p>Si apre con un clic sulla targhetta <em>PSM-3</em> sotto la matrice.</p>
<table class="ctl wide">
<tr><th>Preset di fabbrica, Preset utente</th><td>Caricano un patch completo: manopole e pin.</td></tr>
<tr><th>Salva preset</th><td>Salva il patch corrente con un nome. Un nome già usato viene sovrascritto.</td></tr>
<tr><th>Apri la cartella dei preset</th><td>Mostra i file dei tuoi preset, che puoi copiare su un altro computer.</td></tr>
<tr><th>Togli tutti i pin</th><td>Svuota la matrice senza toccare le manopole.</td></tr>
<tr><th>MIDI</th><td>Azzera le assegnazioni, ripristina quelle di fabbrica o salva le attuali come predefinite.</td></tr>
<tr><th>Qualità</th><td><em>Alta</em> è la resa migliore. <em>Leggera</em> usa circa metà del processore: utile con molte istanze.</td></tr>
<tr><th>Riscaldamento originale del rumore</th><td>Come nello strumento vero, il rumore parte solo 20 o 30 secondi dopo l'accensione. Di norma è disattivato.</td></tr>
</table>
<h2>Se non senti nulla</h2>
<p>Sono gli stessi controlli che suggeriva il manuale EMS.</p>
<ol class="steps">
<li>La spia <strong>Mains</strong> è accesa?</li>
<li>C'è almeno un pin nella colonna {tag("B")} o {tag("C")}?</li>
<li>Le manopole di livello della sorgente, dei trattamenti attraversati e delle uscite sono alzate? Per cominciare:
sorgenti tra 5 e 8, trattamenti tra 7 e 10.</li>
<li>Gli oscillatori sono in una zona udibile? Sotto 3 per gli Oscillatori 1 e 2, e sotto 7 per il 3, senti solo dei
clic.</li>
<li>L'inviluppo è fermo? Con <strong>Off</strong> oltre 7 non riparte da solo: premi Attack o un tasto.</li>
<li>C'è un pin nelle colonne {tag("O")} o {tag("P")}? Una tensione negativa lì chiude l'uscita.</li>
</ol>
<h2>Altre domande</h2>
<table class="ctl wide">
<tr><th>La tastiera è stonata</th><td>Controlla che il pin in I8 sia rosso e che Input Level Channel 1 sia a 6,7. Poi accorda con Frequency.</td></tr>
<tr><th>Il suono è distorto</th><td>Abbassa il livello della sorgente che entra nel filtro o nel modulatore ad anello e alza il livello del trattamento.</td></tr>
<tr><th>Le note non ripartono</th><td>Stai suonando legato: stacca tra una nota e l'altra, oppure è voluto.</td></tr>
<tr><th>Si sente una nota continua</th><td>Un oscillatore è collegato direttamente a un'uscita, senza passare dall'inviluppo.</td></tr>
<tr><th>Il plugin non compare in Logic</th><td>Riavvia il computer dopo l'installazione: Logic rilegge gli Audio Unit all'avvio.</td></tr>
</table>
<h2>Dati essenziali</h2>
<table class="keys">
<tr><th>Formati</th><td>VST3, Audio Unit, applicazione autonoma</td></tr>
<tr><th>Sistema</th><td>macOS 11 o successivo, Apple Silicon e Intel</td></tr>
<tr><th>Voci</th><td>Una, come l'originale</td></tr>
<tr><th>Ingresso audio</th><td>Solo nella versione VST3</td></tr>
<tr><th>Preset utente</th><td>Libreria/Application Support/Collateral Effects/PSM-3/Presets, nella tua cartella utente</td></tr>
</table>
<p class="colophon">PSM-3 è un prodotto Collateral Effects. EMS, VCS3 e Synthi sono nomi dei rispettivi titolari, citati
solo per indicare lo strumento a cui il plugin si ispira. Testi composti in TeX Gyre Adventor e Charter.</p>
</section>''')

    css = open(os.path.join(HERE, "manuale.css"), encoding="utf-8").read()
    doc = ('<!DOCTYPE html><html lang="it"><head><meta charset="utf-8"><title>PSM-3, manuale d\'uso</title>'
           f"<style>{css}</style></head><body>{''.join(P)}</body></html>")
    open(os.path.join(HERE, "manuale.html"), "w", encoding="utf-8").write(doc)
    print("manuale.html scritto,", len(doc) // 1024, "kB,", len(presets), "preset")

if __name__ == "__main__":
    build()
