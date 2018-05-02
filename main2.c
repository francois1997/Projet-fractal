#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "libfractal/fractal.h"
#include <pthread.h>
#include <errno.h>
#include <semaphore.h>
#include <stdbool.h>
#include <limits.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <SDL/SDL.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/resource.h>

#define SIZE_MAX 16777216

///////////////////////////////////////////////////////////////////////
/* Creation des structures necessaires au fonctionnement du programme*/
///////////////////////////////////////////////////////////////////////

// Cette structure permet de gerer le partage de memoire entre les producteurs-consommateurs
struct buff{
    struct fractal **buf;          /* Buffer contenant toutes les fractals */
    int length;             /* Nombre de slots dans le buffer */
    int begin;         /* buf[(begin+1)%n] est le premier element */
    int end;          /* buf[end%n] est le dernier */
    pthread_mutex_t mutex; //empecher a deux producteurs ou consommateurs d'acceder au buffer
    sem_t empty;       /* Nombre de places libres */
    sem_t full;       /* Nombre d'items dans le buffer */
};

//Cette structure permet de stocker le nom des fractals lue dans les fichiers
struct name{
  char* name;
  struct name *next;
};

//Cette structure permet l'acces au debut de la liste chainee des noms des fractals
struct nameacceslist{
  struct name *head; //premier noeud
  sem_t acces;
};

//Structure contenant la tete de la liste chainee des threads
struct listthread{
  struct thread *head;
  int numberthread;
};

//Liste chainee contenant les threads cree
struct thread{
  pthread_t *thread;
  struct thread *next;
};

//Structure permettant de stocker les informations concernant la fractal de plus grande moyenne
struct fractalHigh{
  int average;
  struct fractal *high;
  sem_t acces;
};

//Structure qui stocke le nombre de thread lisant les fichiers qui sont encore
// en cours d'execution
struct numberlecteur{
  int number;
  sem_t acces;
};

//Cette structure permet de stopper certaines categories de thread
//Il y a :  - endoflecture qui specifie au producteur si les threads de lectures sont termines
//          - endofproducteur qui specifie au consommateur si les threads de production sont termines
//          - end qui specifie a tous les threads si il y a eu une erreur et leur indique donc
//          qu'ils doivent se terminer
struct programend{
  int value;
  sem_t acces;
};

/*	La structure fileinfo permet de stocker les données d'un fichier dans la mémoire, toute les informations nescessaire sont présente :
 *	*msg 		: pointeur qui pointent toujour vers le debut de la zone memoire
 *	*readhead 	: pointeur qui est uttilisé pour la lecture dans la zone memoire
 *	readsize 	: long qui decrit combien de charactere reste apres readhead, il sert a éviter les segfault
 *	memload 	: long qui decrit quelle taille la memoire prend, est utile pour munmap
 *	offset 		: long qui decrit ou est la fin de la zone memoire par rapport aux debut du fichier
 *	sizefile 	: long qui decrit la taille du fichier
 *	fd 			: file descriptor du fichier
 *	finished 	: int qui decrit si la zone memoire reouvre la fin du fichier ou pas
 */
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


//Declaration de toutes les fonctions
void clean_all();
int create_all(int etat);
void printallname(struct nameacceslist *list);
int readfile(int argc, char *argv[], int begin, int type);
void * lecture(void* parametre);
struct fractal * split(char* line);
int verifyduplicatename(char* name, struct nameacceslist *list);
int addtolistname(char* name, struct nameacceslist *list);
int removetolistname(const char* name, struct nameacceslist *list);
void freelistname(struct nameacceslist *list);;
int buf_init(struct buff *buf, int n);
void buf_clean(struct buff *buffer);
void buf_insert(struct buff *buffer, struct fractal *item);
int buf_isempty(struct buff *buffer,struct fractal **f);
void setendofprogram(struct programend *f);
int isendofprogram(struct programend *f);
int verify_end(struct buff *buffer,struct fractal **f);
int verify_endproducteur(struct buff *buffer,struct fractal **f);
int fractalhighmodify(struct fractalHigh *f, struct fractal *frac, int average);
struct fractal *getfractalhigh(struct fractalHigh *f);
struct fractal* buf_remove(struct buff *buffer);
void listthread_free(struct listthread *list);
pthread_t *removethread(struct listthread *list);
int insertthread(struct listthread *list,void* funct);
int thread_moyenne();
int thread_all();
void *producer(void *parametre);
void *producermoyenne(void *parametre);
void *consumer(void *parametre);
int refresh(struct fileinfo *file);
int firstlf(struct fileinfo *file);
int read2(struct fileinfo *file, char* biffer, int lenbiffer);


//Les variables sont declarees en variables global et sont mise a NULL
int max_thread;
struct buff *buffer = NULL;
struct buff *listfractal = NULL;
struct nameacceslist *accesname = NULL;
struct numberlecteur *otherfile = NULL;
struct numberlecteur *otherproducteur = NULL;
struct programend *end = NULL;
struct programend *endoflecture = NULL;
struct programend *endofproducteur = NULL;
struct fractalHigh *high = NULL;
struct listthread *producerthread = NULL;
struct listthread *consumerthread = NULL;
pthread_mutex_t verification;


/*
 * @pre
 * @post
 */
