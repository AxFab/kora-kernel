#include <kernel/core.h>
#include <kernel/task.h>
#include <kernel/sys/elf.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

static void *elf_section(task_t *task, inode_t *ino, struct ELF_phEntry *phe)
{
    if ((phe->virtAddr_ & (PAGE_SIZE - 1)) ||
            (phe->fileAddr_ & (PAGE_SIZE - 1))) {
        errno = EBADF;
        return NULL;
    }

    int vma = (phe->flags_ & 7) | VMA_SHARED | VMA_FILE;
    if (phe->flags_ & 2) {
        vma |= ((phe->flags_ & 7) << 4) | VMA_COPY_ON_WRITE;
        vma &= ~VMA_WRITE;
    }
    // TODO -- Set Zero after `phe->fileSize_'
    size_t alloc_size = ALIGN_UP(phe->memSize_, PAGE_SIZE);
    return mspace_map(task->usmem, phe->virtAddr_, alloc_size, ino,
                      phe->fileAddr_ , phe->fileSize_, vma);
}


int elf_open(task_t *task, inode_t *ino)
{
    if (ino == NULL) {
        errno = ENOENT;
        return -1;
    }

    struct ELF_header *head = (struct ELF_header *)kmap(PAGE_SIZE, ino, 0,
                              VMA_FG_RO_FILE);
    if (head == NULL) {
        errno = EIO;
        return -1;
    }

    if (memcmp(ELFident, head, 16) != 0 || head->machine_ != EM_386 ||
            (head->type_ != ET_EXEC && head->type_ != ET_DYN)) {
        kunmap(head, PAGE_SIZE);
        errno = ENOEXEC;
        return -1;
    }

    int i, sections = 0;
    for (i = 0; i < head->phCount_; ++i) {
        size_t off = head->phOff_ + i * sizeof(struct ELF_phEntry);

        /* TODO - Handle the fact that it doesn't hold in one page. */
        assert (off + sizeof(struct ELF_phEntry) < PAGE_SIZE);
        struct ELF_phEntry *phe = (struct ELF_phEntry *)((size_t)head + off);
        if (phe->type_ != PT_LOAD) {
            continue;
        }
        sections++;
        if (elf_section(task, ino, phe) == NULL) {
            kunmap(head, PAGE_SIZE);
            return -1;
        }
    }

    if (sections == 0) {
        kunmap(head, PAGE_SIZE);
        errno = ENOEXEC;
        return -1;
    }

    if (head->type_ == ET_EXEC) {
        task_start(task, head->entry_, 0);
    }

    kunmap(head, PAGE_SIZE);
    errno = 0;
    return 0;
}

