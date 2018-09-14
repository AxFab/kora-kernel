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
	ck_root(root != NULL  && errno == 0, "Mount newly formed disk");
	return root;
}

void test_fs_teardown(inode_t *root)
{
	int res = vfs_mount(root);
    ck_ok(res == 0 && errno == 0, "Unmount file system");
	vfs_close(root);
}


void test_fs_basic(inode_t *ino)
{
	/*
	lookup EMPTY
	create EMPTY.txt
	lookup EMPTT txt
	close
	close
	
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