#ifndef _SRC_DRV_PS2_H
#define _SRC_DRV_PS2_H 1

#include <kernel/core.h>
#include <kernel/input.h>
#include <kernel/mods/fs.h>
#include <kernel/cpu.h>

extern inode_t *mouse_ino;
extern inode_t *kdb_ino;

void PS2_event(inode_t *ino, uint8_t type, int32_t param1, int32_t param2);

void PS2_kdb_handler();

void PS2_mouse_handler();
void PS2_mouse_setup();


#endif  /* _SRC_DRV_PS2_H */
