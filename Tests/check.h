// Mini impalcatura di test: nessuna dipendenza esterna.
#pragma once
#include <cmath>
#include <cstdio>

inline int& checkFailures() { static int n = 0; return n; }
inline int& checkCount()    { static int n = 0; return n; }

#define CHECK(cond) \
    do { ++checkCount(); if (! (cond)) { ++checkFailures(); \
         std::printf("  FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); } } while (0)

#define CHECK_NEAR(a, b, tol) \
    do { ++checkCount(); const double _a = (a), _b = (b), _t = (tol); \
         if (! (std::fabs(_a - _b) <= _t)) { ++checkFailures(); \
         std::printf("  FAIL %s:%d  %s = %g, atteso %g +/- %g\n", __FILE__, __LINE__, #a, _a, _b, _t); } } while (0)

#define CHECK_REL(a, b, rel) \
    do { ++checkCount(); const double _a = (a), _b = (b), _r = (rel); \
         if (! (std::fabs(_a - _b) <= std::fabs(_b) * _r)) { ++checkFailures(); \
         std::printf("  FAIL %s:%d  %s = %g, atteso %g +/- %g%%\n", __FILE__, __LINE__, #a, _a, _b, _r * 100.0); } } while (0)

inline int checkSummary()
{
    if (checkFailures() > 0) { std::printf("  %d di %d controlli falliti\n", checkFailures(), checkCount()); return 1; }
    return 0;
}