int main(int argc, char *argv[])
{

  // pour faire planter le programme
/*
  struct rlimit old, new;
  struct rlimit *newp;
  new.rlim_cur = 536870912;
  new.rlim_max = 1073741824;
  newp = &new;
  prlimit(0, RLIMIT_AS, newp, &old);
  printf("maybe\n");
*/


    int err;
    if(argc <=1) //Verification qu'il y ai au moins 2 arguments et donc un fichier au minimum a lire
    {
          //error(err,"not enouth arguments");
          return -1;
    }
    if(strcmp(argv[1],"-d")==0) //Presence du parametre '-d' dans les arguments
    {
        if(strcmp(argv[2],"--maxthreads")==0) //Presence du parametre '--maxthreads' dans les arguments
        {
            max_thread = atoi(argv[3]);
            if(max_thread < 1) //Verification de la validite du nombre de thread maximal
            {
                printf("bad max_thread number");
                return -1;
            }
            create_all(1); //Initialisation de toutes les variables utilent au programme
            err = readfile(argc,argv,4,1);
            if(err == -1)
            {
                printf("error during read file");
                clean_all(); //free de toutes les variables cree par malloc
                return -1;
            }
            clean_all(); //free de toutes les variables cree par malloc
            return 0;
        }
        else //Non presence du parametre '--maxthreads' dans les arguments
        {
            max_thread = -1; //Si pas de nombre maximum de thread
            create_all(1); //Initialisation de toutes les variables utilent au programme
            err = readfile(argc,argv,2,1);
            if(err == -1)
            {
                printf("error during read file");
                clean_all(); //free de toutes les variables cree par malloc
                return -1;
            }
            clean_all(); //free de toutes les variables cree par malloc
            return 0;
        }
    }
    else //Non presence du parametre '-d' dans les arguments
    {
        if(strcmp(argv[1],"--maxthreads")==0) //Presence du parametre '--maxthreads' dans les arguments
        {
            max_thread = atoi(argv[2]);
            if(max_thread < 1) //Verification de la validite du nombre de thread maximal
            {
                printf("bad max thread number");
                //error(err,"bad max_thread number");
                return -1;
            }
            create_all(2); //Initialisation de toutes les variables utilent au programme
            err = readfile(argc,argv,3,2);
            if(err == -1)
            {
                printf("error during read file");
                clean_all(); //free de toutes les variables cree par malloc
                return -1;
            }
            clean_all(); //free de toutes les variables cree par malloc
            return 0;
        }
        else //Non presence du parametre '--maxthreads' dans les arguments
        {
            max_thread = -1;
            create_all(2) != 0;  //Initialisation de toutes les variables utilent au programme
            err = readfile(argc,argv,1,2);
            if(err == -1)
            {
                printf("error during read file");
                clean_all(); //free de toutes les variables cree par malloc
                return -1;
            }
            clean_all(); //free de toutes les variables cree par malloc
           return 0;
        }
    }
    return -1;
}

/*
 * @pre
 * @post
 */
void clean_all()
{
  printf("Function clean all variables \n");
  if(high != NULL)
  {
    if(high->high != NULL)
    {
      removetolistname(fractal_get_name(high->high),accesname);//on retire d'abord le nom de la liste de nom
      fractal_free(high->high);
    }
    free(high);
  }
  if(accesname != NULL)
  {
    freelistname(accesname);
  }
  if(listfractal != NULL)
  {
    buf_clean(listfractal);
    //free(listfractal);
  }
  if(end != NULL)
  {
    sem_destroy(&(end->acces));
    free(end);
  }
  if(otherfile != NULL)
  {
    sem_destroy(&(otherfile->acces));
    free(otherfile);
  }
  if(endoflecture != NULL)
  {
    sem_destroy(&(endoflecture->acces));
    free(endoflecture);
  }
  if(producerthread != NULL)
  {
    listthread_free(producerthread);
  }
  if(buffer != NULL)
  {
    buf_clean(buffer);
  }
  if(otherproducteur != NULL)
  {
    sem_destroy(&(otherproducteur->acces));
    free(otherproducteur);
  }
  if(consumerthread != NULL)
  {
    listthread_free(consumerthread);
  }
  if(endofproducteur != NULL)
  {
    free(endofproducteur);
  }
}

/*
 * @pre
 * @post
 */
