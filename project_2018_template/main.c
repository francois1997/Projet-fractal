#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "fractal.h"



/* it is important to implement a rendez vous semaphore in main for it to wait for the threads to finish before 
 * finishing itself
 *
 *
 *
 *
*/


pthread_mutex_t buffycreate;
sem_t full1;
sem_t empty1;



struct parametres {
	int fd;
	struct fractal **Buffer;
	};
	
	
	
struct porometres {
	struct fractal** buffer1;
	struct fractal** buffer2;
}



void *FractCreate (void *param)
{
	struct parametres *para = (struct parametres*)param;
	int fd = para->fd;
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
	
	/*  This section is about reading the files containing the frctal structure
	 *	The first part is about getting the name of the fractal
	 *	the second section is about the other data like width height a and b
	 *	the third section is about putting the pointer of the fractal in the buffer
	 *	
	*/
	
	while (test == 1)
	{
		/* this section primarily works on the name of the fractal,  
		 * first it check if we are not a the end of the file or that an error occurs
		 * then it check if it is not an empty line or a comment
		 * it then store each character until the first space character
		 * 
		 *
		*/
		
		
		
		int i = 0;
		char ispace = '';
		while(ispace != ' ' && i < 66)
		{
			int end =read(fd, (void *)(&ispace), sizeof(char));
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
			if(i == 66)
			{
				fprintf(stderr, "Name is too long \n")
				i = 100;
			}
		}
		
		/* this condition here is to skip empty line and comment
		 * when we enter the conditions we get the static information of the fractal
		 * The reading is a dangerous process, if a single line doesn't respect the syntax, the threads terminate
		 * and the rest of the file is skipped
		*/
		
		if(i != 100)
		{
			if (read(fd, (void*)(&width), 4) < 4)
			{
				close(para->fd);
				free(name);
				fprintf(stderr, "Error while reading\n");
				pthread_exit(NULL);
			}
			lseek(fd, 1, SEEK_CUR);
			if (read(fd, (void*)(&height), 4) < 4)
			{
				close(para->fd);
				free(name);
				fprintf(stderr, "Error while reading\n");
				pthread_exit(NULL);
			}
			lseek(fd, 1, SEEK_CUR);
			if (read(fd, (void*)(&a), sizeof(double)) < sizeof(double))
			{
				close(para->fd);
				free(name);
				fprintf(stderr, "Error while reading\n");
				pthread_exit(NULL);
			}
			lseek(fd, 1, SEEK_CUR);
			if (read(fd, (void*)(&b), sizeof(double)) < sizeof(double))
			{
				close(para->fd);
				free(name);
				fprintf(stderr, "Error while reading\n");
				pthread_exit(NULL);
			}
		}
		
		/* This loop serve to go to the next line, only works with file who use LF to go to next the line
		 * It has a second purpose and it is to get out of the greater loop when we are a the end of the file
		 * After that it put fractal which he manage to make inside the buffer
		*/
		
		while (ispace != '\n' && test == 1)
			test = read(fd, (void*)(&ispace), 1);
		
		struct fractal *fracty = fractal_new(name, width, height,  a,  b);
		if (fracty != NULL)
		{
			sem_wait(&empty1);
			pthread_mutex_lock(&buffycreate);
			*((para->buffer) + sem_getvalue(&full1, 0)) = fracty;
			pthread_mutex_unlock(&buffycreate);
			sem_post(&full1);
			//producer comsumer problem
		}
		else if (fracty == NULL)
			fprintf(stderr, "Malloc Error on fractal : %s\n", name);
		
		
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
 
 /* this function try to open all the file that have been passed in the arguments
  * it checks that it is not the - option and then try to open the file, in case of failure
  * it display an error on stderr but still continue to open the others files, when - is detected
  * the stdin is open for the users to input and control is set to 1 to avoid trying to open more than once the stdin
  * it is important to note that all the other - after the first will be ignored
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
		fprintf(stderr, "critical malloc fail");
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
		fprintf(stderr, "malloc fail : Buffer1");
		return -1;
	}
	int sem_init(&empty1, 0, argc*2);
	int sem_init(&full1, 0, 0);
	struct fractal **Buffer2 = (struct fractal**)malloc(sizeof(struct fractal*)*argc*2);
	if (Buffer2 == NULL)
	{
		free(Buffer1);
		fprintf(stderr, "malloc fail : Buffer2");
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
	if (para == NULL)
	{
		free(buffer1);
		for (int i = 0; i < readingThreads; i++)
			close(fd[i]);
		
		exit();
	}
	
	
	
	
	free(buffer1);
	free(para);
    return 0;
}
