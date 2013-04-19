#include "eFile.h"
#include "eDisk.h"
#include "retarget.h"
#include "ctype.h"
#include "os.h"
#include <stdio.h>
#include <string.h>

#define memcpy memmove

static unsigned char _blockBuff[BLOCK_SIZE];
static eFile_File _wFile, _rFile, _file;
static char _wOpen, _rOpen, _sysInit = 0;
static unsigned int _wIndex, _wCluster, _wSector, _wSize, _dirIndex, _wDirIndex;
static unsigned int _rIndex, _rCluster, _rSector, _rSize;
static unsigned char _writeBuff[BLOCK_SIZE], _readBuff[BLOCK_SIZE];

static unsigned short FAT_OFFSET = 0;
static unsigned char	SECT_PER_CLUSTER = 0;
static unsigned char	NUM_FATS = 0;
static unsigned int		FAT_SIZE = 0;
static unsigned int		ROOT_OFFSET = 0;
static unsigned int		_dir = 0;
//static unsigned int		_cluster = 0;
static unsigned long format[] = {0xeb58904d, 0x53444f53, 0x352e3000, 0x02202a09,
													0x02000000, 0x00f80000, 0x3f00ff00, 0x00000000,
													0x0024b703, 0x6b3b0000, 0x00000000, 0x02000000,
													0x01000600, 0x00000000, 0x00000000, 0x00000000,
													0x80002989, 0x4fafc84e, 0x4f204e41, 0x4d452020,
													0x20204641, 0x54333220, 0x202033c9, 0x8ed1bcf4,
													0x7b8ec18e, 0xd9bd007c, 0x884e028a, 0x5640b441,
													0xbbaa55cd, 0x13721081, 0xfb55aa75, 0x0af6c101,
													0x7405fe46, 0x02eb2d8a, 0x5640b408, 0xcd137305,
													0xb9ffff8a, 0xf1660fb6, 0xc640660f, 0xb6d180e2,
													0x3ff7e286, 0xcdc0ed06, 0x41660fb7, 0xc966f7e1,
													0x668946f8, 0x837e1600, 0x7538837e, 0x2a007732,
													0x668b461c, 0x6683c00c, 0xbb0080b9, 0x0100e82b,
													0x00e92c03, 0xa0fa7db4, 0x7d8bf0ac, 0x84c07417,
													0x3cff7409, 0xb40ebb07, 0x00cd10eb, 0xeea0fb7d,
													0xebe5a0f9, 0x7debe098, 0xcd16cd19, 0x6660807e,
													0x02000f84, 0x2000666a, 0x00665006, 0x53666810,
													0x000100b4, 0x428a5640, 0x8bf4cd13, 0x66586658,
													0x66586658, 0xeb33663b, 0x46f87203, 0xf9eb2a66,
													0x33d2660f, 0xb74e1866, 0xf7f1fec2, 0x8aca668b,
													0xd066c1ea, 0x10f7761a, 0x86d68a56, 0x408ae8c0,
													0xe4060acc, 0xb80102cd, 0x1366610f, 0x8275ff81,
													0xc3000266, 0x40497594, 0xc3424f4f, 0x544d4752,
													0x20202020, 0x00000000, 0x00000000, 0x00000000,
													0x00000000, 0x00000000, 0x00000000, 0x00000000,
													0x00000000, 0x00000000, 0x00000000, 0x00000000,
													0x00000000, 0x00000000, 0x00000000, 0x0d0a5265,
													0x6d6f7665, 0x20646973, 0x6b73206f, 0x72206f74,
													0x68657220, 0x6d656469, 0x612eff0d, 0x0a446973,
													0x6b206572, 0x726f72ff, 0x0d0a5072, 0x65737320,
													0x616e7920, 0x6b657920, 0x746f2072, 0x65737461,
													0x72740d0a, 0x00000000, 0x00accbd8, 0x000055aa };
													
static OS_SemaphoreType _clusterSemaphore, _fileModifySemaphore;