int create_all(int etat)
{
    int err = 0;
    listfractal = (struct buff *)malloc(sizeof(struct buff));
    if(listfractal == NULL)
    {
      printf("Listfractal malloc fail. \n");
      return -1;
    }
    err = buf_init(listfractal, 15); //quelle taille ?
    if(err != 0)
    {
      printf("Listfractal creation fail. \n");
      free(listfractal);
      return -1;
    }

    accesname = (struct nameacceslist *)malloc(sizeof(struct nameacceslist));
    if(accesname == NULL)
    {
      printf("accesname malloc fail. \n");
      buf_clean(listfractal);
      free(listfractal);
      return -1;
    }
    accesname->head = NULL;
    err = sem_init(&(accesname->acces), 0, 1);
    if(err == -1){
      printf("semaphore init of accesname fail. \n");
      buf_clean(listfractal);
      free(listfractal);
      free(accesname);
      return -1;
    }

    end = (struct programend *)malloc(sizeof(struct programend));
    if(end == NULL)
    {
      printf("end malloc fail. \n");
      buf_clean(listfractal);
      free(listfractal);
      sem_destroy(&(accesname->acces));
      free(accesname);
      return -1;
    }
    err = sem_init(&(end->acces),0,1);
    if(err != 0)
    {
      printf("error during end semaphore creation");
      buf_clean(listfractal);
      free(listfractal);
      sem_destroy(&(accesname->acces));
      free(accesname);
      free(end);
      return -1;
    }

    otherfile = (struct numberlecteur * )malloc(sizeof(struct numberlecteur));
    if(otherfile == NULL)
    {
      printf("otherfile malloc fail. \n");
      buf_clean(listfractal);
      free(listfractal);
      sem_destroy(&(accesname->acces));
      free(accesname);
      sem_destroy(&(end->acces));
      free(end);
      return -1;
    }
    otherfile->number = 0;
    err = sem_init(&(otherfile->acces), 0, 1);
    if(err == -1){
      printf("error during otherfile semaphore creation");
      buf_clean(listfractal);
      free(listfractal);
      sem_destroy(&(accesname->acces));
      free(accesname);
      sem_destroy(&(end->acces));
      free(end);
      free(otherfile);
      return -1;
    }

    endoflecture = (struct programend *)malloc(sizeof(struct programend));
    if(endoflecture == NULL)
    {
      printf("endoflecture malloc fail. \n");
      buf_clean(listfractal);
      free(listfractal);
      sem_destroy(&(accesname->acces));
      free(accesname);
      sem_destroy(&(end->acces));
      free(end);
      sem_destroy(&(otherfile->acces));
      free(otherfile);
      return -1;
    }
    endoflecture->value = 0;
    err = sem_init(&(endoflecture->acces), 0, 1);
    if(err == -1){
      printf("error during endoflecture semaphore creation");
      buf_clean(listfractal);
      free(listfractal);
      sem_destroy(&(accesname->acces));
      free(accesname);
      sem_destroy(&(end->acces));
      free(end);
      sem_destroy(&(otherfile->acces));
      free(otherfile);
      free(endoflecture);
      return -1;
    }

    producerthread = (struct listthread *)malloc(sizeof(struct listthread));
    if(producerthread == NULL)
    {
      printf("producerthread malloc fail \n");
      buf_clean(listfractal);
      free(listfractal);
      sem_destroy(&(accesname->acces));
      free(accesname);
      sem_destroy(&(end->acces));
      free(end);
      sem_destroy(&(otherfile->acces));
      free(otherfile);
      sem_destroy(&(endoflecture->acces));
      free(endoflecture);
      return -1;
    }
    producerthread->head = NULL;
    producerthread->numberthread = 0;

    ////////////////////////////////////////////////////////////////////
    //Separtion des cas ou '-d' est present
    ////////////////////////////////////////////////////////////////////
    if(etat == 1) //Option avec -d
    {
      buffer = (struct buff*)malloc(sizeof(struct buff));
      if(buffer == NULL)
      {
        printf("Buffer malloc fail. \n");
        buf_clean(listfractal);
        free(listfractal);
        sem_destroy(&(accesname->acces));
        free(accesname);
        sem_destroy(&(end->acces));
        free(end);
        sem_destroy(&(otherfile->acces));
        free(otherfile);
        sem_destroy(&(endoflecture->acces));
        free(endoflecture);
        free(producerthread);
        return -1;
      }
      if(max_thread == -1)
      {
        err = buf_init(buffer, 15);
      }
      else
      {
        err = buf_init(buffer, max_thread);
      }
      if(err != 0)
      {
          printf("Error during buffer creation. \n");
          buf_clean(listfractal);
          free(listfractal);
          sem_destroy(&(accesname->acces));
          free(accesname);
          sem_destroy(&(end->acces));
          free(end);
          sem_destroy(&(otherfile->acces));
          free(otherfile);
          sem_destroy(&(endoflecture->acces));
          free(endoflecture);
          free(producerthread);
          free(buffer);
          return -1;
      }

      otherproducteur = (struct numberlecteur * )malloc(sizeof(struct numberlecteur));
      if(otherproducteur == NULL)
      {
        printf("otherproducteur malloc fail. \n");
        buf_clean(listfractal);
        free(listfractal);
        sem_destroy(&(accesname->acces));
        free(accesname);
        sem_destroy(&(end->acces));
        free(end);
        sem_destroy(&(otherfile->acces));
        free(otherfile);
        sem_destroy(&(endoflecture->acces));
        free(endoflecture);
        free(producerthread);
        buf_clean(buffer);
        return -1;
      }
      otherproducteur->number = 0;
      err = sem_init(&(otherproducteur->acces), 0, 1);
      if(err == -1){
        printf("error during otherproducteur semaphore creation");
        buf_clean(listfractal);
        free(listfractal);
        sem_destroy(&(accesname->acces));
        free(accesname);
        sem_destroy(&(end->acces));
        free(end);
        sem_destroy(&(otherfile->acces));
        free(otherfile);
        sem_destroy(&(endoflecture->acces));
        free(endoflecture);
        free(producerthread);
        buf_clean(buffer);
        free(otherproducteur);
        return -1;
      }

      consumerthread = (struct listthread *)malloc(sizeof(struct listthread));
      if(consumerthread == NULL)
      {
        printf("consumerthread malloc fail \n");
        buf_clean(listfractal);
        free(listfractal);
        sem_destroy(&(accesname->acces));
        free(accesname);
        sem_destroy(&(end->acces));
        free(end);
        sem_destroy(&(otherfile->acces));
        free(otherfile);
        sem_destroy(&(endoflecture->acces));
        free(endoflecture);
        free(producerthread);
        buf_clean(buffer);
        sem_destroy(&(otherproducteur->acces));
        free(otherproducteur);
        return -1;
      }
      consumerthread->head = NULL;
      consumerthread->numberthread = 0;

      endofproducteur = (struct programend *)malloc(sizeof(struct programend));
      if(endofproducteur == NULL)
      {
        printf("enofproducteur malloc fail \n");
        buf_clean(listfractal);
        free(listfractal);
        sem_destroy(&(accesname->acces));
        free(accesname);
        sem_destroy(&(end->acces));
        free(end);
        sem_destroy(&(otherfile->acces));
        free(otherfile);
        sem_destroy(&(endoflecture->acces));
        free(endoflecture);
        free(producerthread);
        buf_clean(buffer);
        sem_destroy(&(otherproducteur->acces));
        free(otherproducteur);
        free(consumerthread);
        return -1;
      }
      endofproducteur->value = 0;
      err = sem_init(&(endofproducteur->acces), 0, 1);
      if(err == -1)
      {
        printf("enofproducteur malloc fail \n");
        buf_clean(listfractal);
        free(listfractal);
        sem_destroy(&(accesname->acces));
        free(accesname);
        sem_destroy(&(end->acces));
        free(end);
        sem_destroy(&(otherfile->acces));
        free(otherfile);
        sem_destroy(&(endoflecture->acces));
        free(endoflecture);
        free(producerthread);
        buf_clean(buffer);
        sem_destroy(&(otherproducteur->acces));
        free(otherproducteur);
        free(consumerthread);
        free(endofproducteur);
        return -1;
      }
    }
    else //option sans -d
    {
      high = (struct fractalHigh *)malloc(sizeof(struct fractalHigh));
      if(high == NULL){
        printf("high malloc fail \n");
        buf_clean(listfractal);
        free(listfractal);
        sem_destroy(&(accesname->acces));
        free(accesname);
        sem_destroy(&(end->acces));
        free(end);
        sem_destroy(&(otherfile->acces));
        free(otherfile);
        sem_destroy(&(endoflecture->acces));
        free(endoflecture);
        free(producerthread);
        return -1;
      }
      err = sem_init(&(high->acces), 0, 1);      /* Au debut, n slots vides */
      if(err !=0)
      {
        printf("error during semaphore high creation");
        buf_clean(listfractal);
        free(listfractal);
        sem_destroy(&(accesname->acces));
        free(accesname);
        sem_destroy(&(end->acces));
        free(end);
        sem_destroy(&(otherfile->acces));
        free(otherfile);
        sem_destroy(&(endoflecture->acces));
        free(endoflecture);
        free(producerthread);
        free(high);
        return -1;
      }
    }
    return 0;
}

