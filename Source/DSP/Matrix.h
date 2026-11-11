// PSM-3 — matrice a pin 16x16 come rete resistiva.
// Righe = sorgenti (1..16), colonne = destinazioni (A..P). Ogni pin e' una resistenza
// tra una riga e una colonna; ogni colonna vede 10 kohm verso massa.
#pragma once
#include <array>
#include <cstdint>

namespace psm3
{
enum class Pin : uint8_t { None = 0, White, Red, Green };

using PinGrid = std::array<uint8_t, 256>;       // indice = riga * 16 + colonna

struct MatrixSolution
{
    std::array<float, 256> t {};                // col[c] = somma_r t[r*16+c] * riga[r]
    std::array<bool, 16> colUsed {};
    std::array<bool, 16> rowUsed {};
};

class Matrix
{
public:
    static constexpr double kRin = 10000.0;

    // Bianco 2k7 +/-5 %, verde 68k +/-5 %; lo scostamento dipende solo da (seed, riga, colonna)
    // e seed 0 = valori nominali esatti. Il rosso (pin di precisione, 2 % nell'originale) qui e'
    // esatto a 2k7, cosi' un patch intonato da tastiera resta intonato su ogni istanza.
    static double pinOhms(Pin p, int row, int col, uint32_t seed);

    // Risolve la rete. Da chiamare fuori dal thread audio.
    static void build(const PinGrid& pins, uint32_t seed, const std::array<double, 16>& rowOutOhms, MatrixSolution& out);

    static void apply(const MatrixSolution& s, const std::array<double, 16>& rowVolts, std::array<double, 16>& colVolts);
};

extern const std::array<double, 16> kRowOutOhms;
}
