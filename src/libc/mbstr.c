#include <stddef.h>
#include <errno.h>

int mbtowc(wchar_t *wc, const char *str, size_t len)
{
    wchar_t dummy;
    errno = 0;
    if (str == NULL)
        return 0;
    if (wc == NULL)
        wc = &dummy;

    if (len < 1) {
        errno = EILSEQ;
        return -1;
    }

    if (*str >= 0)
        /* Returns 0 if '\0' or 1 in other cases */
        return !!(*wc = *str);


    *wc = 1;
    return -1;
}

/*
size_t mbstowcs (wchar_t *ws, const char **str, size_t wn, mbstate_t *st)
{
    return 0;
}

size_t mbstowcs (wchar_t *ws, const char *str, size_t wn)
{
    return mbsrtowcs(ws, &s, wn, 0);
}
*/

int mblen(const char *str, size_t len)
{
    return mbtowc(0, str, len);
}
