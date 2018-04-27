#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <stdint.h>


#define SIZE_MAX 16777216

struct fileinfo {
	long *msg;
	const char *readhead;
	long readsize;
	long memload;
	unsigned long offset;
	unsigned long sizefile;
	int fd;
	uint8_t finished;
	
};


int refresh(struct fileinfo *file)
{
	if (file->offset == 0)
	{
		if (file->sizefile > SIZE_MAX)
		{
			file->msg = (long*)mmap(NULL, SIZE_MAX, PROT_READ, MAP_PRIVATE, file->fd, file->offset);
			if (file->msg == MAP_FAILED)
			{
				fprintf(stderr, "unable to map file fd :%d\n", file->fd);
				return -1;
			}
			file->offset = file->offset + SIZE_MAX;
			file->memload = SIZE_MAX;
			file->readsize = SIZE_MAX;
		}
		else
		{
			file->msg = (long*)mmap(NULL, file->sizefile, PROT_READ, MAP_PRIVATE, file->fd, file->offset);
			if (file->msg == MAP_FAILED)
			{
				fprintf(stderr, "unable to map file fd :%d\n", file->fd);
				return -1;
			}
			file->memload = file->sizefile;
			file->readsize = file->sizefile;
			file->finished = 1;
		}
		file->readhead = (char*)file->msg;
	}
	else
	{
		if (munmap(file->msg, file->memload) == -1)
		{
			fprintf(stderr, "something went terribly wrong, unable to munmap\n");
			return -1;
		}
		if (file->sizefile > SIZE_MAX)
		{
			file->msg = (long*)mmap(NULL, SIZE_MAX, PROT_READ, MAP_PRIVATE, file->fd, file->offset);
			if (file->msg == MAP_FAILED)
			{
				fprintf(stderr, "unable to map file fd :%d\n", file->fd);
				return -1;
			}
			file->offset = file->offset + SIZE_MAX;
			file->memload = SIZE_MAX;
			file->readsize = SIZE_MAX;
		}
		else
		{
			file->msg = (long*)mmap(NULL, file->sizefile, PROT_READ, MAP_PRIVATE, file->fd, file->offset);
			if (file->msg == MAP_FAILED)
			{
				fprintf(stderr, "unable to map file fd :%d\n", file->fd);
				return -1;
			}
			file->memload = file->sizefile;
			file->readsize = file->sizefile;
			file->finished = 1;
		}
		file->readhead = (char*)file->msg;
	}
	return 0;
}


int firstlf(struct fileinfo *file)
{
	int i = 0;
	while ( *((file->readhead)+i) != '\n' && (file->readsize)-i != 0)
	{
		i++;
	}
	if ( *((file->readhead)+i) != '\n' && (file->readsize)-i == 0)							//arrived at the end of the memory map but not at the end of the line		
	{
		if (file->finished == 1)
		{
			printf("File finished fd :%d\n", file->fd);
			return 1;
		}
		if (refresh(file) == -1)
		{
			fprintf(stderr, "error on refresh\n");
			return -1;
		}
		return firstlf(file);
	}
	else if ( *((file->readhead)+i) == '\n' && (file->readsize)-i == 0)						//arrived at the end of the memory map and at the end of the line
	{
		if (file->finished == 1)
		{
			printf("File finished fd :%d\n", file->fd);
			return 1;
		}
		if (refresh(file) == -1)
		{
			fprintf(stderr, "error on refresh\n");
			return -1;
		}
		return 0;
	}
	else if (*((file->readhead)+i) == '\n' && (file->readsize)-i != 0)						//arrived at the end of line but not at the end of the memory map
	{
		file->readhead = file->readhead + i + 1;
		file->readsize = file->readsize - (i+1);
		return 0;
	}
	
	
	fprintf(stderr, "nothing happened, well that's an error\n");
	fprintf(stderr, "error in firstlf function\n");
	return -1;
}


/*	this function reads int the memory map and fill the buffer with the first valid line found
 *	
 *	it accept a fileinfo struct, a buffer and it's length
 *
 *	it returns an int, 
 *	0 everything's good you can use the buffer and recall this function
 *  -1 huho there's a big error you should stop the the calling thread
 *  1 well it seems we are at the end of the file you should ignore what's in biffer
 *  2 we are at the end of the file too but this time you can use what's inside biffer
 */
int read(struct fileinfo *file, char* biffer, int lenbiffer)
{
	int status = 0;
	int t = 0;
	int i = 0;
	int j = 0;
	do
	{
		while ( *((file->readhead)+j) != '\n' && (file->readsize)-j != 0 && i < lenbiffer)
		{
			*(biffer + i) = *((file->readhead)+j);
			j++;
			i++;
		}
		if (i == lenbiffer) 																					//arrived at the end of the buffer
		{
			frpintf(stderr, "line too long for the biffer or the biffer too small\n");
			fprintf(stderr, "line skipped\n");
			t = firstlf(file);
			if (t == -1)
			{
				fprintf(stderr, "error in firstlf function");
				return -1;
			}
			if (t == 1)
			{
				return 1;
			}
			return read(file, biffer, lenbiffer);	
		}
		else if (*((file->readhead)+j) != '\n' && (file->readsize)-j == 0) 										//arrived at the end of the memory map but not a the end of the line
		{
			if (file->finished == 1)
			{
				printf("File finished fd :%d\n", file->fd);
				return 1;
			}
			if (refresh(file) == -1)
			{
				fprintf(stderr, "error on refresh\n");
				return -1;
			}
			j = 0;
			status = 1;
		}
		else if (*((file->readhead)+j) == '\n' && (file->readsize)-j == 0)										//arrived at the end of the memory map and at the end of the line
		{
			if (file->finished == 1)
			{
				printf("File finished fd :%d\n", file->fd);
				*(biffer + i) = '\n';
				return 2;
			}
			if (refresh(file) == -1)
			{
				fprintf(stderr, "error on refresh\n");
				return -1;
			}
			return 0;
		}
		else if (*((file->readhead)+j) == '\n' && (file->readsize)-j != 0)										//arrived at the end of the line but not of the memory map
		{
			file->readhead = file->readhead + j+1;
			file->readsize = (file->readsize)-(j+1);
			*(biffer + i) = '\n';
			return 0;
		}
		
			
	} while (status);
	
	fprintf(stderr, "nothing happened, well that's an error\n");
	fprintf(stderr, "error in read function\n");
	return -1;
}
 
 
 
void *reading(void* parametre)
{
	struct fileinfo file;
	struct fileinfo *fileptr = &file;
	fileptr->fd = (int)*(parametre);
	
	struct stat buff;
	struct stat *buffptr = &buff;
	fstat(fileptr->fd, buffptr);
	fileptr->sizefile = buffptr->st_size;
	
	fileptr->offset = 0;
	fileptr->finished = 0;
	
	char biffer[5*65];
	int lenbiffer = 5*65;
	
	if (refresh(fileptr) == -1)
	{
		/*stop the reading thread*/
	}
	
	
	
	
	
}