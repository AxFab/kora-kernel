#include <kernel/vfs.h>
#include <kernel/device.h>

int main()
{
	// TODO - Clean FS and DEV list!
	imgdk_setup();
	fatfs_setup();
	
	inode_t *sdA = vfs_search_device("sdA");
	fatfs_format(sdA);
	// ck_ok(sdA != NULL, "#1");
	
	inode_t *root = vfs_mount("sdA", "fatfs");
	kprintf(-1, "");
    return 0;
}