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
// when using static mode
int my_file_open(lfs_t *lfs, lfs_file_t *file,const char *path,int flags)
{
    static uint8_t file_buffer[256];
    static struct lfs_file_config uploadFileConfig;
	int rValue;
    uploadFileConfig.buffer = file_buffer;
    rValue = lfs_file_opencfg(lfs, file, path, flags, &uploadFileConfig);
	return rValue;
}

int main(void)
{
    lfs_t lfs;
    lfs_file_t file;
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
		.block_count = 40, // 10kb
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
    // run a simple test
    // int write test
    printf("Test 1: Int file.\r\n");
    for(int i=0;i<10;i++)
    {
        uint32_t test = 0;
        // intTest_file
        my_file_open(&lfs, &file, "intTest_file", LFS_O_RDWR | LFS_O_CREAT);
        lfs_file_read(&lfs, &file, &test, sizeof(test)); // /sizeof(test) -> 4
        printf("test_count:%d\r\n",test);
        test += 1;
        // return file pointer to zero, as lfs_file_read moves it from zero
        lfs_file_rewind(&lfs, &file);
        lfs_file_write(&lfs, &file, &test, sizeof(test));
        // remember the storage is not updated until the file is closed successfully
        lfs_file_close(&lfs, &file);
    }
    // store a string
    // demostrate chunck writing and reading, as embbeded devices dont have much ram
    printf("Test 2: Text file.\r\n");
    for(int i=0;i<1;i++)
    {
        char text_wr[128];
        char text_rd[128];
        // textTest_file
        strcpy(text_wr,"The Hobbit, or There and Back Again");
        my_file_open(&lfs, &file, "textTest_file", LFS_O_RDWR | LFS_O_CREAT);
        lfs_file_write(&lfs, &file, text_wr, strlen(text_wr));
        lfs_file_close(&lfs, &file); // important
        // open and read the file
        my_file_open(&lfs, &file, "textTest_file", LFS_O_RDWR | LFS_O_CREAT);
        int32_t len = lfs_file_size(&lfs, &file); // get the total file size
        lfs_file_read(&lfs, &file, &text_rd, len);
        lfs_file_close(&lfs, &file); // important
        printf("text file=%s \r\n",text_rd);
    }
    // fill the file system
    printf("Test 3: Text file.\r\n");
    char books[][128] =
    {
        "The Lord of the Rings, The Fellowship of the Ring\r\n",
        "The Lord of the Rings, The Two Towers\r\n",
        "The Lion, the Witch and the Wardrobe\r\n",
        "Alice's Adventures in Wonderland\r\n",
        "Howl's Moving Castle\r\n",
        "Coraline\r\n",
        "City of Ghosts\r\n"};
    for(int i=0;i<100;i++)
    {
        char text_wr[128+10];
        // textTest_file
        sprintf(text_wr,"%d : %s",i,books[rand() % 6]);
        my_file_open(&lfs, &file, "textTest_file2", LFS_O_RDWR | LFS_O_CREAT | LFS_O_APPEND);
        lfs_file_write(&lfs, &file, text_wr, strlen(text_wr));
        lfs_file_close(&lfs, &file); // important
    }
    // block
    // open file
    my_file_open(&lfs, &file, "textTest_file2", LFS_O_RDWR | LFS_O_CREAT);
    // read the file in chunks of 128, as the file will be vary big to read at once
    printf("----data in file----");
    for(int i=0;i<100;i++)
    {
        int r=0;
        char text_rd[128+10]={0};
        // open and read the file
        r = lfs_file_read(&lfs, &file, &text_rd, 128);
        if(r>0)
        {
            printf("%s",text_rd);
        }
        else
        {
            printf("error, or file end");
            break;
        }
    }
    lfs_file_close(&lfs, &file);
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
    //printf("addr=%d, size=%d \r\n", addr, size);
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