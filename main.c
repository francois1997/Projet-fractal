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
 * finishing itself Done FIXED
 * a new thread is to be created to avoid the deadlock on the calculus FIXED
 * if by a test of someone the calculus threads failed to be created another deadlock can appear FIXED
 * when the main enter the while where it checks the threads and the buffer
 * also i should implement a failsafe for when neither the calculus threads or reading threads are created FIXED
 *
*/


pthread_mutex_t buffycreate;
sem_t full1;
sem_t empty1;

pthread_mutex_t buffycalculus;
sem_t full2;
sem_t empty2;

sem_t rdv1;
sem_t rdv2;
sem_t rdv3;

int fully1 = 0;
int fully2 = 0;


struct parametres {
	int fd;
	struct fractal **Buffer;
	};



struct porometres {
	struct fractal** buffer1;
	struct fractal** buffer2;
};



void *FractCreate (void *param){
	//check that calculating threads have been created before launching anything
	// *CRITICAL* change the reading with the mmap function and fstat

	int test2 = 0;
	int test3 = -1;


	while (test2 == 0 || test3 == -1)
	{
		test3 = sem_getvalue(&rdv2, &test2);
		sleep(1);
	}

	//start execution
  printf("Reading thread starting\n");
	struct parametres *para = (struct parametres*)param;
	int fd = para->fd;
	char *name = (char*)malloc(sizeof(char)*65);
	if (name == NULL)
	{
		fprintf(stderr, "critical malloc failure\n");
		close(para->fd);
		sem_wait(&rdv1);
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
		char ispace = 0;
		while(ispace != ' ' && i < 65)
		{
			int end =read(fd, (void *)(&ispace), sizeof(char));
			if(end == -1)
			{
				close(para->fd);
				free(name);
				fprintf(stderr, "Error while reading\n");
				sem_wait(&rdv1);
				pthread_exit(NULL);
			}
			else if (end == 0)
			{
				close(para->fd);
				free(name);
				printf("Process finished\n");
				sem_wait(&rdv1);
				pthread_exit(NULL);
			}

			//it checks wether it is a comment or an empty line

			if ((ispace == '#' && i == 0) || (i == 0 && ispace == ' '))
			{
				i == 100;
			}
			else if(ispace != ' ')
			{
				*(name + i) = ispace;
			}
			else if(ispace == ' ' && i < 64)
				*(name + i) = '\0';

			if(i == 64)
			{
				fprintf(stderr, "Name is too long \n");
				i = 100;
			}
			if (i != 100)
				i++;

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
				sem_wait(&rdv1);
				pthread_exit(NULL);
			}
			lseek(fd, 1, SEEK_CUR);
			if (read(fd, (void*)(&height), 4) < 4)
			{
				close(para->fd);
				free(name);
				fprintf(stderr, "Error while reading\n");
				sem_wait(&rdv1);
				pthread_exit(NULL);
			}
			lseek(fd, 1, SEEK_CUR);
			if (read(fd, (void*)(&a), sizeof(double)) < sizeof(double))
			{
				close(para->fd);
				free(name);
				fprintf(stderr, "Error while reading\n");
				sem_wait(&rdv1);
				pthread_exit(NULL);
			}
			lseek(fd, 1, SEEK_CUR);
			if (read(fd, (void*)(&b), sizeof(double)) < sizeof(double))
			{
				close(para->fd);
				free(name);
				fprintf(stderr, "Error while reading\n");
				sem_wait(&rdv1);
				pthread_exit(NULL);
			}
		}

		/* This loop serve to go to the next line, only works with file who use LF to go to next the line
		 * It has a second purpose and it is to get out of the greater loop when we are a the end of the file
		 * After that it put fractal which he manage to make inside the buffer
		*/

		while (ispace != '\n' && test == 1)
			test = read(fd, (void*)(&ispace), 1);

		if (i != 100)
		{
			//i need to make a linked list with the name of the fractal, and be able to compare to avoid multiple
			struct fractal *fracty = fractal_new(name, width, height,  a,  b);
			if (fracty != NULL)
			{
				sem_wait(&empty1);
				pthread_mutex_lock(&buffycreate);
				*((para->Buffer) + fully1) = fracty;
				fully1++;
				fracty = NULL;
				pthread_mutex_unlock(&buffycreate);
				sem_post(&full1);
				//producer comsumer problem
			}
			else if (fracty == NULL)
				fprintf(stderr, "Malloc Error on fractal : %s\n", name);

		}
	}



	free(name);
	close(fd);
	sem_wait(&rdv1);
	pthread_exit(NULL);
	}



	/*Haute possibilité de deadlock dans fractCalculus FIXED*/