//---------- eFile_Init-----------------
// Activate the file system, without formating
// disk periodic task needs to be running for this to work
// Input: none
// Output: 0 if successful and 1 on failure (already initialized)
// since this program initializes the disk, it must run with 
//    the disk periodic task operating
int eFile_Init(void) {
  // initialize file system
	if(_sysInit)
		return 1;
  if(eDisk_Init(DRIVE) || eDisk_ReadBlock(_blockBuff, BOOT_SECTOR))
		return 1;
	
	SECT_PER_CLUSTER = _blockBuff[SECT_PER_CLUST_INDEX];
	FAT_OFFSET = (_blockBuff[RES_SECT_INDEX + 1] << 8) | _blockBuff[RES_SECT_INDEX];
	NUM_FATS = _blockBuff[NUM_FATS_INDEX];
	FAT_SIZE = (_blockBuff[FAT_SIZE_INDEX + 3] << 24)
					 | (_blockBuff[FAT_SIZE_INDEX + 2] << 16)
					 | (_blockBuff[FAT_SIZE_INDEX + 1] << 8)
					 | _blockBuff[FAT_SIZE_INDEX]; // fat size in sectors
	_dir = ROOT_OFFSET = FAT_OFFSET + (NUM_FATS * FAT_SIZE);
	
	OS_InitSemaphore(&_clusterSemaphore, 1);
	OS_InitSemaphore(&_fileModifySemaphore, 1);
	
	_sysInit = 1;
	return 0;
}

int eFile_Info(void)
{
	CHECK_DISK // check disk state
	
	printf("Sectors per cluster: %d\n", SECT_PER_CLUSTER);
	printf("FAT start index: %d\n", FAT_OFFSET);
	printf("Number of FATs: %d\n", NUM_FATS);
	printf("Sectors per FAT: %d\n", FAT_SIZE);
	printf("Root start index: %d\n", ROOT_OFFSET);
	return 0;
}

//---------- eFile_Format-----------------
// Erase all files, create blank directory, initialize free space manager
// Input: none
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_Format(void)
{
	int i;
	
	CHECK_DISK // check disk state
	
	eFile_WClose();
	eFile_RClose();
	
	// Sector 0
	_eFile_ClearBlockBuff();
	for(i = 0; i < 128; i++)
	{
		char a, b, c, d;
		a = (char) (format[i] & 0xFF);
		b = (char) ((format[i] >> 8) & 0xFF);
		c = (char) ((format[i] >> 16) & 0xFF);
		d = (char) ((format[i] >> 24) & 0xFF);
		format[i] = ((a << 24) | (b << 16) | (c << 8) | d);
		memcpy(&_blockBuff[i*4], &format[i], 4);
	}
	eDisk_WriteBlock(_blockBuff, 0);
	// Root dir at 0x8000 = 32,768
	_eFile_ClearBlockBuff();
	for(i = 0; i < SECT_PER_CLUSTER; i++)
		eDisk_WriteBlock(_blockBuff, 0x8000 + i);
	// First FAT dir at 2346
	memset(_blockBuff, 0xFF, 16);
	_blockBuff[0] = 0xF8;
	_blockBuff[3] = _blockBuff[7] = 
		_blockBuff[11] = _blockBuff[15] = 0x0F;
	eDisk_WriteBlock(_blockBuff, 2346);

	eFile_Init();
  return 0;
}

