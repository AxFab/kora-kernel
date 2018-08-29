#include <kernel/vfs.h>
#include <kernel/device.h>

void imgdk_setup();
void fatfs_setup();
int fatfs_format(inode_t *blk);

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