/*
 * @pre
 * @post
 */
void printallname(struct nameacceslist *list)
{
  sem_wait(&(list->acces));
  struct name *current = list->head;
    while(current != NULL)
    {
      printf("%s \n",current->name);
      current = (current->next);
    }
  sem_post(&(list->acces));
}

/*
 * @pre
 * @post
 */
int readfile(int argc, char *argv[], int begin, int type)
{
  int err = 0;
  pthread_t lecteur[argc-begin]; //tableau de thread de taille egal au nombre de fichier
  int trynumber = 0;
  int threadreaderfail = 0;
  for(int i = begin; (i < argc) && (isendofprogram(end) == 0);i++)
  {
    char * filename = *(argv+i);
    err = pthread_create(&lecteur[i-begin],NULL,(void *)&lecture,(void *)(*(argv+i)));
    if(err != 0)
    {
      if(trynumber > 10) //si pthread_create echoue, reessayer
      {
          threadreaderfail++;
          if(threadreaderfail > argc-begin-1) //si aucun thread de lecture n'a
                                              //ete cree alors envoyer le message d'arret
          {
            printf("Error during reader thread creation \n");
            setendofprogram(end); //met la valeur de la struture end à -1
            return -1;
          }
      }
      else
      {
        trynumber++;
        i--;
      }
    }
    trynumber = 0;
    sem_wait(&(otherfile->acces));
    (otherfile->number)++;
    sem_post(&(otherfile->acces));
  }
  if(type == 1) // type 1 = avec parametre '-d'
  {
      err = thread_all();
      if(err == -1)
      {
          printf("error during thread calculating");
          return -1;
      }
  }
  else // type 2 = sans parametre '-d'
  {
      err = thread_moyenne();
      if(err == -1)
      {
          printf("error during thread calculating");
          return -1;
      }
  }
  //Recuperation des threads de lecture
  for(int i = begin; i < argc;i++)
  {
    err = pthread_join(lecteur[i-begin], NULL);
    if(err!=0) //erreur lors d'un thread
    {
        printf("lecture pthread end with error");
        setendofprogram(end);
    }
  }
  return isendofprogram(end); //recupere la valeur de la structure end : 0 = pas d'erreur -1 = erreur
}

/*
 * @pre
 * @post
 */


 void * lecture(void* parametre)
 {
	printf("Lecture du fichier : %s \n",(char *)parametre);
	int err=0;
	char* filename = (char *)parametre;
	int file = open(filename,O_RDONLY);
	if(file==-1)
	{
		printf("error during file opening : %s",filename);
		sem_wait(&(otherfile->acces));
		(otherfile->number)--;
		sem_post(&(otherfile->acces));
		pthread_exit(NULL);
	}
	else
	{
		struct fileinfo filu;
		struct fileinfo *fileptr = &filu;
		fileptr->fd = file;
		struct stat buff;
		struct stat *buffptr = &buff;
		fstat(fileptr->fd, buffptr);
		fileptr->sizefile = buffptr->st_size;

		fileptr->offset = 0;
		fileptr->finished = 0;
		int readtest = 0;

		char biffer[5*65];
		int lenbiffer = 5*65;
		if (refresh(fileptr) == -1)
		{
			if(close(file)==-1)
			{
				printf("error during file closing : %s",filename);
			}
			sem_wait(&(otherfile->acces));
			(otherfile->number)--;
			sem_post(&(otherfile->acces));
			pthread_exit(NULL);
		}
		while (fileptr->finished == 0 || readtest == 0)
		{
			readtest = read2(fileptr, biffer, lenbiffer);
			if (readtest == 0 || readtest == 2)
			{
				struct fractal *newfract = split(biffer);
				if(newfract == NULL)
				{
				      printf("unable to create a fractal\n");
				}
				else
				{
					if(verifyduplicatename(newfract->name,accesname)==0)
					{
						buf_insert(listfractal, newfract);
						err = addtolistname(newfract->name,accesname);
						if(err != 0)
						{
							if(close(file)==-1)
							{
								printf("error during file closing : %s",filename);
							}
							printf("fail during add name to list");
							munmap(fileptr->msg, fileptr->memload);
							sem_wait(&(otherfile->acces));
							(otherfile->number)--;
							sem_post(&(otherfile->acces));
							pthread_exit(NULL);
						}
					}
					else
					{
						fractal_free(newfract);
					}
				}
			}
			else if (readtest == -1 || readtest == 1)
			{
				if(close(file)==-1)
				{
					printf("error during file closing : %s",filename);
				}
				printf("file finished\n");
				munmap(fileptr->msg, fileptr->memload);
				sem_wait(&(otherfile->acces));
				(otherfile->number)--;
				sem_post(&(otherfile->acces));
				pthread_exit(NULL);

			}
		}



		if(close(file)==-1)
		{
			printf("error during file closing : %s",filename);
		}
		munmap(fileptr->msg, fileptr->memload);
		sem_wait(&(otherfile->acces));
		(otherfile->number)--;
		sem_post(&(otherfile->acces));
		pthread_exit(NULL);
	}
 }

