#include "check.h"
#include "DSP/Matrix.h"
using namespace psm3;

static PinGrid empty() { PinGrid g; g.fill(0); return g; }
static void put(PinGrid& g, int row, int col, Pin p) { g[(size_t) (row * 16 + col)] = (uint8_t) p; }

int main()
{
    std::array<double, 16> zeroOut; zeroOut.fill(0.0);
    std::array<double, 16> out1k;   out1k.fill(1000.0);
    std::array<double, 16> out3k;   out3k.fill(3000.0);
    std::array<double, 16> rows{}, cols{};
    MatrixSolution s;

    // seed 0 = valori nominali esatti
    CHECK(Matrix::pinOhms(Pin::White, 3, 4, 0) == 2700.0);
    CHECK(Matrix::pinOhms(Pin::Green, 3, 4, 0) == 68000.0);

    { // un pin, sorgente ideale
        auto g = empty(); put(g, 0, 1, Pin::White);
        Matrix::build(g, 0, zeroOut, s);
        rows.fill(0.0); rows[0] = 1.0;
        Matrix::apply(s, rows, cols);
        CHECK_REL(cols[1], 10000.0 / 12700.0, 0.001);
        CHECK(cols[0] == 0.0 && cols[2] == 0.0);
        CHECK(s.colUsed[1] && ! s.colUsed[0] && s.rowUsed[0] && ! s.rowUsed[1]);
    }
    { // un pin, sorgente da 1 kohm
        auto g = empty(); put(g, 0, 1, Pin::White);
        Matrix::build(g, 0, out1k, s);
        rows.fill(0.0); rows[0] = 1.0;
        Matrix::apply(s, rows, cols);
        CHECK_REL(cols[1], 10000.0 / 13700.0, 0.001);
    }
    { // due pin sulla stessa colonna: 1 V e 0 V, sorgenti ideali
        auto g = empty(); put(g, 0, 7, Pin::White); put(g, 1, 7, Pin::White);
        Matrix::build(g, 0, zeroOut, s);
        rows.fill(0.0); rows[0] = 1.0;
        Matrix::apply(s, rows, cols);
        const double expected = (1.0 / 2700.0) / (2.0 / 2700.0 + 1.0 / 10000.0);
        CHECK_REL(cols[7], expected, 0.001);
        rows[1] = 1.0;                      // entrambe a 1 V: somma
        Matrix::apply(s, rows, cols);
        CHECK_REL(cols[7], 2.0 * expected, 0.001);
    }
    { // pin verde attenuato
        auto g = empty(); put(g, 2, 3, Pin::Green);
        Matrix::build(g, 0, zeroOut, s);
        rows.fill(0.0); rows[2] = 1.0;
        Matrix::apply(s, rows, cols);
        CHECK_REL(cols[3], 10000.0 / 78000.0, 0.001);
    }
    { // carico di riga: la stessa sorgente da 3 kohm in 4 colonne
        auto g = empty(); for (int c = 0; c < 4; ++c) put(g, 5, c, Pin::White);
        Matrix::build(g, 0, out3k, s);
        rows.fill(0.0); rows[5] = 1.0;
        Matrix::apply(s, rows, cols);
        // nodo di riga: 3k verso la sorgente, 4 rami da 12k7 verso massa
        const double vRow = (1.0 / 3000.0) / (1.0 / 3000.0 + 4.0 / 12700.0);
        const double expected = vRow * 10000.0 / 12700.0;
        for (int c = 0; c < 4; ++c) CHECK_REL(cols[c], expected, 0.001);
        CHECK(expected < 10000.0 / 15700.0);
    }
    { // griglia vuota
        Matrix::build(empty(), 0, out1k, s);
        rows.fill(5.0);
        Matrix::apply(s, rows, cols);
        for (int c = 0; c < 16; ++c) { CHECK(cols[c] == 0.0); CHECK(! s.colUsed[c]); }
    }
    { // tolleranze deterministiche
        for (int i = 0; i < 1000; ++i)
        {
            const int r = i % 16, c = (i / 16) % 16; const uint32_t seed = 1u + (uint32_t) i / 256u;
            const double w = Matrix::pinOhms(Pin::White, r, c, seed), rd = Matrix::pinOhms(Pin::Red, r, c, seed);
            CHECK(w >= 2565.0 && w <= 2835.0);
            CHECK(rd >= 2646.0 && rd <= 2754.0);
            CHECK(w == Matrix::pinOhms(Pin::White, r, c, seed));
        }
        CHECK(Matrix::pinOhms(Pin::White, 1, 1, 5) != Matrix::pinOhms(Pin::White, 1, 2, 5));
    }
    { // griglia piena
        PinGrid g; g.fill((uint8_t) Pin::White);
        Matrix::build(g, 77, kRowOutOhms, s);
        rows.fill(1.0);
        Matrix::apply(s, rows, cols);
        for (int c = 0; c < 16; ++c) CHECK(std::isfinite(cols[c]) && cols[c] > 0.0 && cols[c] < 1.0);
    }
    return checkSummary();
}