void *FractCalculus (void *param){

	struct porometres *para = (struct porometres*)param;
	struct fractal *fractalis;
	/*
	 * A critical error is occuring sem_getvalue migth not be the last element
	 * A mutex is nescessary to avoid the semfull to change
	 * the solution chosen is global variable
	*/
	int test1 = 0;
	int test2 = 0;
	sem_getvalue(&rdv1, &test1);
	sem_getvalue(&full1, &test2);
	while (test1 != 0 || test2 != 0)
	{
		if (sem_trywait(&full1) == 0)
		{

			pthread_mutex_lock(&buffycreate);
			fractalis = *((para->buffer1) + fully1 - 1);
			*((para->buffer1) + buffpos1 - 1) = NULL;
			fully1--;
			pthread_mutex_unlock(&buffycreate);
			sem_post(&empty1);

			unsigned long height = fractal_get_height(fractalis);
			unsigned long width = fractal_get_width(fractalis);
			for (int i = 0; i < height; i++)
			{
				for(int j = 0; j < width; j++)
				{
					int k =fractal_compute_value(fractalis, j, i);
					fractalis->moyenne = (fractalis->moyenne) + k;
					fractal_set_value(fractalis, j, i, k);
				}
			}
			fractalis->moyenne = (fractalis->moyenne)/(height * width);


			sem_wait(&empty2);
			pthread_mutex_lock(&buffycalculus);
			*((para->buffer2) + fully2) = fractalis;
			fully2++;
			pthread_mutex_unlock(&buffycalculus);
			sem_post(&full2);
		}
		sem_getvalue(&rdv1, &test1);
		sem_getvalue(&full1, &test2);
	}


	sem_wait(&rdv2);
	pthread_exit(NULL);
}