/*
void * lecture(void* parametre)
{

  printf("Lecture du fichier : %s \n",(char *)parametre);
  int err=0;
  char* filename = (char *)parametre;
  int file = open(filename,O_RDONLY);
  if(file==-1)
  {
    printf("error during file opening : %s",filename);
    sem_wait(&(otherfile->acces));
    (otherfile->number)--;
    sem_post(&(otherfile->acces));
    pthread_exit(NULL);
  }
  else
  {
    int etat = 1;
    while(etat!=0)
    {
      int i = 0;
      char caractere = ' ';
      char ligne[64+32*2+2*sizeof(double)];
      while(caractere != '\n' && etat!=0)
      {
        etat = read(file,(void*)&caractere,sizeof(char));
        if(etat <= 0)
        {
          if(close(file)==-1)
          {
                  printf("error during file closing : %s",filename);
				  //code recurant
                  sem_wait(&(otherfile->acces));
                  (otherfile->number)--;
                  sem_post(&(otherfile->acces));
                  pthread_exit(NULL);
          }
          if(etat < 0)
          {
            printf("error during file reading : %s",filename);
          }
          sem_wait(&(otherfile->acces));
          (otherfile->number)--;
          sem_post(&(otherfile->acces));
          pthread_exit(NULL);
        }
        //printf("%c",caractere);
        *(ligne+i) = caractere;
        i++;
      }
      //printf("\n");
      if((*ligne != '#') && (*ligne != '\n')) //il ne s'agit pas d'une ligne de commentaire ni d'une ligne vide
      {
        struct fractal *newfract = split(ligne);
        if(newfract == NULL)
        {
          if(close(file)==-1)
          {
                 printf("error during file closing : %s",filename);
                 sem_wait(&(otherfile->acces));
                 (otherfile->number)--;
                 sem_post(&(otherfile->acces));
                 pthread_exit(NULL);
          }
          printf("fail during fractal build");
          sem_wait(&(otherfile->acces));
          (otherfile->number)--;
          sem_post(&(otherfile->acces));
          pthread_exit(NULL);
        }
        else
        {
          if(verifyduplicatename(newfract->name,accesname)==0)
          {
            buf_insert(listfractal, newfract);
            err = addtolistname(newfract->name,accesname);
            if(err != 0)
            {
              if(close(file)==-1)
              {
                      printf("error during file closing : %s",filename);
                      sem_wait(&(otherfile->acces));
                      (otherfile->number)--;
                      sem_post(&(otherfile->acces));
                      pthread_exit(NULL);
              }
              printf("fail during add name to list");
              sem_wait(&(otherfile->acces));
              (otherfile->number)--;
              sem_post(&(otherfile->acces));
              pthread_exit(NULL);
            }
          }
          else
          {
            fractal_free(newfract);
          }
        }
      }
    }
    if(close(file)==-1)
    {
            printf("error during file closing : %s",filename);
            sem_wait(&(otherfile->acces));
            (otherfile->number)--;
            sem_post(&(otherfile->acces));
            pthread_exit(NULL);
    }
    sem_wait(&(otherfile->acces));
    (otherfile->number)--;
    sem_post(&(otherfile->acces));
    pthread_exit(NULL);
  }
}
*/


/*
 * @pre
 * @post
 */
struct fractal * split(char* line)
{
   int i = 0;
   int position=0;
   int place=0;
   int numberarg=0;
   char *splitedline[5];
   char ligne1[65];
   char ligne2[65];
   char ligne3[65];
   char ligne4[65];
   char ligne5[65];
   *splitedline = ligne1;
   *(splitedline + 1) = ligne2;
   *(splitedline + 2) = ligne3;
   *(splitedline + 3) = ligne4;
   *(splitedline + 4) = ligne5;
   while(*(line+i) != '\n' && numberarg < 5 && position<65)
   {
     if(*(line+i) == ' ')
     {
       *((*(splitedline+place))+position) = '\0';
       position = 0;
       place++;
       numberarg++;
     }
     else
     {
       *((*(splitedline+place))+position) = *(line+i);
       position++;
     }
     i++;
   }

   if(*(line+i) == '\n')
   {
     *((*(splitedline+place))+position) = '\0';
     numberarg++;
   }
   if(numberarg==5)
   {
     const char* name = *splitedline;
     int width = atoi(*(splitedline+1));
     int height = atoi(*(splitedline+2));
     double a = atof(*(splitedline+3));
     double b = atof(*(splitedline+4));
     //struct fractal *newfract;// = fractal_new(name, width, height, a, b);
     if (width != 0 && height != 0)
     {
       struct fractal* newfract = fractal_new(name, width, height, a, b);
       return newfract;
     }
     return NULL;
   }
   else
   {
     return NULL; //invalide line
   }
}

/*
 * @pre
 * @post
 */
int verifyduplicatename(char* name, struct nameacceslist *list)
{
  sem_wait(&(list->acces));
  struct name *current = list->head;
  while(current != NULL)
  {
    if(strcmp((name),current->name)==0) //si le nom de la nouvelle fractal existe deja
    {
      sem_post(&(list->acces));
      return -1;
    }
    current = (current->next);
  }
  sem_post(&(list->acces));
  return 0; //le nom de la nouvelle fractal n'existe pas encore
}

/*
 * @pre
 * @post
 */
int addtolistname(char* name, struct nameacceslist *list)
{
  sem_wait(&(list->acces));
  struct name *current = list->head;
  if(current == NULL)
  {
    struct name *node = (struct name *)malloc(sizeof(struct name));
    if(node == NULL)
    {
      return -1;
    }
    node->name = name;
    node->next = NULL;
    list->head = node;
    sem_post(&(list->acces));
  }
  else
  {
    while(current->next != NULL)
    {
      current = (current->next);
    }
    struct name *node = (struct name *)malloc(sizeof(struct name));
    if(node == NULL)
    {
      return -1;
    }
    current->next = node;
    node->name = name;
    node->next = NULL;
    sem_post(&(list->acces));
  }
}

/*
 * @pre
 * @post
 */
int removetolistname(const char* name, struct nameacceslist *list)
{
  sem_wait(&(list->acces));
  struct name *current = list->head;
  if(current == NULL)
  {
    sem_post(&(list->acces));
    return -1; //not found
  }
  if(strcmp(current->name,name)==0) //it's the first one
  {
    if(current->next == NULL)
    {
        free(current);
        list->head = NULL;
        sem_post(&(list->acces));
        return 0;
    }
    else
    {
      struct name *suivant = current->next;
      free(current);
      list->head = suivant;
      sem_post(&(list->acces));
      return 0;
    }
  }
  else
  {
    while(current->next != NULL)
    {
      if(strcmp(current->next->name,name)==0)
      {
        struct name *suivant = current->next->next;
        free(current->next);
        current->next = suivant;
        sem_post(&(list->acces));
        return 0;
      }
      current = (current->next);
    }
    sem_post(&(list->acces));
    return -1;  //not found
  }
}

