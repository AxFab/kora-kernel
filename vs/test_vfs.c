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
	// TODO - Clean FS and DEV list!
	imgdk_setup();
	fatfs_setup();
	
	inode_t *sdA = vfs_search_device("sdA");
	fatfs_format(sdA);
	// ck_ok(sdA != NULL, "#1");
	
	inode_t *root = vfs_mount("sdA", "fatfs");

	CHECK_TSUITE("File System");
	CHECK_TCASE(test_fs_fat16);
    return 0;
}