void *BitCreator (void *param){

	struct fractal **buffer = (struct fractal**)param;
	struct fractal *fractalis;
	int test1 = 0;
	int test2 = 0;
	sem_getvalue(&rdv2, &test2);
	sem_getvalue(&full2, &test1);

	while (test1 != 0 || test2 != 0)
	{
		if (sem_trywait(&full2))
		{
			pthread_mutex_lock(&buffycalculus);
			fractalis = *(buffer + fully2 - 1);
			*(buffer + buffpos2 - 1) == NULL;
			fully2--;
			pthread_mutex_unlock(&buffycalculus);
			sem_post(&empty2);


			if (write_bitmap_sdl((const struct fractal*)fractalis, (const char *)(fractalis->name)) == -1)
			{
				fprintf(stderr, "seems like the fractal : %s didn't create itself \n", fractalis->name);
			}
			fractal_free(fractalis);
			fractalis = NULL;
		}

		sem_getvalue(&rdv2, &test2);
		sem_getvalue(&full2, &test1);
	}

	sem_wait(&rdv3);
	pthread_exit(NULL);
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


void fileopener(int filenumber, char ** filename, int *fd, int offset){

  printf("%d file(s) will be opened\n", filenumber);
	int control = 0;
	for (int i = 0; i < filenumber; i++)
	{
		if(!argCmp(filename[i + offset], "-"))
		{
			fd[i] = open(filename[i + offset], O_RDONLY);
			printf("fd : %d\n", fd[i]);
			if (fd[i] == -1)
			{
				fprintf(stderr, "error, unable to open File : %s \n", filename[i + offset]);
			}
		}
		else if (argCmp(filename[i + offset], "-") && control == 0)
		{
			fd[i] = 0;
			control ++;
		}

	}
 }

 struct parametres *threadcreate(pthread_t *threads, int fd[], struct fractal **buffer, int number){

	int rendevous = 0;
	int test = 0;
	struct parametres *para = (struct parametres *)malloc(sizeof(struct parametres)*number);
	if (para == NULL)
	{
		fprintf(stderr, "critical malloc fail\n");
		return NULL;
	}


	for (int i = 0; i < number; i++)
	{
		if (fd[i] >= 0)
		{
			(para + i)->Buffer = buffer;
			(para + i)->fd = fd[i];
			test = pthread_create((threads+i), NULL, &FractCreate, (void*)(para+i));
			rendevous++;
			if (test != 0)
			{
				fprintf(stderr, "reading thread creation failed, thread number : %i \n", i);
				rendevous--;
			}
		}
	}
	sem_init(&rdv1,0,rendevous);
	if (rendevous == 0)
	{
		fprintf(stderr, "No reading threads created\n");
		return NULL;
  }

	printf("%d reading(s) Thread(s) created\n", rendevous);
	return para;
 }




 int calculusPublisher(int printNumber, struct porometres *editeurImprimeur, pthread_t *threads){

	int rendevous = 0;
	int test = 0;
	for (int i = 0; i < printNumber; i++)
	{
		test = pthread_create((threads + i), NULL, &FractCalculus, (void*)editeurImprimeur);
		rendevous++;
		if (test != 0)
		{
			rendevous--;
			fprintf(stderr, "calculus thread creation failed, thread number : %i \n", i);
		}
	}
	sem_init(&rdv2, 0, rendevous);
	if (rendevous == 0)
		fprintf(stderr, "no calculating threads created\n");

	printf("%d calculating thread(s) created\n", rendevous);
	return rendevous;
 }



 int bitThreadsCreate(int ThreadNumber, struct fractal **buffer, pthread_t *threads){

	int rendevous = 0;
	int test = 0;
	for (int i =0; i < ThreadNumber; i++)
	{
		test = pthread_create((threads+i), NULL, &BitCreator , (void*)buffer);
		rendevous++;
		if (test != 0)
		{
			rendevous--;
			fprintf(stderr, "Bitmap creation thread failed initialisation\n");
		}
	}
	sem_init(&rdv3, 0, rendevous);
	return rendevous;

 }


int main(int argc, char *argv[]){

	// initialisation of all the variables needed

	const char *d = "-d";
	const char *maxthreads = "--maxthreads";
	struct fractal *fractalis = NULL;
	struct fractal *compa;
	struct parametres *para;
	int test1;
	int test2;
	sem_init(&empty1, 0, argc*2);
	sem_init(&full1, 0, 0);
	sem_init(&full2, 0, 0);
	sem_init(&empty2, 0, argc*2);
	struct fractal **Buffer1 = (struct fractal**)malloc(sizeof(struct fractal*) * argc * 2);
	if (Buffer1 == NULL)
	{
		sem_destroy(&empty1);
		sem_destroy(&empty2);
		sem_destroy(&full1);
		sem_destroy(&full2);
		fprintf(stderr, "malloc fail : Buffer1\n");
		return -1;
	}
	struct fractal **Buffer2 = (struct fractal**)malloc(sizeof(struct fractal*) * argc * 2);
	if (Buffer2 == NULL)
	{
		free(Buffer1);
		sem_destroy(&empty1);
		sem_destroy(&empty2);
		sem_destroy(&full1);
		sem_destroy(&full2);
		fprintf(stderr, "malloc fail : Buffer2\n");
		return -1;
	}
	struct porometres *lachouf = (struct porometres*)malloc(sizeof(struct porometres));
	if (lachouf == NULL)
	{
		free(Buffer1);
		free(Buffer2);
		sem_destroy(&empty1);
		sem_destroy(&empty2);
		sem_destroy(&full1);
		sem_destroy(&full2);
		fprintf(stderr, "malloc fail : lachouf\n");
		return -1;
	}
	lachouf->buffer1 = Buffer1;
	lachouf->buffer2 = Buffer2;


	//checking the number of arguments, a input file and output file is always needed


    if (argc >= 3)
	{
		if (argCmp(argv[1], d) && argCmp(argv[2], maxthreads))     	//case when both options are used
		{
			//checking how many threads should be created for reading and calculating

			int readingThreads = argc - 5;							//-5 because of [nameoffunc, -d, --maxthreads, N, fileout]
			pthread_t readerThreads[readingThreads];
			pthread_t calculusThreads[(int)argv[3]];
			pthread_t BitThreads[(int)argv[3]];
			int fd[readingThreads];

			//opening the streams for the files

			fileopener(readingThreads, argv, fd, 4);      			// add an offset

			//creating the threads for reading

			para = threadcreate(readerThreads, fd, Buffer1, readingThreads);
			if (para == NULL)
			{
				free(Buffer1);
				free(Buffer2);
				free(lachouf);
				sem_destroy(&empty1);
				sem_destroy(&empty2);
				sem_destroy(&full1);
				sem_destroy(&full2);
				sem_destroy(&rdv1);
				for (int i = 0; i < readingThreads; i++)
					close(fd[i]);

				exit(-1);
			}

			//creating the threads for calculating

			int deadlockDect = calculusPublisher((int)argv[3], lachouf, calculusThreads);
			if (deadlockDect == 0)
			{
				free(Buffer1);
				free(Buffer2);
				free(lachouf);
				sem_destroy(&empty1);
				sem_destroy(&empty2);
				sem_destroy(&full1);
				sem_destroy(&full2);
				sem_destroy(&rdv1);
				sem_destroy(&rdv2);
				for (int i = 0; i < readingThreads; i++)
					close(fd[i]);

				fprintf(stderr, "it didn't like that at all\n");
				exit(-1);
			}

			// creating threads to create each image

			int bitThreads = bitThreadsCreate((int)argv[3], Buffer2, BitThreads);
			if (bitThreads == 0)
			{
				fprintf(stderr, "welp it's gonna be slow\n");
				int test11 = 0;
				int test12 = 0;
				sem_getvalue(&rdv2, &test11);
				sem_getvalue(&full2, &test12);

				while (test11 != 0 || test12 != 0)
				{
					if (sem_trywait(&full2) == 0)
					{
						pthread_mutex_lock(&buffycalculus);
						fractalis = *(Buffer2 + fully2 - 1);
						*(Buffer2 + buffpos2 - 1) == NULL;
						fully2--;
						pthread_mutex_unlock(&buffycalculus);
						sem_post(&empty2);


						if (write_bitmap_sdl((const struct fractal*)fractalis, (const char *)(fractalis->name)) == -1)
						{
							fprintf(stderr, "seems like the fractal : %s didn't create itself \n", fractalis->name);
						}
						fractal_free(fractalis);
						fractalis = NULL;
					}

					sem_getvalue(&rdv2, &test11);
					sem_getvalue(&full2, &test12);
				}


			}

			int rendezvous3 = 0;
			sem_getvalue(&rdv3, &rendezvous3);
			while (rendezvous3 != 0)
			{
				sleep(5);
				sem_getvalue(&rdv3, &rendezvous3);
			}


		}
		else if (argCmp(argv[1], d))								//case when only -d option is used
		{
			//checking how many threads should be created for reading and calculating

			int readingThreads = argc - 3;
			pthread_t readerThreads[readingThreads];
			pthread_t calculusThreads[argc*4];
			pthread_t BitThreads[argc*3];
			int fd[readingThreads];

			//opening the streams for the files

			fileopener(readingThreads, argv, fd, 2);

			//creating the threads for reading

			para = threadcreate(readerThreads, fd, Buffer1, readingThreads);
			if (para == NULL)
			{
				free(Buffer1);
				free(Buffer2);
				free(lachouf);
				sem_destroy(&empty1);
				sem_destroy(&empty2);
				sem_destroy(&full1);
				sem_destroy(&full2);
				sem_destroy(&rdv1);
				for (int i = 0; i < readingThreads; i++)
					close(fd[i]);

				exit(-1);
			}

			//creation of calculating threads

			int deadlockDect = calculusPublisher(argc*4, lachouf, calculusThreads);
			if (deadlockDect == 0)
			{
				free(Buffer1);
				free(Buffer2);
				free(lachouf);
				sem_destroy(&empty1);
				sem_destroy(&empty2);
				sem_destroy(&full1);
				sem_destroy(&full2);
				sem_destroy(&rdv1);
				sem_destroy(&rdv2);
				for (int i = 0; i < readingThreads; i++)
					close(fd[i]);

				fprintf(stderr, "it didn't like that at all\n");
				exit(-1);
			}

			//creation of bitmap creation threads as well as a failsafe in case we are unable to create them

			int bitThreads = bitThreadsCreate(argc*3, Buffer2, BitThreads);
			if (bitThreads == 0)
			{
				fprintf(stderr, "welp it's gonna be slow\n");
				int test11 = 0;
				int test12 = 0;
				sem_getvalue(&rdv2, &test11);
				sem_getvalue(&full2, &test12);

				while (test11 != 0 || test12 != 0)
				{
					if (sem_trywait(&full2) == 0)
					{
						pthread_mutex_lock(&buffycalculus);
						fractalis = *(Buffer2 + fully2 - 1);
						*(Buffer2 + buffpos2 - 1) == NULL;
						fully2--;
						pthread_mutex_unlock(&buffycalculus);
						sem_post(&empty2);


						if (write_bitmap_sdl((const struct fractal*)fractalis, (const char *)(fractalis->name)) == -1)
						{
							fprintf(stderr, "seems like the fractal : %s didn't create itself \n", fractalis->name);
						}
						fractal_free(fractalis);
						fractalis = NULL;
					}

					sem_getvalue(&rdv2, &test11);
					sem_getvalue(&full2, &test12);
				}


			}

			int rendezvous3 = 0;
			sem_getvalue(&rdv3, &rendezvous3);
			while (rendezvous3 != 0)
			{
				sleep(5);
				sem_getvalue(&rdv3, &rendezvous3);
			}

		}
		else if (argCmp(argv[1], maxthreads))						//case when only --maxthreads option is used
		{
			//checking how many threads should be created for reading and calculating

			int readingThreads = argc - 4;
			pthread_t readerThreads[readingThreads];
			pthread_t calculusThreads[(int)argv[2]];
			int fd[readingThreads];

			//opening the streams for the files

			fileopener(readingThreads, argv, fd, 3);

			//creating the threads for reading

			para = threadcreate(readerThreads, fd, Buffer1, readingThreads);
			if (para == NULL)
			{
				free(Buffer1);
				free(Buffer2);
				free(lachouf);
				sem_destroy(&empty1);
				sem_destroy(&empty2);
				sem_destroy(&full1);
				sem_destroy(&full2);
				sem_destroy(&rdv1);
				for (int i = 0; i < readingThreads; i++)
					close(fd[i]);

				exit(-1);
			}

			//creation of calculating threads

			int deadlockDect = calculusPublisher((int)argv[3], lachouf, calculusThreads);
			if (deadlockDect == 0)
			{
				free(Buffer1);
				free(Buffer2);
				free(lachouf);
				sem_destroy(&empty1);
				sem_destroy(&empty2);
				sem_destroy(&full1);
				sem_destroy(&full2);
				sem_destroy(&rdv1);
				sem_destroy(&rdv2);
				for (int i = 0; i < readingThreads; i++)
					close(fd[i]);

				fprintf(stderr, "it didn't like that at all\n");
				exit(-1);
			}

			//comparaison between fractal and freeing of the memory occupied
			int test1 = 0;
			int test2 = 0;
			sem_getvalue(&full2, &test2);
			sem_getvalue(&rdv2, &test1);
			while(test1 != 0 || test2 != 0)
			{
				if (sem_trywait(&full2) == 0)
				{
					pthread_mutex_lock(&buffycalculus);
					compa = *(Buffer2 + fully2 - 1);
					*(Buffer2 + fully2 - 1) = NULL;
					fully2--;
					pthread_mutex_unlock(&buffycalculus);
					sem_post(&empty2);

					if (fractalis != NULL)
					{
						if (compa->moyenne > fractalis->moyenne)
						{
							fractal_free(fractalis);
							fractalis = compa;
						}
						else
							fractal_free(compa);

					}
					else
						fractalis = compa;

				}
				sem_getvalue(&full2, &test2);
				sem_getvalue(&rdv2, &test1);
			}

			//creating the bitmap image

			if(write_bitmap_sdl(((const struct fractal*)fractalis), (const char*)(fractalis->name)) == -1)
				fprintf(stderr, "conversion failure\n");

		}
		else														//case when none of the options are used
		{

			//checking how many threads should be created for reading and calculating

			int readingThreads = argc - 2;
			pthread_t readerThreads[readingThreads];
			pthread_t calculusThreads[argc*4];
			int fd[readingThreads];

			//opening the streams for the files

			fileopener(readingThreads, argv, fd, 1);

			//creating the threads for reading

			para = threadcreate(readerThreads, fd, Buffer1, readingThreads);
			if (para == NULL)
			{
				printf("para is NULL\n");
				free(Buffer1);
				free(Buffer2);
				free(lachouf);
				sem_destroy(&empty1);
				sem_destroy(&empty2);
				sem_destroy(&full1);
				sem_destroy(&full2);
				sem_destroy(&rdv1);
				for (int i = 0; i < readingThreads; i++)
					close(fd[i]);

				exit(-1);
			}

			//creation of calculating threads

			int deadlockDect = calculusPublisher(argc*4, lachouf, calculusThreads);
			if (deadlockDect == 0)
			{
				free(Buffer1);
				free(Buffer2);
				free(lachouf);
				sem_destroy(&empty1);
				sem_destroy(&empty2);
				sem_destroy(&full1);
				sem_destroy(&full2);
				sem_destroy(&rdv1);
				sem_destroy(&rdv2);
				for (int i = 0; i < readingThreads; i++)
					close(fd[i]);

				fprintf(stderr, "it didn't like that at all\n");
				exit(-1);
			}

			//comparaison between fractal and freeing of the memory occupied
			int test1 = 0;
			int test2 = 0;
			sem_getvalue(&full2, &test2);
			sem_getvalue(&rdv2, &test1);
			while(test1 != 0 || test2 != 0)
			{
				if (sem_trywait(&full2) == 0)
				{
					pthread_mutex_lock(&buffycalculus);
					compa = *(Buffer2 + fully2 - 1);
					*(Buffer2 + fully2 - 1) = NULL;
					fully2--;
					pthread_mutex_unlock(&buffycalculus);
					sem_post(&empty2);

					if (fractalis != NULL)
					{
						if (compa->moyenne > fractalis->moyenne)
						{
							fractal_free(fractalis);
							fractalis = compa;
						}
						else
						{
							fractal_free(compa);
							compa == NULL;
						}
					}
					else
						fractalis = compa;


				}
				sem_getvalue(&full2, &test2);
				sem_getvalue(&rdv2, &test1);
			}

			//Bitmap creation

			if(write_bitmap_sdl(((const struct fractal*)fractalis), (const char*)(fractalis->name)) == -1)
				fprintf(stderr, "conversion failure\n");
		}
	}
	else
	{
		free(Buffer1);
		free(Buffer2);
		free(lachouf);
		sem_destroy(&empty1);
		sem_destroy(&empty2);
		sem_destroy(&full1);
		sem_destroy(&full2);
		printf("there are missing arguments, hey bro mind stopping putting strain on my code ?\n");
		return -1;
	}




	fractal_free(fractalis);
	free(lachouf);
	free(Buffer2);
	free(Buffer1);
	free(para);
	sem_destroy(&empty1);
	sem_destroy(&empty2);
	sem_destroy(&full1);
	sem_destroy(&full2);
	sem_destroy(&rdv1);
	sem_destroy(&rdv2);
	sem_destroy(&rdv3);
    return 0;
}