/*
 * @pre
 * @post
 */
void freelistname(struct nameacceslist *list)
{
  struct name *head = list->head;
  if(head!=NULL)
  {
    struct name *current = head;
    struct name *suivant = head->next;
    printallname(accesname);
    while(current->next != NULL)
    {
      free(current);
      current = suivant;
      suivant = current->next;
    }
    free(current->name);
    free(current);
  }
  sem_destroy(&(list->acces));
  free(list);
}

/*
 * @pre buffer!=NULL, n>0
 * @post a construit un buffer partagé contenant n slots
 * MALLOC : buf, (empty, full)->semaphore
 */
int buf_init(struct buff *buf, int n)
{
    int err;
    buf->buf = (struct fractal **)malloc(sizeof(struct fractal*)*n); //cree un tableau de n slot
    if((buf->buf)==NULL)
    {
       return -1;
    }
    buf->length = n;                       /* Buffer content les entiers */
    buf->begin = buf->end = 0;        /* Buffer vide si front == rear */
    err = sem_init(&(buf->empty), 0, n);      /* Au debut, n slots vides */
    if(err !=0)
    {
      printf("error during semaphore creation");
      free(buf->buf);
      return -1;
    }
    err = sem_init(&(buf->full), 0, 0);      /* Au debut, rien a consommer */
    if(err !=0)
    {
      printf("error during semaphore creation");
      free(buf->buf);
      sem_destroy(&(buf->empty));
      return -1;
    }
}



/*
 * @pre fract!=NULL
 * @post libere le buffer et la structure
 */
void buf_clean(struct buff *buffer)
{
    struct fractal *f;
    int ret = buf_isempty(buffer,&f);
    printf(" Buffer is empty  == -1 sinon 0 :%d \n",ret);
    while(ret == 0 && f != NULL)
    {
      printf("Il reste une fractal : %s \n",fractal_get_name(f));
      fractal_free(f);
      ret = buf_isempty(buffer,&f);
    }
    free(buffer->buf);
    sem_destroy(&(buffer->empty));
    sem_destroy(&(buffer->full));
    free(buffer);
}

/* @pre buffer!=NULL
 * @post ajoute item a la fin du buffer partage. Ce buffer est gere
 *       comme une queue FIFO
 */
void buf_insert(struct buff *buffer, struct fractal *item)
{
    sem_wait(&(buffer->empty));
    pthread_mutex_lock(&buffer->mutex);
    struct fractal **val = buffer->buf;
    val[((buffer->end)+1)%(buffer->length)] = item;
    buffer->end = (buffer->end + 1)%(buffer->length);
    pthread_mutex_unlock(&buffer->mutex);
    sem_post(&(buffer->full));
}

/* @pre buf!=NULL
 * @post retire le dernier item du buffer partage
 */
struct fractal* buf_remove(struct buff *buffer)
{
    sem_wait(&(buffer->full));
    pthread_mutex_lock(&buffer->mutex);
    struct fractal *retour = ((buffer->buf)[(buffer->begin)+1]);
    ((buffer->buf)[(buffer->begin)+1]) = NULL;
    buffer->begin = (buffer->begin + 1)%(buffer->length);
    pthread_mutex_unlock(&buffer->mutex);
    sem_post(&(buffer->empty));
    return retour;
}

/*
 * @pre
 * @post
 */
int buf_isempty(struct buff *buffer,struct fractal **f)
{
  int err = 0;
  err = sem_trywait(&(buffer->full));
  if(err != 0)
  {
    *f = NULL;
    return -1;
  }
  else
  {
    pthread_mutex_lock(&buffer->mutex);
    struct fractal *retour = ((buffer->buf)[((buffer->begin)+1)%(buffer->length)]);
    ((buffer->buf)[((buffer->begin)+1)%(buffer->length)]) = NULL;
    buffer->begin = (buffer->begin + 1)%(buffer->length);
    pthread_mutex_unlock(&buffer->mutex);
    sem_post(&(buffer->empty));
    *f = retour;
    return 0;
  }
}

/*
 * @pre
 * @post
 */
void setendofprogram(struct programend *f)
{
  sem_wait(&(f->acces));
  f->value = -1;
  sem_post(&(f->acces));
}

/*
 * @pre
 * @post
 */
int isendofprogram(struct programend *f)
{
  sem_wait(&(f->acces));
  int val = f->value;
  sem_post(&(f->acces));
  return val;
}

/*
 * @pre
 * @post
 */
int verify_end(struct buff *buffer,struct fractal **f)
{
  int val;
  sem_getvalue(&(buffer->full),&val);
  //printf("la valeur de la semaphore pour consommateur est : %d",val);
  int etat = buf_isempty(buffer,f);
  sem_wait(&(otherfile->acces));
  int etat2 = otherfile->number;
  sem_post(&(otherfile->acces));
  if(etat == 0 || etat2 != 0) //encore des fractal dans buffer ou encore des fichiers de lecture
  {
      return 0;
  }
  else
  {
    setendofprogram(endoflecture);
    printf("Fin du programme pour les producteurs \n");
    return -1;
  }
}

/*
 * @pre
 * @post
 */
int verify_endproducteur(struct buff *buffer,struct fractal **f)
{
  pthread_mutex_lock(&verification);
  int numbertry = 1;
  int etat = 0;
  while(numbertry >=0)
  {
    etat = buf_isempty(buffer,f);
    if(*f != NULL)
    {
      numbertry = -1;
    }
    else
    {
      numbertry--;
    }
  }
  int etat2 = isendofprogram(endoflecture);
  sem_wait(&(otherproducteur->acces));
  int etat3 = otherproducteur->number;
  sem_post(&(otherproducteur->acces));
  if(etat == 0 || etat3 != 0 || etat2 == 0) //encore des fractal dans buffer ou encore des fichiers de lecture
  {
    pthread_mutex_unlock(&verification);
    return 0;
  }
  else
  {
    setendofprogram(endofproducteur);
    printf("Fin du programme pour les consommateurs \n");
    pthread_mutex_unlock(&verification);
    return -1;
  }
}