//---------- eFile_Create-----------------
// Create a new, empty file with one allocated block
// Input: file name is an ASCII string up to seven characters 
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_Create(const char name[FILE_NAME_SIZE], char attr)
{
	int cluster, i;
	
	CHECK_DISK // check disk state
	
	OS_bWait(&_fileModifySemaphore);
	memset(&_file, 0, sizeof(eFile_File));
	_file = _eFile_Find(name, 0);
	if(_file.name[0]) // exists
	{
		OS_bSignal(&_fileModifySemaphore);
		return 1;
	}
	
	_eFile_FATName(name, _file.name, &toupper, 0); // somewhat unsafe; overflows to ext
	_file.attr = attr; // archive is 0x20, dir is 0x10
	cluster = _eFile_FreeCluster(); // also marks as in-use
//	if(attr == 0x10)
//			cluster += 2;
	_file.cluster[0] = cluster & 0xFF;
	_file.cluster[1] = (cluster >> 8) & 0xFF;
	_file.hiCluster[0] = (cluster >> 16) & 0xFF;
	_file.hiCluster[1] = (cluster >> 24) & 0xFF;
	memset(_file.size, 0, 4);
	
	if(cluster < 0)
	{
		OS_bSignal(&_fileModifySemaphore);
		return 1; // no free space
	}
	
	eDisk_ReadBlock(_blockBuff, _dir);
	// write two empty files
	if(!_blockBuff[0])
		_blockBuff[0] = 0xe5;
	if(!_blockBuff[32])
		_blockBuff[32] = 0xe5;
	
	for(i = 64; i < 512; i += 32) // doesn't support large dirs
	{
		if(_blockBuff[i] != 0xe5 && _blockBuff[i] != 0x00)
			continue;
		memcpy(&_blockBuff[i], &_file, sizeof(eFile_File));
		eDisk_WriteBlock(_blockBuff, _dir);
/*		if(attr == 0x10) // make directory
		{
			// directory "."
			memset(&_file, 0, 32);
			memset(&_file.name[1], 0x20, 10);
			_file.name[0] = '.';
			_file.attr = 0x10;
			memset(&_file.size[0], 0, 4);
			_file.cluster[0] = cluster & 0xFF;
			_file.cluster[1] = (cluster >> 8) & 0xFF;
			_file.hiCluster[0] = (cluster >> 16) & 0xFF;
			_file.hiCluster[1] = (cluster >> 24) & 0xFF;
			memset(_blockBuff, 0, 512);
			memcpy(_blockBuff, &_file, 32);
			// directory ".."
			memset(&_file, 0, 32);
			memset(&_file.name[2], 0x20, 9);
			_file.name[0] = _file.name[1] = '.';
			_file.attr = 0x10;
			memset(&_file.size[0], 0, 4);
			_file.cluster[0] = _cluster & 0xFF;
			_file.cluster[1] = (_cluster >> 8) & 0xFF;
			_file.hiCluster[0] = (_cluster >> 16) & 0xFF;
			_file.hiCluster[1] = (_cluster >> 24) & 0xFF;
			memcpy(&_blockBuff[32], &_file, 32);
			eDisk_WriteBlock(_blockBuff, (ROOT_OFFSET + SECT_PER_CLUSTER*(cluster-2)));
		}*/
		_dirIndex = i;
		OS_bSignal(&_fileModifySemaphore);
		return 0;
	}
	OS_bSignal(&_fileModifySemaphore);
	return 1; // no space in dir
}

//---------- eFile_WOpen-----------------
// Open the file, read into RAM last block
// Input: file name is a single ASCII letter
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_WOpen(const char name[FILE_NAME_SIZE])
{
	CHECK_DISK // check disk state
	
	memset(&_file, 0, sizeof(eFile_File));
	if(_wOpen)
		return 1;
	
	OS_bWait(&_fileModifySemaphore);
	_file = _eFile_Find(name, 0);
	if(!_file.name[0])
	{
		OS_bSignal(&_fileModifySemaphore);
		if(eFile_Create(name, 0x20))
			return 1;
		OS_bWait(&_fileModifySemaphore);
	}
	
	_wFile = _file;
	_wDirIndex = _dirIndex;
	_wCluster = (_file.cluster[0] | (_file.cluster[1] << 8));
	_wSize = _file.size[0] | (_file.size[1] << 8) |
					(_file.size[2] << 16) | (_file.size[3] << 24);
	_wIndex = _wSize % 512;
	// seek to end of file
	eDisk_ReadBlock(_writeBuff, FAT_OFFSET + _wCluster / 128);
	while(1)
	{
		int next, // next cluster
				index;
		index = (_wCluster % 128) * 4;
		next = _writeBuff[index] | (_writeBuff[index + 1] << 8)
					 | (_writeBuff[index + 2] << 16) | (_writeBuff[index + 3] << 24);
		if((next & 0x0FFFFFF8) == 0x0FFFFFF8)
			break; // last block
		if((FAT_OFFSET + _wCluster / 128) != (FAT_OFFSET + next / 128))
			eDisk_ReadBlock(_writeBuff, FAT_OFFSET + next / 128);
		_wCluster = next;
	}
	_wSector = ROOT_OFFSET + SECT_PER_CLUSTER*(_wCluster - 2) + ((_wSize % (32 * 512)) / 512);
	eDisk_ReadBlock(_writeBuff, _wSector);
	_wOpen = 1;
	OS_bSignal(&_fileModifySemaphore);
  return 0;
}


