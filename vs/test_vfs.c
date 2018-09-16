#include <kernel/vfs.h>
#include <kernel/device.h>
#include <errno.h>

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

#define START_TEST(n)  void n() {
#define END_TEST  }
#define ck_ok(n,m)  do { if (!(n)) __assert_fail("Test fails: " #n " - " m, __FILE__, __LINE__); } while(0)
#define CHECK_TCASE(n)  n()
#define CHECK_TSUITE(m)  kprintf(-1, (m))

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void imgdk_setup();

inode_t *test_fs_setup(CSTR dev, kmod_t *fsmod, void(*format)(inode_t *))
{
	// TODO - Clean FS and DEV list!
	imgdk_setup();
	fsmod->setup();
	
	inode_t *disk = vfs_search_device(dev);
	ck_ok(disk != NULL && errno == 0, "Search disk");
	format(disk);
	
	inode_t *root = vfs_mount(dev, fsmod->name);
	ck_ok(root != NULL  && errno == 0, "Mount newly formed disk");
	return root;
}

void test_fs_teardown(inode_t *root)
{
	int res = vfs_umount(root);
    ck_ok(res == 0 && errno == 0, "Unmount file system");
	vfs_close(root);
}


void test_fs_basic(inode_t *root)
{
    inode_t *ino1 = vfs_lookup(root, "EMPTY.TXT");
    ck_ok(ino1 == NULL && errno == ENOENT);

    inode_t *ino2 = vfs_create(root, "EMPTY.TXT", S_IFREG, NULL, 0);
    ck_ok(ino2 != NULL && errno == 0);

    inode_t *ino3 = vfs_lookup(root, "EMPTY.TXT");
    ck_ok(ino3 != NULL && errno == 0);
    ck_ok(ino2 == ino3);
    vfs_close(ino2);
    vfs_close(ino3);

    inode_t *ino4 = vfs_create(root, "FOLDER", S_IFDIR, NULL, 0);
    ck_ok(ino4 != NULL && errno == 0);

    inode_t *ino5 = vfs_create(ino4, "FILE.O", S_IFREG, NULL, 0);
    ck_ok(ino5 != NULL && errno == 0);

    inode_t *ino6 = vfs_search(root, root, "FOLDER/FILE.O", NULL);
    /*
	
	create FOLDER
	create FILE.O
	search FOLDER/FILE.O
	close
	close
	
	unlink FOLDER
	lookup ..
	unlink FILE.O
	lookup ..
	unlink FOLDER
	lookup ..
	*/
}


void test_fs(CSTR dev, kmod_t *fsmod, void(*format)(inode_t *))
{
    inode_t *ino = test_fs_setup(dev, fsmod, format);
    test_fs_basic(ino);
    test_fs_teardown(ino);
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

extern kmod_t kmod_info_fatfs;
int fatfs_format(inode_t *blk);

START_TEST(test_fs_fat16)
{
	test_fs("sdA", &kmod_info_fatfs, fatfs_format);
}
END_TEST

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int main(int argc, char **argv)
{
	CHECK_TSUITE("File System");
	CHECK_TCASE(test_fs_fat16);
    return 0;
}