/*
 * @pre
 * @post
 */
int fractalhighmodify(struct fractalHigh *f, struct fractal *frac, int average)
{
  sem_wait(&(f->acces));
  if(f->average < average)
  {
    f->average = average;
    if(f->high != NULL)
    {
      removetolistname(fractal_get_name(f->high), accesname);
      fractal_free(f->high);
    }
    f->high = frac;
    sem_post(&(f->acces));
    return 1;
  }
  else
  {
    sem_post(&(f->acces));
    return 0;
  }
}

/*
 * @pre
 * @post
 */
struct fractal *getfractalhigh(struct fractalHigh *f)
{
  sem_wait(&(f->acces));
  struct fractal *big =  f->high;
  sem_post(&(f->acces));
  return big;
}

/*
 * @pre
 * @post
 */
void listthread_free(struct listthread *list)
{
  struct thread *current = list->head;
  struct thread *next;
  while(current != NULL)
  {
    next = current->next;
    free(current->thread);
    free(current);
    current = next;
  }
  free(list);
}

/*
 * @pre
 * @post
 */
pthread_t *removethread(struct listthread *list)
{
  if(list == NULL)
  {
    return NULL;
  }
  struct thread *head = list->head;
  if(head == NULL)
  {
    return NULL;
  }
  list->head = head->next;
  list->numberthread = list->numberthread - 1;
  pthread_t *retour = head->thread;
  free(head);
  return retour;
}

/*
 * @pre
 * @post
 */
int insertthread(struct listthread *list,void* funct)
{
  int err = 0;
  if(list == NULL)
  {
    return -2;
  }
  struct thread *head = list->head;
  pthread_t *new = (pthread_t *)malloc(sizeof(pthread_t));
  if(new == NULL)
  {
    return -1;
  }
  struct thread *add = (struct thread*)malloc(sizeof(struct thread));
  if(add == NULL)
  {
    free(new);
    return -1;
  }
  add->thread = new;
  add->next = NULL;

  if(head == NULL)
  {
    err=pthread_create(new,NULL,funct,NULL);
    if(err!=0)
    {
        free(new);
        free(add);
        printf("pthread_create fail");
        return -1;
    }
    head = add;
    list->head = head;
    list->numberthread = list->numberthread + 1;
    return 0;
  }
  else
  {
    struct thread *current = head;
    while(current->next != NULL)
    {
      current = current->next;
    }
    err=pthread_create(new,NULL,funct,NULL);
    if(err!=0)
    {
        free(new);
        free(add);
        printf("pthread_create fail");
        return -1;
    }
    current->next = add;
    list->numberthread = list->numberthread + 1;
    return 0;
  }
}

/*
 * @pre
 * @post
 */
int thread_moyenne()
{
    int err;
    int number = 0;
    if(fractalhighmodify == NULL)
    {
      printf("fractalhighmodify doesn't exist. \n");
      setendofprogram(end);
      return -1;
    }
    fractalhighmodify(high,NULL,INT_MIN);
    for(int i=0;((i<max_thread || max_thread < 0) && (isendofprogram(endoflecture)==0) && (isendofprogram(end) == 0));i++) {
        err=insertthread(producerthread,(void*)&producermoyenne);
        if(err!=0)
        {
            printf("pthread_create producter fail. \n");
            //end of programme ?
            return -1;
        }
        number++;
    }
    int value = 0;
    int numberrecup =0;
    while(value == 0)
    {
        pthread_t *current = removethread(producerthread);
        if(current == NULL)
        {
          printf("Plus de thread producteur \n");
          value = -1;
        }
        else
        {
          numberrecup ++;
          err = pthread_join(*current, NULL);
          free(current);
          if(err!=0)
          {
              printf("pthread end with error");
              setendofprogram(end);
              return -1;
          }
        }
    }
    if(isendofprogram(end) == 0)
    {
      struct fractal *big = getfractalhigh(high);
      if(big == NULL)
      {
        printf("No fractal high\n");
        setendofprogram(end);
        return -1;
      }
      printf("Le nom de la fractal est : %s \n",fractal_get_name(big));
      err = write_bitmap_sdl(big, fractal_get_name(big));
      if(err != 0)
      {
        printf("Error with write bitmap function");
        return -1;
      }
      printf("thread créé : %d et thread recupere  : %d \n",number,numberrecup);
      return 0;
    }
    else
    {
      printf("Pogram stop with end message");
      return -1;
    }
}

/*
 * @pre
 * @post
 */
int thread_all()
{
    int err = 0;
    int numberproducteur = 0;
    int numberconsumer = 0;
    int arret =0;
    int i =0; //pour les producteurs
    int j =0; //pour les consommateurs
    while((arret == 0 )&& (isendofprogram(end)== 0))
    {
      if(i < 30 && (i<max_thread || max_thread < 0) && (isendofprogram(endoflecture)==0) && (isendofprogram(end)==0))
      {
        err=insertthread(producerthread,(void*)&producer);
        if(err!=0)
        {
            printf("pthread_create fail\n");
        }
        else
        {
          sem_wait(&(otherproducteur->acces));
          (otherproducteur->number)++;
          sem_post(&(otherproducteur->acces));
          numberproducteur++;
          i++;
        }
      }
      if(j<20 && j < numberproducteur+1 && (isendofprogram(endofproducteur)==0) && (isendofprogram(end)==0))
      {
        err = sem_trywait(&(buffer->full));
        if(err != 0)
        {
            sleep(0.5);
        }
        else
        {
          sem_post(&(buffer->full));
          err=insertthread(consumerthread,(void*)&consumer);
          // printf("Consommateur \n");
          if(err!=0)
          {
              printf("pthread_create fail\n");
          }
          else
          {
            numberconsumer++;
            j++;
          }
        }
      }
      else
      {
        arret = -1;
        printf("Arret de création des threads \n");
      }
    }
    printf("numbre thread producteur créé : %d \n",numberproducteur);
    printf("Nombre de thread consumer créé : %d \n",numberconsumer);


    int numberproducteurrecup = 0;
    int value = 0;
    while(value == 0)
    {
        pthread_t *current = removethread(producerthread);
        if(current == NULL)
        {
          printf("Plus de thread producteur \n");
          value = -1;
        }
        else
        {
          numberproducteurrecup ++;
          err = pthread_join(*current, NULL);
          free(current);
          if(err!=0)
          {
              setendofprogram(end);
              printf("pthread end with error");
              return -1;
          }
        }
    }
    printf("numbre thread producteur récupèré : %d \n",numberproducteurrecup);


    value = 0;
    int numberconsumerrecup =0;
    while(value == 0)
    {
        pthread_t *current = removethread(consumerthread);
        if(current == NULL)
        {
          printf("Plus de thread consommateur \n");
          value = -1;
        }
        else
        {
          numberconsumerrecup ++;
          err = pthread_join(*current, NULL);
          free(current);
          if(err!=0)
          {
              setendofprogram(end);
              printf("pthread end with error");
              return -1;
          }
        }
    }
    printf("numbre thread consommateur récupèré : %d \n",numberconsumerrecup);
    return 0;
}

