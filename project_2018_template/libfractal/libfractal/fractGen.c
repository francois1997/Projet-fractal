#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>

struct line{
	char *phrase;
	int len;
};

int widthmax = 4096;
int heigthmax = 2160;

struct line *newline(struct line *lin, int lenmax)
{
	int namelen = (rand()%(lenmax+1));
	int len = namelen;
	for (int i = 0; i < namelen; i++)
	{
		char tem = (char)((rand()%26)+97);
		*((lin->phrase)+i) = tem;
	}

	//width and heigth

	int width = (rand()%widthmax)+1;
	int heigth = (rand()%heigthmax)+1;
	char wid[5];
	char hei[5];
	sprintf(wid,"%d",width);
	sprintf(hei,"%d",heigth);
	*((lin->phrase)+len) = ' ';
	len++;
	int i = 0;
	while(wid[i] != '\0')
	{
		*((lin->phrase)+len+i) = wid[i];
		i++;
	}
	len = len + i;
	i = 0;
	*((lin->phrase)+len) = ' ';
	len++;
	while(hei[i] != '\0')
	{
		*((lin->phrase)+len+i) = hei[i];
		i++;
	}
	len = len + i;
	*((lin->phrase)+len) = ' ';
	len++;

	//a and b

	char a[6];
	char b[6];
	double da = (((double)(rand() % 200))/100)-1;
	double db = (((double)(rand() % 200))/100)-1;
	sprintf(a,"%.2f",da);
	sprintf(b,"%.2f",db);
	i=0;
	while (a[i] != '\0')
	{
		*((lin->phrase)+len+i) = a[i];
		i++;
	}
	len = len + i;
	*((lin->phrase)+len) = ' ';
	len++;
	i=0;
	while (b[i] != '\0')
	{
		*((lin->phrase)+len+i) = b[i];
		i++;
	}
	len = len + i;
	*((lin->phrase)+len) = '\n';
	len++;
	lin->len = len;
	return lin;
}


int main(int argc, char *argv[]){

	int numarg = 3;
	int numfile = argc-numarg;
	int fd[numfile];
	int failures = numfile;
	struct line newlie;
	struct line *lieptr = &newlie;


	if (argc < 4)
	{
		printf("uses ./a [int number fract max] [int max name length] [string Filepath]\n");
		printf("you can put more than one file\n");
		return -1;
	}

	int fractmax = atoi(argv[1]);
	int lenmax = atoi(argv[2]);
	if (fractmax <= 0 || lenmax <= 0)
	{
		fprintf(stderr, "values of argument 1 and 2 have to be greater than 0\n");
		return -1;
	}

	for (int i = numarg; i < argc; i++)
	{
		fd[i-numarg] = open(argv[i], O_RDWR);
		if (fd[i-numarg] < 0)
		{
			fprintf(stderr, "unable to open file :%s\n", argv[i]);
			failures--;
		}
	}
	if (failures == 0)
	{
		fprintf(stderr, "unable to open any file\n");
		return -1;
	}

	char alinea[lenmax + 30];
	lieptr->phrase = alinea;
	for (int i = 0; i < numfile; i++)
	{
		int fractnum = (rand()%fractmax)+1;
		for (int j = 0; j < fractnum; j++)
		{
			newline(lieptr, lenmax);
			if (fd[i] > 0)
			{
				if (write(fd[i], (void*)(lieptr->phrase), sizeof(char)*(lieptr->len)) < lieptr->len)
				{
					fprintf(stderr, "error while printing on file :%s", argv[i+numarg]);
					i++;
				}
			}
		}
	}

	for (int i = 0; i < numfile; i++)
	{
		close(fd[i]);
	}
	return 0;
}
