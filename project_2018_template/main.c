#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "fractal.h"


struct parametres {
	int fd;
	struct fractal **Buffer;
	};



void *FractCreate (void *param)
{
	struct parametres *para = (struct parametres*)param;
	char *name = (char*)malloc(sizeof(char)*65);
	if (name == NULL)
	{
		fprintf(stderr, "critical malloc failure\n");
		close(para->fd);
		pthread_exit(NULL);
	}
	
	unsigned long width;
	unsigned long height;
	double a;
	double b;
	
	int test = 1;
	
	while (test == 1)
	{
		int i = 0;
		char ispace = '';
		while(ispace != ' ' && i < 66)
		{
			int end =read(fd, (void *)&ispace, sizeof(char));
			if(end == -1)
			{
				close(para->fd);
				free(name);
				fprintf(stderr, "Error while reading\n");
				pthread_exit(NULL);
			}
			else if (end == 0)
			{
				close(para->fd);
				free(name);
				printf("Process finished\n");
				pthread_exit(NULL);
			}
			
			if((ispace != '#' && i == 0) || (i == 0 && ispace == ' '))
			{
				i == 100;
			}
			else if(ispace != ' ')
			{
				*(name + i) = ispace;
			}
			else if(ispace == ' ')
				*(name + i) = '\0';
			
			i++;
		}
		
		if(i != 100)
		{
			if (read(fd, (void*)width, 4) < 4)
			{
				close(para->fd);
				free(name);
				fprintf(stderr, "Error while reading\n");
				pthread_exit(NULL);
			}
			lseek(fd, 1, SEEK_CUR);
			if (read(fd, (void*)height, 4) < 4)
			{
				close(para->fd);
				free(name);
				fprintf(stderr, "Error while reading\n");
				pthread_exit(NULL);
			}
			lseek(fd, 1, SEEK_CUR);
			if (read(fd, (void*)a, sizeof(double)) < sizeof(double))
			{
				close(para->fd);
				free(name);
				fprintf(stderr, "Error while reading\n");
				pthread_exit(NULL);
			}
			lseek(fd, 1, SEEK_CUR);
			if (read(fd, (void*)b, sizeof(double)) < sizeof(double))
			{
				close(para->fd);
				free(name);
				fprintf(stderr, "Error while reading\n");
				pthread_exit(NULL);
			}
		}
		
		while (ispace != '\n' && test == 1)
			test = read(fd, (void*)ispace, 1);
		
		
		
		
		//producer comsumer problem
	}
	
	
	
	free(name);
	close(fd);
	}


int argCmp(char *string1, char *string2)
{
	int i =0;
	while (*(string1 + i) != '\0' && *(string2 +i) != '\0')
	{
		if (*(string1 + i) != *(string2 + i))
			return 0;
		
		i++;
	}
	if(*(string1 + i) == *(string2 + i))
		return 1;
	
	return 0;
}


/* 
void RthreadCreation(int readingThreads, char *filename[], pthread_t threadlist[], struct fractal **Buffer)
{
	struct argCreator argum[readerThreads];
	for (int i = 0; i < readingThreads; i++)
	{
		(argum + i)->buffer = buffer;
		(argum + i)->filename = filename[i];
	}
	
	for (int i = 0; i < readingThreads; i++)
	{
		pthread_create(&(threadlist[i]), NULL, &FractCreate, (void *)(argum + i));
	}
}
 */
 
 
void fileopener(int filenumber, char ** filename, int *fd)
 {
	int control = 0;
	for (int i = 0; i < filnumber; i++)
	{
		if(!argCmp(filename[i], "-"))
		{
			fd[i] = open(filename[i], O_RDONLY);
			if (fd[i] == -1)
			{
				fprintf(stderr, "error, unable to open File : %s \n", filename[i]);
			}
		}
		else if (argCmp(filename[i], "-") && control == 0)
		{
			fd[i] = 0;
			control ++;
		}
		
	}
 }
 
 struct parametres *threadcreate(pthread_t *threads, int fd[], struct fractal **buffer, int number)
 {
	int test = 0;
	struct parametres *para = (struct parametres *)malloc(sizeof(struct parametres)*number);
	if (para == NULL)
	{
		fprintf(stderr, "malloc fail");
		return NULL;
	}
	
	
	for (int i = 0; i < number; i++)
	{
		if (fd[i] => 0)
		{
			(para + i)->buffer = buffer;
			(para + i)->fd = fd[i];
			test = pthread_create((threads+i), NULL, &FractCreate, (void*)(para+i))
			if (test != 0)
				fprintf(stderr, "thread creation failed, thread number : %i \n", i);
			
		}
	}
	return para;
 }
 
 

int main(int argc, char *argv[])
{
	const char *d = "-d";
	const char *maxthreads = "--maxthreads";
	struct parametres *para;
	struct fractal **Buffer1 = (struct fractal**)malloc(sizeof(struct fractal*)*argc*2);
	if (Buffer1 == NULL)
	{
		fprintf(stderr, "malloc fail");
		return -1;
	}
	
	
    if (argc => 3)
	{
		if (argCmp(argv[2], d) && argCmp(argv[3], maxthreads))     	//case when both options are used
		{
			int readingThreads = argc - 5;							//-5 because of [nameoffunc, -d, --maxthreads, N, fileout]
			pthread_t readerThreads[readingThreads];
			int fd[readingThreads];
			
			fileopener(readingThreads, argv[4], fd);
			para = threadcreate(readerThreads, fd, Buffer1, readingThreads);
			
			
		}
		else if (argCmp(argv[1], d))								//case when only -d option is used
		{
			int readingThreads = argc - 3;
			pthread_t readerThreads[readingThreads];
			int fd[readingThreads];
			
			fileopener(readingThreads, argv[2]);
			para = threadcreate(readerThreads, fd, Buffer1, readingThreads);
		}
		else if (argCmp(argv[2], maxthreads))						//case when only --maxthreads option is used
		{
			int readingThreads = argc - 4;
			pthread_t readerThreads[readingThreads];
			int fd[readingThreads];
			
			fileopener(readingThreads, argv[3]);
			para = threadcreate(readerThreads, fd, Buffer1, readingThreads);
		}
		else														//case when none of the options are used
		{
			int readingThreads = argc - 2;
			pthread_t readerThreads[readingThreads];
			int fd[readingThreads];
			
			fileopener(readingThreads, argv[1]);
			para = threadcreate(readerThreads, fd, Buffer1, readingThreads);
		}
	}
	else
	{
		free(Buffer1);
		printf("there are missing arguments, hey bro mind stopping putting strain on my code ?\n")
		return -1;
	}
	
	
	free(para);
    return 0;
}