/*
 * @pre
 * @post
 */
void *producer(void *parametre)
{
  struct fractal *f;
  while((isendofprogram(endoflecture) == 0) && (isendofprogram(end)== 0) && (verify_end(listfractal,&f) == 0))
  {
    if(f != NULL)
    {
        int val;
        for(int a=0; (a<(fractal_get_width(f)*fractal_get_height(f)));a++)
        {
            int x = a % (fractal_get_width(f));
            int y = a/(fractal_get_width(f));
            val = fractal_compute_value(f, x, y);
            fractal_set_value(f,x,y,val);
        }
        if(isendofprogram(end)!= 0)
        {
          removetolistname(fractal_get_name(f),accesname);
          fractal_free(f);
          pthread_exit(NULL);
        }
        printf("Une fractal terminee avec succes : %s \n",fractal_get_name(f));
        buf_insert(buffer, f);
    }
    else
    {
      printf("Thread producteur sleep \n");
      sleep(0.5);
    }
  }
  sem_wait(&(otherproducteur->acces));
  (otherproducteur->number)--;
  sem_post(&(otherproducteur->acces));
  pthread_exit(NULL);
}

/*
 * @pre
 * @post
 */
void *producermoyenne(void *parametre)
{
  struct fractal *fractalhigh;
  int sum = 0;
  struct fractal *f;
  while((isendofprogram(endoflecture) == 0) && (isendofprogram(end) == 0) && (verify_end(listfractal,&f) == 0))
  {
    if(f != NULL)
    {
        int val;
        sum = 0;
        for(int a=0; (a<(fractal_get_width(f)*fractal_get_height(f)));a++)
        {
            int x = a % (f->width);
            int y = a/(f->width);
            val = fractal_compute_value(f, x, y);
            sum = sum + val;
            fractal_set_value(f,x,y,val);
        }
        if(isendofprogram(end) != 0)
        {
          printf("End programme different de 0 \n");
          removetolistname(fractal_get_name(f), accesname);
          fractal_free(f);
          pthread_exit(NULL);
        }
        sum = sum / (fractal_get_width(f))*(fractal_get_height(f));
        printf("Une fractal terminee avec succes : %s \n",fractal_get_name(f));
        int retour = fractalhighmodify(high,f,sum);
        if(retour == 0)
        {
          removetolistname(fractal_get_name(f), accesname);
          fractal_free(f);
        }
     }
  }
  pthread_exit(NULL);
}

/*
 * @pre
 * @post
 */
void *consumer(void *parametre)
{
 struct fractal *fract = NULL;
 while((isendofprogram(endofproducteur) == 0) && (isendofprogram(end)  == 0) && (verify_endproducteur(buffer,&fract) == 0))
 {
   if(fract != NULL)
   {
     printf("une fractal bitmaper de nom : %s \n",fractal_get_name(fract));
     write_bitmap_sdl(fract, fractal_get_name(fract));
     removetolistname(fractal_get_name(fract), accesname);
     fractal_free(fract);
     fract = NULL;
   }
   else
   {
   }
 }
 pthread_exit(NULL);
}

/*	the refresh function loads part/whole file in memory depending on the file size
 *	it does so by modifying the fileinfo structure
 *
 *	it accept a fileinfo structure only
 *
 *	it return an int,
 *	return 0 when succsesful
 *	return -1 when failed
 */


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
		if ((file->sizefile - file->offset) > SIZE_MAX)
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
			file->msg = (long*)mmap(NULL, (file->sizefile - file->offset), PROT_READ, MAP_PRIVATE, file->fd, file->offset);
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


/*	this function search for the first linefeed found inside memory, it handles the different cases
 *
 *	it accept a fileinfo structure only
 *
 *	it returns a int different in each case,
 *	return 0 if everything went correctly
 *	return 1 if we arrived at the end of the file
 *	return -1 in case of error
 */


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
int read2(struct fileinfo *file, char* biffer, int lenbiffer)
{
	int status = 0;
	int t = 0;
	int i = 0;
	int j = 0;
	if (*(file->readhead) == ' ' || *(file->readhead) == '#')
	{
		t = firstlf(file);
		if (t == 1)
		{
			return 1;
		}
		else if (t == -1)
		{
			return -1;
		}
		return read2(file, biffer, lenbiffer);
	}
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
			fprintf(stderr, "line too long for the biffer or the biffer too small\n");
			fprintf(stderr, "line skipped\n");
			t = firstlf(file);
			if (t == -1)
			{
				fprintf(stderr, "error in firstlf function\n");
				return -1;
			}
			if (t == 1)
			{
				return 1;
			}
			return read2(file, biffer, lenbiffer);
		}
		else if (*((file->readhead)+j) != '\n' && (file->readsize)-j == 0) 										//arrived at the end of the memory map but not a the end of the line
		{
			if (file->finished == 1)
			{
				printf("File finished fd :%d\n", file->fd);
				printf("return code 1\n");
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
				printf("return code 2\n");
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
	fprintf(stderr, "error in read2 function\n");
	return -1;
}