//---------- eFile_Write-----------------
// save at end of the open file
// Input: data to be saved
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_Write(const char data)
{
	CHECK_DISK // check disk state
	
	if(!_wOpen || !data)
		return 1;
	
	OS_bWait(&_fileModifySemaphore);
	if(_wIndex == 512) // need to allocate more data
	{
		_wSector++;
		if(_wSector % SECT_PER_CLUSTER == 0)
		{
			// allocate new cluster from FAT
			int next = _eFile_FreeCluster();
			if(next < 0)
			{
				OS_bSignal(&_fileModifySemaphore);
				return 1;
			}
			eDisk_WriteBlock(_writeBuff, _wSector);
			eDisk_ReadBlock(_blockBuff, FAT_OFFSET + _wCluster / 128);
			_blockBuff[(_wCluster % 128) * 4] = next & 0xFF;
			_blockBuff[(_wCluster % 128) * 4 + 1] = (next >> 8) & 0xFF;
			_blockBuff[(_wCluster % 128) * 4 + 2] = (next >> 16) & 0xFF;
			_blockBuff[(_wCluster % 128) * 4 + 3] = (next >> 24) & 0xFF;
			eDisk_WriteBlock(_blockBuff, FAT_OFFSET + _wCluster / 128);
			_wCluster = next;
			_wSector = ROOT_OFFSET + SECT_PER_CLUSTER*(_wCluster - 2);
			eDisk_ReadBlock(_writeBuff, _wSector);
			_wIndex = 0;
		}
		else
		{
			eDisk_WriteBlock(_writeBuff, _wSector - 1);
			eDisk_ReadBlock(_writeBuff, _wSector);
			_wIndex = 0;
		}
	}
	_writeBuff[_wIndex++] = data;
	_wSize++;
	OS_bSignal(&_fileModifySemaphore);
  return 0;
}

//---------- eFile_Close-----------------
// Deactivate the file system
// Input: none
// Output: 0 if successful and 1 on failure (not currently open)
int eFile_Close(void)
{
	CHECK_DISK // check disk state

	if(_wOpen || _rOpen)
	{
		eFile_WClose();
		eFile_RClose();
	}	
  return (_sysInit = 0);
}

//---------- eFile_WClose-----------------
// close the file, left disk in a state power can be removed
// Input: none
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_WClose(void)
{
	CHECK_DISK // check disk state
	
  // close the file for writing
  if(!_wOpen)
		return 1;
	// acquire semaphore to modify file
	OS_bWait(&_fileModifySemaphore);
	// Write the data to the file
	eDisk_WriteBlock(_writeBuff, _wSector);
	// Update file size and write
	_wFile.size[0] = (_wSize & 0xFF);
	_wFile.size[1] = ((_wSize >> 8) & 0xFF);
	_wFile.size[2] = ((_wSize >> 16) & 0xFF);
	_wFile.size[3] = ((_wSize >> 24) & 0xFF);
	eDisk_ReadBlock(_blockBuff, _dir);
	memcpy(&_blockBuff[_wDirIndex], &_wFile, 32 /* bytes */);
	eDisk_WriteBlock(_blockBuff, _dir);
	// release semaphore and return
	OS_bSignal(&_fileModifySemaphore);
	return (_wOpen = 0);
}

