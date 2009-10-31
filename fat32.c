#include "fat32.h"
#include "sdcard.h"
#include "serial.h"
#include "convert.h"

#include <inttypes.h>
#include <ctype.h>

/* macros to aid in reading data from byte buffers */
#define GET32(p) (*((uint32_t*)(p)))
#define GET16(p) (*((uint16_t*)(p)))
#define FAT_EOF 0x0ffffff8

/* the global fat32 fs structure */
static struct fat32fs_t fat;
#define CLUSTER(cn) ((fat.cluster_begin_lba + ((uint32_t)((cn ? cn : fat.root_dir_first_cluster) & 0x0fffffff) - 2) * fat.sectors_per_cluster) & 0x0fffffff)

/* global directory entry data for parameter passing and holding the current directory */
static struct fat32dirent_t ret_file;
static struct fat32dirent_t cur_dir;
static const char* fncmp;

/* shared sector buffers */
/* each routine is responsible for writing its own buffer modifications to memory */
static uint8_t sect[512];
static uint32_t cur_sect;
static uint8_t* cur_line;
static uint32_t fatsect[512 / sizeof(uint32_t)];
static uint32_t cur_fatsect;

/* abstract readsector with error checking */
int readsector(uint32_t lba, uint8_t *buffer)
{
	int r;
	if ((r = mmc_readsector(lba, buffer)))
		send_str("error reading sector\n");
	return r;
}

/* abstract writesector with error checking */
int writesector(uint32_t lba, uint8_t *buffer)
{
	int r;
	if ((r = mmc_writesector(lba, buffer)))
		send_str("error writing sector\n");
	return r;
}

/* string conversion function */
const char* str_to_fat(const char * str)
{
	static char name[12];
	int i, j;
	
	// copy first eight characters
	for (i=0; i<11; i++) {
		if ((i > 0 && str[i] == '.' && !(i == 1 && str[0] == '.')) || str[i] == '\0')
			break;
		name[i] = (isalnum(str[i]) || str[i] == '.' || str[i] == '~') ? toupper(str[i]) : '_';
	}
	
	// pad with spaces
	for (j=i; j<11; j++) name[j] = ' ';
	
	// copy extension
	if (str[i] == '.') {
		i++;
		for (j=0; j<3; j++) {
			if (str[i+j] == '\0') break;
			name[8+j] = isalnum(str[i+j]) ? toupper(str[i+j]) : '_';
		}
	}
	
	name[11] = '\0';
	
	return name;
}

/* reads the next cluster from the FAT */
uint32_t fat_readnext(uint32_t cur_cluster)
{
	// compute FAT sector and index
	uint32_t s = ((cur_cluster >> 7) + fat.fat_begin_lba);
	uint16_t i = cur_cluster & 0x7f;
	
	// load next FAT sector into buffer
	if (cur_fatsect != s) {
		cur_fatsect = s;
		readsector(cur_fatsect, (uint8_t*)fatsect);
	}
	
	// return next cluster
	return fatsect[i];
}

/* writes the next cluster to the FAT, returning what it was */
/* uses a blocking write-through philosophy for the buffer, which can be inefficient */
uint32_t fat_writenext(uint32_t cur_cluster, uint32_t new_cluster)
{
	uint32_t r;

	// load the next FAT sector into buffer
	r = fat_readnext(cur_cluster);

	// write FAT sectors (mirrored FATs)
	fatsect[cur_cluster & 0x7f] = new_cluster;
	writesector(cur_fatsect, (uint8_t*)fatsect);
	writesector(cur_fatsect + fat.sectors_per_fat, (uint8_t*)fatsect);

	// return next cluster
	return r;
}

/* clears a FAT cluster chain more efficiently than repeatedly calling fat_writenext() */
void fat_clearchain(uint32_t first_cluster)
{
	uint32_t cur_cluster = first_cluster;

	// compute FAT sector and index of the first cluster
	uint32_t s = ((cur_cluster >> 7) + fat.fat_begin_lba);
	uint16_t i = cur_cluster & 0x7f;
	

	while (cur_cluster != 0 && cur_cluster < FAT_EOF) {
		// load next FAT sector into buffer if not already there
		if (cur_fatsect != s) {
			cur_fatsect = s;
			readsector(cur_fatsect, (uint8_t*)fatsect);
		}
		
		// update FAT sector buffer
		cur_cluster = fatsect[i];
		fatsect[i] = 0;
	
		// compute FAT sector and index
		s = ((cur_cluster >> 7) + fat.fat_begin_lba);
		i = cur_cluster & 0x7f;

		// write FAT sector buffer to memory
		if (cur_fatsect != s || cur_cluster == 0 || cur_cluster >= FAT_EOF) {
			writesector(cur_fatsect, (uint8_t*)fatsect);
			writesector(cur_fatsect + fat.sectors_per_fat, (uint8_t*)fatsect);
		}
	}

}

