


#include <kernel/core.h>
#include <kernel/vfs.h>
#include <kernel/task.h>
#include "dlib.h"



typedef struct kmnt kmnt_t;
struct kmnt {
    inode_t *root;
    llnode_t node;
    CSTR name;
};

splock_t kmod_lock;
llhead_t kmod_mounted;
llhead_t kmod_standby;
llhead_t kmod_started;
extern proc_t kproc;

void kmod_register(kmod_t *mod)
{
    splock_lock(&kmod_lock);
    ll_enqueue(&kmod_standby, &mod->node);
    splock_unlock(&kmod_lock);
}


void kmod_mount(inode_t *root)
{
    kmnt_t *mnt;
    mnt = kalloc(sizeof(kmnt_t));
    mnt->root = vfs_open(root);
    ll_enqueue(&kmod_mounted, &mnt->node);
}


void kmod_loader()
{
    kmnt_t *mnt;
    kmod_t *mod;
    splock_lock(&kmod_lock);
    for (;;) {
        mnt = ll_dequeue(&kmod_mounted, kmnt_t, node);
        if (mnt == NULL)
            break;
        splock_unlock(&kmod_lock);
        kprintf(KLOG_MSG, "Browsing \033[90m%s\033[0m for driver by \033[90m#%d\033[0m\n", mnt->name, kCPU.running->pid);

            inode_t *ino;
            char filename[100];
            void *ctx = vfs_opendir(mnt->root, NULL);
            while ((ino = vfs_readdir(mnt->root, filename, ctx)) != NULL) {
                dynlib_t *dlib = kalloc(sizeof(dynlib_t));
                dlib->ino = ino;
                dlib->io = bio_create(ino, VMA_FILE_RO, PAGE_SIZE, 0);
                int ret = elf_parse(dlib);
                // dlib_rebase(&kMMU.kspace, dlib);
                // dlib_symbols(&kproc, dlib);
                // find symbols kinfo_, push as mod!
                kprintf(-1, " -Open: %s   %s (%d)\n", filename, sztoa(ino->length), ret);
            }
            vfs_closedir(mnt->root, ctx);

        splock_lock(&kmod_lock);
    }

    for (;;) {
        mod = ll_dequeue(&kmod_standby, kmod_t, node);
        splock_unlock(&kmod_lock);
        if (mod == NULL) {
            async_wait(NULL, NULL, 50000);
            splock_lock(&kmod_lock);
            continue;
        }
        kprintf(KLOG_MSG, "Loading driver \033[90m%s\033[0m by \033[90m#%d\033[0m\n", mod->name, kCPU.running->pid);
        mod->setup();
        splock_lock(&kmod_lock);
        ll_enqueue(&kmod_started, &mod->node);
    }
}
