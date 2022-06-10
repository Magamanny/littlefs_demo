#include "lfs.h"
// little fs prototype of eeprom flash wrapper
int eepromfs_erase(const struct lfs_config *c, lfs_block_t block);
int eepromfs_read(const struct lfs_config *c, lfs_block_t block,
				  lfs_off_t off, void *buffer, lfs_size_t size);
int eepromfs_prog(const struct lfs_config *c, lfs_block_t block,
				  lfs_off_t off, const void *buffer, lfs_size_t size);
int eepromfs_sync(const struct lfs_config *c);
// memory modle wrapper
uint8_t eeprom[10*1024]; // 10kb
void readData(uint32_t addr, uint8_t *buffer, uint32_t size);
void writeData(uint32_t addr, uint8_t *buffer, uint32_t size);
void memDump();

int main(void)
{
    lfs_t lfs;
    // global
	static uint8_t lfs_readBuff[256];
	static uint8_t lfs_writeBuff[256];
	static uint8_t lfs_lookaheadBuff[128];
	// configuration of the filesystem is provided by this struct
	const struct lfs_config cfg = {
		// block device operations
		/*
		.read = ramfs_read,
		.prog = ramfs_prog,
		.erase = ramfs_erase,
		.sync = ramfs_sync,
		*/
		.read = eepromfs_read,
		.prog = eepromfs_prog,
		.erase = eepromfs_erase,
		.sync = eepromfs_sync,

		// block device configuration
		.read_size = 256,
		.prog_size = 256,
		.block_size = 256, // size of a single block, flash memory is arranged in sectorse(eg. 25cms04 ic)
		.block_count = 2048, // total size of 25CSM04 are 524,288 bytes
		.block_cycles = 100, // not less then 100
		.cache_size = 256,
		.lookahead_size = 128,
		// static buffer to avoid malloc and free [bad things happen whene they are used]
        // use LFS_NO_MALLOC define flage to disable use of malloc
		.read_buffer = lfs_readBuff,
		.prog_buffer = lfs_writeBuff,
		.lookahead_buffer = lfs_lookaheadBuff,
	};
    printf("Hello Little FS.\r\n");
    // mount the filesystem
	int err = lfs_mount(&lfs, &cfg);
	// reformat if we can't mount the filesystem
	// this should only happen on the first boot
	if (err)
	{
		int err2 = lfs_format(&lfs, &cfg);
		int err3 = lfs_mount(&lfs, &cfg);
		if (err3)
		{
			printf("Fail to mount, error=%d\r\n",err3);
		}
	}
    //memDump();
    return 0;
}

// simple memory modeled using ram
void memDump()
{
    for(int i=0;i<10*1024;i++)
    {
        printf("%2d", eeprom[i]);
        if(i%32==0)
        {
            printf("\r\n");
        }
    }
}
// block size is 256, 256 bytes can be writen at a time.
void readData(uint32_t addr, uint8_t *buffer, uint32_t size)
{
    if(size > 256)
    {
        printf("! block size exceeded");
    }
    for(uint32_t i=0; i<size;i++)
    {
        buffer[i] = eeprom[i+addr];
    }
}
void writeData(uint32_t addr, uint8_t *buffer, uint32_t size)
{
    if(size > 256)
    {
        printf("! block size exceeded");
    }
    for(uint32_t i=0; i<size;i++)
    {
        eeprom[i+addr] = buffer[i];
    }
}
// EEPROM wrapper funciton for the littlefs lib
// For now it will use a buffer in ram for storage
int eepromfs_read(const struct lfs_config *c, lfs_block_t block,
				  lfs_off_t off, void *buffer, lfs_size_t size)
{
	int ret = 0; // currently no error detection
    // use define block size to calculate the address in memory
	uint32_t addr = block * (c->block_size) + off;
	readData(addr, (uint8_t *)buffer, size);
	if (ret == -1)
	{
		return -1;
	}
	else
	{
		return 0;
	}
}
int eepromfs_prog(const struct lfs_config *c, lfs_block_t block,
				  lfs_off_t off, const void *buffer, lfs_size_t size)
{
	int ret = 0;
	uint32_t addr = block * c->block_size + off;
	writeData(addr, (uint8_t *)buffer, size);
	if (ret == -1)
	{
		return -1;
	}
	else
	{
		return 0;
	}
}
int eepromfs_erase(const struct lfs_config *c, lfs_block_t block)
{
	int block_addr = block * 256;
	int ret = 0;
	const uint8_t data[256] = {0xff};
	writeData(block_addr, (uint8_t*)data, 256);
	if (ret == -1)
	{
		return -1;
	}
	else
	{
		return 0;
	}
}
int eepromfs_sync(const struct lfs_config *c)
{
	return 0;
}