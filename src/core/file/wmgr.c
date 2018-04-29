#include <kernel/core.h>
#include <kernel/files.h>

surface_t * l;

void wmgr_add_display(surface_t *screen)
{
    l = screen;
}


surface_t *wmgr_window()
{
    return l;
}