//---------- eFile_ROpen-----------------
// Open the file, read first block into RAM 
// Input: file name is a single ASCII letter
// Output: 0 if successful and 1 on failure (e.g., trouble read to flash)
int eFile_ROpen(const char name[FILE_NAME_SIZE])
{
	CHECK_DISK // check disk state
	
  // open a file for reading
	memset(&_file, 0, sizeof(eFile_File));
	if(_rOpen)
		return 1;
	
	OS_bWait(&_fileModifySemaphore);
	_file = _eFile_Find(name, 0);
	if(!_file.name[0])
	{
		OS_bSignal(&_fileModifySemaphore);
		return 1;
	}
	
	_rFile = _file;
	_rCluster = (_file.cluster[0] | (_file.cluster[1] << 8) |
							(_file.hiCluster[0] << 16) | (_file.hiCluster[1] << 24));
	_rSector = ROOT_OFFSET + SECT_PER_CLUSTER*(_rCluster - 2);
	_rIndex = 0;
	eDisk_ReadBlock(_readBuff, _rSector);
	_rOpen = 1;
	_rSize = 0;
	OS_bSignal(&_fileModifySemaphore);
  return 0;
}

//---------- eFile_ReadNext-----------------
// retreive data from open file
// Input: none
// Output: return by reference data
//         0 if successful and 1 on failure (e.g., end of file)
int eFile_ReadNext(char *pt)
{
	int size = (_rFile.size[3] << 24) | (_rFile.size[2] << 16)
						 | (_rFile.size[1] << 8) | _rFile.size[0];
	
	CHECK_DISK // check disk state
  if(_rSize >= size)
		return 1;
	
	OS_bWait(&_fileModifySemaphore);
	if(_rIndex == 512)
	{
		// get next sector
		_rSector++;
		if(_rSector % SECT_PER_CLUSTER == 0)
		{
			// fetch next cluster from FAT
			int next, c;
			eDisk_ReadBlock(_blockBuff, FAT_OFFSET + _rCluster / 128);
			c = (_rCluster % 128) * 4;
			next = (_blockBuff[c + 3] << 24) | (_blockBuff[c + 2] << 16)
					 | (_blockBuff[c + 1] << 8)  | _blockBuff[c];
			if((next & 0x0FFFFFF8) == 0x0FFFFFF8) // no more clusters
			{
				OS_bSignal(&_fileModifySemaphore);
				return 1;
			}
			_rCluster = next;
			_rSector = ROOT_OFFSET + SECT_PER_CLUSTER*(_rCluster - 2);
			eDisk_ReadBlock(_readBuff, _rSector);
			_rIndex = 0;
		}
		else
		{
			eDisk_ReadBlock(_readBuff, _rSector);
			_rIndex = 0;
		}
	}
	
	*pt = _readBuff[_rIndex++];
	_rSize++;
	OS_bSignal(&_fileModifySemaphore);
  return 0;
}

//---------- eFile_RClose-----------------
// close the reading file
// Input: none
// Output: 0 if successful and 1 on failure (e.g., wasn't open)
int eFile_RClose(void)
{
	CHECK_DISK // check disk state
	
  // close the file for writing
  if(!_rOpen)
		return 1;
  return (_rOpen = 0);
}

//---------- eFile_Directory-----------------
// Display the directory with filenames and sizes
// Input: pointer to a function that outputs ASCII characters to display
// Output: characters returned by reference
//         0 if successful and 1 on failure (e.g., trouble reading from flash)
int eFile_Directory(int(*fp)(const char *format, ...))
{	
	int offset = 0;
	
	CHECK_DISK // check disk state
	
	while(1)
	{
		int index;
		eDisk_ReadBlock(_blockBuff, _dir + offset);
		for(index = 0; index < 512; index += 32)
		{
			int i;
			unsigned char* pt = &_blockBuff[index];

			if(_blockBuff[index] == 0xE5)
				continue;
			else if(_blockBuff[index] == 0x00)
				return 0;
			for(i = 0; *pt != 0x20 && i < 8; i++, pt++)
				fp("%c", tolower(*pt));
			pt = &_blockBuff[index + 8];
			if(*pt != 0x20 && *pt != 0x00)
			{
				fp(".");
				for(i = 0; *pt != 0x20 && i < 4; i++, pt++)
					fp("%c", tolower(*pt));
			}
			
			if(_blockBuff[index + 11] & 0x10) // directory
			{
				fp("/\n");
				continue;
			}
			
			i = _blockBuff[index + 28] | (_blockBuff[index + 29] << 8) |
					(_blockBuff[index + 30] << 16) | (_blockBuff[index + 31] << 24);
			fp(": %d Bytes\n", i);
		}
		offset++;
	}
}

