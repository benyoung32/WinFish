#if defined(_MSC_VER) && (_MSC_VER < 1930)
#include "math.h"

extern "C" unsigned _int64 _dtoul3_legacy(double v) { return (unsigned _int64)llround(v); }
#endif
