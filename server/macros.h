#ifndef __MACROS_H__
#define __MACROS_H__

#define FORCE_INLINE __attribute__((always_inline)) inline

// Macros to contrain values
#define NOLESS(v, n) do { if (v < n) v = n; } while (0)
#define NOMORE(v, n) do { if (v > n) v = n; } while (0)

#define WITHIN(V, L, H) ((V) >= (L) && (V) <= (H))
#define NUMERIC(a) WITHIN(a, '0', '9')
#define DECIMAL(a) (NUMERIC(a) || a == '.')
#define NUMERIC_SIGNED(a) (NUMERIC(a) || (a) == '-' || (a) == '+')
#define DECIMAL_SIGNED(a) (DECIMAL(a) || (a) == '-' || (a) == '+')
#define IS_EOL(a) ((a == '\n') || (a == '\r'))
#define IS_SPACE(a) ((a == ' ') || (a == '\t') || (a == '\v') || (a == '\f') || (a == '\n') || (a == '\r'))
#define COUNT(a) (sizeof(a) / sizeof(*a))
#define ZERO(a) memset(a, 0, sizeof(a))
#define COPY(a, b) memcpy(a, b, min(sizeof(a), sizeof(b)))
#define IS_CMD_PREFIX(a) ((a == 'G') || (a == 'M') || (a == 'T') || (a == 'P'))
#define IS_EXPONENT_PREFIX(a) ((a == 'E') || (a == 'e'))
#define IS_TERMINAL(a) ((a == '\0'))
#define IS_BASE64(a) (\
    WITHIN(a, 'A', 'Z')\
    || WITHIN(a, 'a', 'z')\
    || NUMERIC(a)\
    || (a == '/') \
    || (a == '+') \
)
#define IS_SINGLE_PRINTABLE(a) WITHIN(a, ' ', '~')

#ifndef MIN
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#endif

#define HAS_NUM(p) (\
    NUMERIC(p[0]) \
    || (p[0] == '.' && NUMERIC(p[1])) \
    || (\
        (p[0] == '-' || p[0] == '+') \
        && (\
            NUMERIC(p[1]) \
            || (p[1] == '.' && NUMERIC(p[2]))\
        )\
    )\
)

#define HAS_ARG(p) (\
    HAS_NUM(p)\
    || IS_BASE64(p[0])\
)

#define NOOP \
    do { } while (0)

#define CEILING(x, y) (((x) + (y)-1) / (y))

#define UNEAR_ZERO(x) ((x) < 0.000001)
#define NEAR_ZERO(x) WITHIN(x, -0.000001, 0.000001)
#define NEAR(x,y) NEAR_ZERO((x)-(y))

#define RECIPROCAL(x) (NEAR_ZERO(x) ? 0.0 : 1.0 / (x))
#define FIXFLOAT(f) (f + 0.00001)

#endif // __MACROS_H__