int eFile_List(char list[MAX_FILES][13])
{	
	int i, index;
	
	CHECK_DISK // check disk state
	
	eDisk_ReadBlock(_blockBuff, _dir);
	for(i = 0, index = 0; i < 512; i += 32)
	{
		int j, k;
		if(_blockBuff[i] == 0x00 || _blockBuff[i] == 0xe5)
			continue;
		for(j = 0; j < 8 && _blockBuff[i+j] != 0x20; j++)
		{
			list[index][j] = tolower(_blockBuff[i + j]);
			//printf("%c", list[index][j]);
		}
		list[index][j++] = '.';
		for(k = 8; k < 11 && _blockBuff[i+k] != 0x20; j++, k++)
			list[index][j] = tolower(_blockBuff[i + k]);
		index++;
	}
	return 0;
}

//---------- eFile_Delete-----------------
// delete this file
// Input: file name is a single ASCII letter
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_Delete(const char name[FILE_NAME_SIZE])
{
	int cluster;
	
	CHECK_DISK // check disk state
	
  _file = _eFile_Find(name, 0);
	if(!_file.name[0])
		return 1; // does not exist
	
	OS_bWait(&_fileModifySemaphore);
	_file.name[0] = 0xe5; // mark as deleted
	eDisk_ReadBlock(_blockBuff, _dir);
	memcpy(&_blockBuff[_dirIndex], &_file, 32 /* bytes */);
	eDisk_WriteBlock(_blockBuff, _dir);
	// reclaim FAT entries
	cluster = (_file.cluster[0] | (_file.cluster[1] << 8) |
							(_file.hiCluster[0] << 16) | (_file.hiCluster[1] << 24));
	eDisk_ReadBlock(_blockBuff, FAT_OFFSET + cluster / 128);
	while(1)
	{
		int next, // next cluster
				index;
		index = (cluster % 128) * 4;
		next = _blockBuff[index] | (_blockBuff[index + 1] << 8)
					 | (_blockBuff[index + 2] << 16) | (_blockBuff[index + 3] << 24);
		_blockBuff[index] = _blockBuff[index + 1] = 
			_blockBuff[index + 2] = _blockBuff[index + 3] = 0;
		if((next & 0x0FFFFFF8) == 0x0FFFFFF8)
			break; // last block
		if((FAT_OFFSET + cluster / 128) != (FAT_OFFSET + next / 128))
		{
			eDisk_WriteBlock(_blockBuff, FAT_OFFSET + cluster / 128);
			eDisk_ReadBlock(_blockBuff, FAT_OFFSET + next / 128);
		}
		cluster = next;
	}
	eDisk_WriteBlock(_blockBuff, FAT_OFFSET + cluster / 128);
	OS_bSignal(&_fileModifySemaphore);
	return 0;
}

//---------- eFile_RedirectToFile-----------------
// open a file for writing 
// Input: file name is a single ASCII letter
// stream printf data into file
// Output: 0 if successful and 1 on failure (e.g., trouble read/write to flash)
int eFile_RedirectToFile(const char *name)
{
	CHECK_DISK // check disk state
	
  if(eFile_WOpen(name)) // creates file if doesn't exist
	{
		printf("Cannot open file\n");
		return 1;  // cannot open file
	}
	RT_StreamToFile(1);
  return 0;
}

