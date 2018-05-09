#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>

struct line{
	char *phrase;
	int len;
};

int widthmax = 1280;
int heigthmax = 720;
int complexity = 0;
int armageddon = 0;
char *rndcom = "#Random comment\n";
char *rndempt = "              \n";

struct line *newline(struct line *lin, int lenmax)
{
	int len = 0;
	int i =0;

	if (complexity == 5)
	{
		len = (rand()%(30*60));
		int j = 0;
		for (int i = 0; i < len; i++)
		{
				*(lin->phrase + i) = (char)(rand()%128);
				j++;
		}
		*(lin->phrase + j) = '\n';
		lin->len = len+1;
		return lin;
	}

	if (complexity >= 2 && (rand()%(15 - complexity)) == 0)
	{
			strncpy(lin->phrase, rndcom, strlen(rndcom));
			lin->len = strlen(rndcom);
			return lin;
	}
	if (complexity >= 2 && (rand()%(15 - complexity)) == 0)
	{
			strncpy(lin->phrase, rndempt, strlen(rndempt));
			lin->len = strlen(rndempt);
			return lin;
	}

	// name

	if (complexity == 0)
	{
		int namelen = (rand()%(lenmax))+1;
		len = namelen;
		for (int i = 0; i < namelen; i++)
		{
			char tem = (char)((rand()%26)+97);
			*((lin->phrase)+i) = tem;
		}
	}
	else if (complexity == 1)
	{
		int namelen = (rand()%(lenmax)+1);
		len = namelen;
		for (int i = 0; i < namelen; i++)
		{
			char tem = (char)((rand()%26)+97);
			*((lin->phrase)+i) = tem;
		}
	}
	else if (complexity < 5)
	{
		int namelen = (rand()%100);
		len = namelen;
		for (int i = 0; i < namelen; i++)
		{
			char tem = (char)((rand()%26)+97);
			*((lin->phrase)+i) = tem;
		}
	}

	//width and heigth
	if (complexity == 0)
	{
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
	}
	else if (complexity == 1)
	{
		int width = (rand()%widthmax)+1;
		int heigth = (rand()%heigthmax)+1;
		char wid[5];
		char hei[5];
		sprintf(wid,"%d",width);
		sprintf(hei,"%d",heigth);
		if (rand()%5 == 0)
		{
			char spot = (char)((rand()%(128-32)+32));
			wid[rand()%5] = spot;
			spot = (char)((rand()%(128-32)+32));
			hei[rand()%5] = spot;
		}
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
	}
	else if (complexity < 5)
	{
		int width = (rand()%widthmax)+1;
		int heigth = (rand()%heigthmax)+1;
		char wid[5];
		char hei[5];
		sprintf(wid,"%d",width);
		sprintf(hei,"%d",heigth);
		if (rand()%(5 - complexity) == 0)
		{
			char spot = (char)(rand()%(128));
			wid[rand()%5] = spot;
			spot = (char)(rand()%(128));
			hei[rand()%5] = spot;
		}
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
	}

	//a and b
	if (complexity == 0)
	{
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
	}
	else if (complexity == 1)
	{
		char a[6];
		char b[6];
		double da = (((double)(rand() % 200))/100)-1;
		double db = (((double)(rand() % 200))/100)-1;
		sprintf(a,"%.2f",da);
		sprintf(b,"%.2f",db);
		if (rand()%(5) == 0)
		{
			char spot = (char)((rand()%(128-32)+32));
			a[rand()%6] = spot;
			spot = (char)((rand()%(128-32)+32));
			b[rand()%6] = spot;
		}

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
	}
	else if (complexity < 5)
	{
		char a[6];
		char b[6];
		double da = (((double)(rand() % 200))/100)-1;
		double db = (((double)(rand() % 200))/100)-1;
		sprintf(a,"%.2f",da);
		sprintf(b,"%.2f",db);
		if (rand()%(5- complexity) == 0)
		{
			char spot = (char)(rand()%(128));
			a[rand()%6] = spot;
			spot = (char)(rand()%(128));
			b[rand()%6] = spot;
		}

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
	}


	return lin;
}


int main(int argc, char *argv[]){

	srand(getpid()+time(NULL));

	int numarg = 4;
	int numfile = argc-numarg;
	int fd[numfile];
	int failures = numfile;
	struct line newlie;
	struct line *lieptr = &newlie;


	if (argc < numarg + 1)
	{
		printf("uses ./a [int level of complexity] [int number fract max] [int max name length] [string Filepath]...\n");
		printf("you can put more than one file\n");
		return -1;
	}

	complexity = atoi(argv[1]);
	int fractmax = atoi(argv[2]);
	int lenmax = atoi(argv[3]);
	if (fractmax <= 0 || lenmax <= 0 || complexity < 0 || complexity > 5)
	{
		fprintf(stderr, "values of arguments 2 and 3 have to be greater than 0 and arg 1 must be within [0,5]\n");
		return -1;
	}
	if (complexity == 5)
	{
		printf("you're crazy\n");

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

	char alinea[lenmax + 30*((complexity+1)*10)];
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