/* fills a fat32dirent_t from raw data */
void load_dirent(struct fat32dirent_t * de, const uint8_t * d)
{
	de->attrib = d[0xb];

	// set type
	switch (*d) {
		case 0xe5 : de->type = DIRENT_BLANK; break;
		case 0x00 : de->type = DIRENT_END; break;
		default :
			if ((de->attrib & 0xf) == 0xf) de->type = DIRENT_EXTFILENAME;
			else if (de->attrib & 0x8) de->type = DIRENT_VOLLABEL;
			else de->type = DIRENT_FILE;
			break;
	}
	
	int i;
	
	if (de->type != DIRENT_FILE) return;
	
	// get filename
	for (i=0; i<11; i++) {
		de->filename[i] = d[i];
	}
	de->filename[11] = '\0';
	
	// get data
	de->cluster = ((uint32_t)GET16(d + 0x14) << 16) | GET16(d + 0x1a);
	de->size = GET32(d + 0x1c);
}

/* can read partitions 0-3 (primary partitions only) */
int init_partition(int part)
{
	uint8_t* p;
	
	// read MBR
	if (readsector(0, sect)) {
		send_str("error: problem reading sector\n");
		return 1;	
	} else {
		send_str("initializing partition #");
		send_int(part + 1);
		send_char('\n');
	}
	
	// sanity check
	if (GET16(sect + 510) != 0xaa55) {
		send_str("error: bad MBR\n");
		return 2;
	}
	
	// check if partition is fat32
	p = sect + 446 + part*16;
	if (p[4] != 0x0b && p[4] != 0x0c) {
		send_str("error: partition is not FAT32\nfound type code: 0x");
		send_hexbyte(p[4]);
		send_char('\n');
		return 3;
	}
	
	// partition start value
	fat.partition_begin_lba = GET32(p + 8);
	
	// read partition Volume ID
	if (readsector(fat.partition_begin_lba, sect)) {
		send_str("error: problem reading sector\n");
		return 1;	
	}
	
	// check bytes per sector (should be 512)
	if (GET16(sect + 0xb) != 512) {
		send_str("error: partition uses non-512 byte sectors\n");
		return 4;
	}
	
	// check for 2 FATs
	if (sect[0x10] != 2) {
		send_str("warning: partition not using mirrored FATs\nwrite routines may corrupt data\n");
	}
	
	// sanity check
	if (GET16(sect + 510) != 0xaa55) {
		send_str("error: missing Volume ID signature\n");
		return 2;
	}
	
	// read and compute important constants
	fat.sectors_per_cluster = sect[0xd];
	fat.sectors_per_fat = GET32(sect + 0x24);
	
	// start plus #_of_reserved_sectors
	fat.fat_begin_lba = fat.partition_begin_lba + GET16(sect + 0xe);

	// start plus (#_of_FATs * sectors_per_FAT)
	fat.cluster_begin_lba = fat.fat_begin_lba + (sect[0x10] * fat.sectors_per_fat);
	
	// root directory first cluster
	fat.root_dir_first_cluster = GET32(sect + 0x2c);
	
	// read in the first sector of the first FAT cluster
	cur_fatsect = fat.fat_begin_lba;
	readsector(fat.fat_begin_lba, (uint8_t*)fatsect);
	
	// set current directory to root directory
	cur_dir.cluster = fat.root_dir_first_cluster;
	
	return 0;
}

/* prints a text sector */
void print_sect(uint8_t* s)
{
	send_nstr((char*)s, 512);
}

/* prints a dirent */
char print_dirent(struct fat32dirent_t* de)
{
	if (de->type == DIRENT_FILE) {
		send_str(de->filename);
		if (IS_SUBDIR(*de)) send_str(" (dir)");
		send_char('\n');
	}
	
	return 0;
}

