#include <stdint.h>
#include <stddef.h>

// ---------------------------------------------------------------------------
uint64_t __udivmoddi4(uint64_t num, uint64_t den, uint64_t *rem_p)
{
    uint64_t quot = 0, qbit = 1;

    if ( den == 0 ) {
        return 1 / ((unsigned)den);    /* INT: divide by zero */
    }

    /* Left-justify denominator and count shift */
    while ( (int64_t)den >= 0 ) {
        den <<= 1;
        qbit <<= 1;
    }

    while ( qbit ) {
        if ( den <= num ) {
            num -= den;
            quot += qbit;
        }

        den >>= 1;
        qbit >>= 1;
    }

    if ( rem_p ) {
        *rem_p = num;
    }

    return quot;
}


// ---------------------------------------------------------------------------
int64_t __divdi3(int64_t num, int64_t den)
{
    int minus = 0;
    int64_t v;

    if ( num < 0 ) {
        num = -num;
        minus = 1;
    }

    if ( den < 0 ) {
        den = -den;
        minus ^= 1;
    }

    v = __udivmoddi4(num, den, NULL);

    if ( minus ) {
        v = -v;
    }

    return v;
}


// ---------------------------------------------------------------------------
int64_t __moddi3(int64_t num, int64_t den)
{
    int minus = 0;
    int64_t v;

    if ( num < 0 ) {
        num = -num;
        minus = 1;
    }

    if ( den < 0 ) {
        den = -den;
        minus ^= 1;
    }

    __udivmoddi4(num, den, (uint64_t *)&v);

    if ( minus ) {
        v = -v;
    }

    return v;
}


// ---------------------------------------------------------------------------
uint64_t __udivdi3(uint64_t num, uint64_t den)
{
    return __udivmoddi4(num, den, NULL);
}


// ---------------------------------------------------------------------------
uint64_t __umoddi3(uint64_t num, uint64_t den)
{
    uint64_t v;

    __udivmoddi4(num, den, &v);
    return v;
}

int abs (int value)
{
    return ( value >= 0 ) ? value : -value;
}

long labs (long value)
{
    return ( value >= 0 ) ? value : -value;
}

long long llabs (long long value)
{
    return ( value >= 0 ) ? value : -value;
}

typedef struct {
    int quot, rem;
} div_t;
typedef struct {
    long quot, rem;
} ldiv_t;
typedef struct {
    long long quot, rem;
} lldiv_t;

div_t div (int numer, int denom)
{
    div_t rc;
    rc.quot = numer / denom;
    rc.rem  = numer % denom;
    return rc;
}

ldiv_t ldiv (long numer, long denom)
{
    ldiv_t rc;
    rc.quot = numer / denom;
    rc.rem  = numer % denom;
    return rc;
}

lldiv_t lldiv (long long numer, long long denom)
{
    lldiv_t rc;
    rc.quot = numer / denom;
    rc.rem  = numer % denom;
    return rc;
}




// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
