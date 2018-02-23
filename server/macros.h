#ifndef MACROS_H
#define MACROS_H

#define FORCE_INLINE __attribute__((always_inline)) inline

// Macros to contrain values
#define NOLESS(v, n) \
    do               \
    {                \
        if (v < n)   \
            v = n;   \
    } while (0)
#define NOMORE(v, n) \
    do               \
    {                \
        if (v > n)   \
            v = n;   \
    } while (0)

#define WITHIN(V, L, H) ((V) >= (L) && (V) <= (H))
#define NUMERIC(a) WITHIN(a, '0', '9')
#define DECIMAL(a) (NUMERIC(a) || a == '.')
#define NUMERIC_SIGNED(a) (NUMERIC(a) || (a) == '-' || (a) == '+')
#define DECIMAL_SIGNED(a) (DECIMAL(a) || (a) == '-' || (a) == '+')
#define IS_EOL(c) ((c == '\n') || (c == '\r'))
#define IS_SPACE(c) ((c == ' ') || (c == '\t') || (c == '\v') || (c == '\f') || (c == '\n') || (c == '\r'))
#define COUNT(a) (sizeof(a) / sizeof(*a))
#define ZERO(a) memset(a, 0, sizeof(a))
#define COPY(a, b) memcpy(a, b, min(sizeof(a), sizeof(b)))

#define NOOP \
    do { } while (0)

#define CEILING(x, y) (((x) + (y)-1) / (y))

#endif //__MACROS_H