/* finds a dirent by name */
char find_dirent(struct fat32dirent_t* de)
{
	int i;
	char cmp = 1;
	if (de->type == DIRENT_FILE) {
		// compare names
		for (i=0; i<11; i++) {
			if (de->filename[i] != fncmp[i]) {
				// not a match
				cmp = 0;
				break;
			}
		}
		
		// store if match
		if (cmp) return 1;
	}
	
	ret_file.type = DIRENT_BLANK;
	return 0;
}

/* calls a subroutine for each dirent in the directory */
void loop_dir(uint32_t fcluster, char (*funct)(struct fat32dirent_t*))
{
	uint32_t cluster = fcluster;
	uint32_t fsect = cur_sect = CLUSTER(fcluster);
	readsector(cur_sect, sect);
	
	uint8_t* p = sect;
	struct fat32dirent_t d;
	
	// print all directory entries
	do {
		// load next sector, following cluster chains
		if (p - sect > 512) {
			cur_sect++;
		
			// get next cluster from fat
			if (cur_sect - fsect >= fat.sectors_per_cluster) {
				cluster = fat_readnext(cluster);
				cur_sect = fsect = CLUSTER(cluster);
			}
			
			if (cluster >= FAT_EOF) break;
			
			// read new sector
			readsector(cur_sect, sect);
			p = sect;
		}
	
		// fill dirent struct
		load_dirent(&d, p);
		
		// call provided function with each entry
		if (funct(&d)) {
			load_dirent(&ret_file, p);
			cur_line = p;
			break;
		}
		
		p += 32;
	} while (d.type != DIRENT_END);
}

/* calls a subroutine for each dirent in the directory */
void loop_file(uint32_t fcluster, void (*funct)(uint8_t *))
{
	uint32_t cluster = fcluster;
	uint32_t fsect = cur_sect = CLUSTER(fcluster);
	
	// loop through all sectors
	while (1) {
		// load next sector, following cluster chains
	
		// get next cluster from fat
		if (cur_sect - fsect >= fat.sectors_per_cluster) {
			cluster = fat_readnext(cluster);
			cur_sect = fsect = CLUSTER(cluster);
		}
		
		if (cluster >= FAT_EOF) break;
		
		// read new sector
		readsector(cur_sect, sect);
		
		// call provided function with each sector
		funct(sect);
		
		cur_sect++;
	}
}



/* higher level fs routines */
/* they do about what you would expect of them */

void ls(void)
{
	loop_dir(cur_dir.cluster, print_dirent);
}

void cd(const char * s)
{
	fncmp = str_to_fat(s);
	loop_dir(cur_dir.cluster, find_dirent);
	
	if (IS_SUBDIR(ret_file))
		cur_dir.cluster = ret_file.cluster;
}

/* doesn't check whether the new name already exists */
void rn(const char * s, const char * snew)
{
	int i;
	const char* n;
	
	// find file
	fncmp = str_to_fat(s);
	loop_dir(cur_dir.cluster, find_dirent);
	
	n = str_to_fat(snew);
	
	if (n[0] == ' ') return;
	
	// modify buffered sector memory
	if (IS_FILE(ret_file)) {
		// insert spaces
		for (i=0; i<11; i++)
			cur_line[i] = *n++;
	} else return;
	
	// write modified sector buffer to card
	writesector(cur_sect, sect);
}


void del(const char * s)
{
	// find file
	fncmp = str_to_fat(s);
	loop_dir(cur_dir.cluster, find_dirent);
	
	if (!IS_FILE(ret_file) || IS_SUBDIR(ret_file)) return;

	// clear FAT chain
	if (ret_file.cluster != 0)
		fat_clearchain(ret_file.cluster);
	
	// erase directory entry
	*cur_line = 0xe5;
	writesector(cur_sect, sect);
}

void cat(const char * s)
{
	// find file
	fncmp = str_to_fat(s);
	loop_dir(cur_dir.cluster, find_dirent);
	
	// print if a printable file
	if (IS_FILE(ret_file) && !IS_SUBDIR(ret_file))
		loop_file(ret_file.cluster, print_sect);
	else send_str("non-printable file");
	
	send_char('\n');
}


