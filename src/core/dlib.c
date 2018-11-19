/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2018  <Fabien Bavent>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   - - - - - - - - - - - - - - -
 */

struct proc {
    dynlib_t exec;
};

proc_t *dlib_process(CSTR execname)
{
    proc_t *proc = kalloc(sizeof(proc_t));
    proc->execname = strdup(execname);
    proc->root = NULL;
    proc->pwd = NULL;
    proc->acl = NULL;
    proc->session = NULL;
    proc->env = "PATH=/usr/bin:/bin\n"
        "OS=Kora\n"
        "HOME=/home/root\n"
        "HOSTNAME=my-home-pc\n"
        "ARCH=x86\n"
        "LANG=en_US.UTF=8\n"
        "TZ=Europe/Paris\n"
        "PWD=/home/root\n"
        "USER=root\n"
        "UID=9fe14c6\n"
        "LD_LIBRARY_PATH=\n"
        "SHELL=krish\n";
    // TODO
    return proc;
}

inode_t *dlib_lookfor_all(proc_t *proc, inode_t *dir, CSTR libname, CSTR xpath)
{
    char *rl, *rd;
    char *path = strdup(xpath);
    for (char *dirlist = strtok_r(path, ";", &rl), dirlist, dirlist = strtok_r(NULL, ";", &rl)) {
        for (char *dirname = strtok_r(dirlist, ":", &rd), dirname, dirname = strtok_r(NULL, ":", &rd)) {

            inode_t *dir = vfs_search(proc->root, dir, dirname, proc->acl);
            if (dir == NULL)
                continue;

            inode_t *ino = vfs_lookfor(dirr, libname, proc->acl);
            vfs_close(dir);
            if (ino != NULL) {
                free(path);
                return ino;
            }
        }

    }
    free(path);
    return NULL;
}


inode_t *dlib_lookfor(proc_t *proc, inode_t *dir, CSTR libname, CSTR env, CSTR sys)
{
    inode_t *ino;
    if (strchr(libname, '/'))
        return vfs_search(proc->root, dir, libname, proc->acl);

    if (proc->exec.rpath != NULL) {
        ino = dlib_lookfor_all(proc, dir, libname, proc->exec.rpath);
        if (ino != NULL)
            return ino;
    }

    if (!proc->req_set_uid) {
        CSTR lpath = proc_getenv(proc, env);
        if (lpath != NULL) {
            ino = dlib_lookfor_all(proc, dir, libname, lpath)
            if (ino != NULL)
                return ino;
        }
    }

    return dlib_lookfor_all(proc, dir, libname, sys);
}


int dlib_open(dynlib_t *dlib)
{
    // Read Elf file
    dlib->io = bio_create(ino, PAGE_SIZE, 0);
    if (elf_parse(dlib) != 0) {
        bio_destroy(dlib->io);
        dlib->io = NULL;
        assert(errno != 0);
        return -1;
    }

    // Look for dependancies
    dyndep_t* dep;
    for ll_each(dlib->depends, dep, dyndep_t, node) {
        ino = dlib_lookfor(proc, proc->execdir, dep->name, "LD_LIBRARY_PATH", "/usr/lib:/lib");
        if (ino == NULL) {
            // TODO - Missing library!
            errno = ENOENT;
            return -1;
        }

        dynlib_t *lib = hmp_get(&proc->libs_maps, ino, sizeof(void*));
        if (lib != NULL) {
            lib->rcu++;
            deb->lib = lib;
            continue;
        }

        lib = kalloc(sizeof(dynlib_t));
        lib->ino = ino;
        dep->lib = lib;
        ll_enqueue(&proc->queue , &proc->exec->node);
    }
    return 0;
}


int dlib_openexec(proc_t *proc)
{
    dynlib_t *lib
    assert(proc->rpath == NULL);
    // Look for executable file
    inode_t ino = dlib_lookfor(proc, proc->pwd, execname, "PATH", "/usr/bin:/bin");
    if (ino == NULL) {
        assert (errno != 0);
        return -1;
    }
    proc->execdir = vfs_parentof(ino);
    proc->exec.ino = ino;

    // Load exec with libraries
    ll_enqueue(&proc->queue, &proc->exec->node);
    while (proc->queue.count_ > 0) {
        lib = ll_dequeue(&proc->queue, dynlib_t, node);
        ll_append(&proc->library, lib->node);
        if (dlib_open(dlib) != 0) {
            dlib_destroy(&proc->exec);
            assert(errno = 0);
            return -1;
        }

        hmp_put(&proc->libs_map, &dlib->ino, sizeof(void*), dlib);

    }

    // Add libraries to process memory space
    for ll_each(&proc->queue, lib, dynlib_t, node) {
        dlib_rebase(proc->mspace, lib);
        for ll_each(&lib->internal_symbols, symbol, dlibsym_t, node) {
            hmp_put(&proc->symbols, symbol->name, strlen(symbol->name), symbol);
            // TODO - Do not replace first occurence of a symbol.
        }
    }

    // Resolve symbols -- Might be done lazy!
    for ll_each(&proc->queue, lib, dynlib_t, node) {
        for ll_each(&lib->external_symbols, symbols, dlibsym_t, node) {
            // Resolve symbol
            sym = hmp_get(&proc->symbols, symbols->name, strlen(symbol->name));
            // symbol->offset = sym->offset;
            // symbol->size = sym->size;
        }
    }
    return 0l
}