//---------- eFile_EndRedirectToFile-----------------
// close the previously open file
// redirect printf data back to UART
// Output: 0 if successful and 1 on failure (e.g., wasn't open)
int eFile_EndRedirectToFile(void)
{
	CHECK_DISK // check disk state
	
  RT_StreamToFile(0);
	if(eFile_WClose())
		return 1;
  return 0;
}


int eFile_ChangeDirectory(char newdir[13])
{
	int cluster, sector;
	
	CHECK_DISK // check disk state
	
	_file = _eFile_Find(newdir, 1);
	if(!_file.name[0] || _file.attr != 0x10)
		return 1;
	cluster = (_file.cluster[0] | (_file.cluster[1] << 8) |
							(_file.hiCluster[0] << 16) | (_file.hiCluster[1] << 24));
//	_cluster = cluster;
	if(cluster == 0)
		cluster += 2;
	sector = ROOT_OFFSET + SECT_PER_CLUSTER*(cluster-2);
	_dir = sector;
	return 0;
}

static void _eFile_ClearBlockBuff(void) {
  memset(_blockBuff, 0, sizeof(unsigned char) * BLOCK_SIZE);
}

static eFile_File _eFile_Find(const char name[FILE_NAME_SIZE], int directory)
{
	eFile_File file;
	int index, done = 0;
	char n[FILE_NAME_SIZE];
	
	eDisk_ReadBlock(_blockBuff, _dir);
	_eFile_FATName(name, n, &tolower, directory); // convert name
	
	// search through files
	for(index = 64; index < 512; index += 32)
	{
		int i, found = 1;
		
		if(_blockBuff[index] == 0xE5)
			continue;
		else if(_blockBuff[index] == 0x00)
			break;		
		// compare name
		for(i = 0; i < 11; i++)
		{
			if(tolower(_blockBuff[index+i]) != n[i])
			{
				found = 0;
				break;
			}
		}
		if(!found)
			continue;
		// found it - populate struct
		memcpy(&file, &_blockBuff[index], 32 /* bytes */);
		done = 1;
		_dirIndex = index;
		break;
	}
	if(!done)
		file.name[0] = 0;
	return file;
}

static void _eFile_FATName(const char name[FILE_NAME_SIZE], char n[FILE_NAME_SIZE], int (*f)(int), int dir)
{
	int i, j;
	for(i = j = 0; i < 8; i++)
	{
		if((name[j] == '.' && !dir) || name[j] == 0x00)
			n[i] = ' ';
		else
			n[i] = f(name[j++]);
	}
	if((name[j] == '.' && !dir))
		j++;
	for(i = 8; i < 11; i++)
	{
		if((name[j] == '.' && !dir) || name[j] == 0x00)
			n[i] = ' ';
		else
			n[i] = f(name[j++]);
	}
}

static int _eFile_FreeCluster(void)
{
	// find free cluster in FAT and mark as allocated
	int i;
	OS_bWait(&_clusterSemaphore);
	for(i = 0; i < FAT_SIZE; i++)
	{
		int index;
		eDisk_ReadBlock(_blockBuff, i + FAT_OFFSET);
		for(index = 0; index < 512; index += 4)
		{
			int sector, j;
			if(_blockBuff[index] || _blockBuff[index + 1]
					|| _blockBuff[index + 2] || _blockBuff[index + 3])
				continue;
			_blockBuff[index] = _blockBuff[index + 1] = _blockBuff[index + 2] = 0xFF;
			_blockBuff[index + 3] = 0x0F;
			eDisk_WriteBlock(_blockBuff, i + FAT_OFFSET);
			sector = ROOT_OFFSET + SECT_PER_CLUSTER*(((i * 128 + index) / 4) - 2);
			_eFile_ClearBlockBuff();
			for(j = 0; j < SECT_PER_CLUSTER; j++)
				eDisk_WriteBlock(_blockBuff, sector + j); // clear out newly allocated cluster
			OS_bSignal(&_clusterSemaphore);
			return (i * 128 + index) / 4;
		}
	}
	OS_bSignal(&_clusterSemaphore);
	return -1;
}
