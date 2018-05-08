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
#define BITMAP_ALL 1
#define BITMAP_AVERAGE 2

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

//Structure contenant la tete de la liste chainee des fractal high
struct listfractalhigh{
  struct nodefractal *head;
  int average;
  sem_t acces;
};

//Liste chainee contenant les fractal high
struct nodefractal{
  struct fractal *fract;
  struct nodefractal *next;
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
 *	finished 	: int qui decrit si la zone memoire recouvre la fin du fichier ou pas
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
int bitmapallfractalhigh(struct listfractalhigh *f);
void deleteallfractalhigh(struct listfractalhigh *f);
int HighAverageModify(struct listfractalhigh *f, struct fractal *frac, int average, struct buff *buffer, int type);
int addtolistfractalhigh(struct listfractalhigh *f, struct fractal *frac);
int clean_listfractalhigh(struct listfractalhigh *f, struct buff *buffer, int type);
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
struct listthread *producerthread = NULL;
struct listthread *consumerthread = NULL;
struct listfractalhigh *listhigh = NULL;
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
  new.rlim_cur = 4194304;
  new.rlim_max = 8388608;
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
                printf("bad max_thread number\n");
                return -1;
            }
            err = create_all(BITMAP_ALL); //Initialisation de toutes les variables utilent au programme
            if(err !=0)
            {
              return -1;
            }
            err = readfile(argc,argv,4,1);
            if(err == -1)
            {
                printf("error during read file\n");
                clean_all(); //free de toutes les variables cree par malloc
                return -1;
            }
            bitmapallfractalhigh(listhigh);
            clean_all(); //free de toutes les variables cree par malloc
            return 0;
        }
        else //Non presence du parametre '--maxthreads' dans les arguments
        {
            max_thread = -1; //Si pas de nombre maximum de thread
            err = create_all(BITMAP_ALL); //Initialisation de toutes les variables utilent au programme
            if(err !=0)
            {
              return -1;
            }
            err = readfile(argc,argv,2,1);
            if(err == -1)
            {
                printf("error during read file\n");
                clean_all(); //free de toutes les variables cree par malloc
                return -1;
            }
            bitmapallfractalhigh(listhigh);
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
                printf("bad max thread number\n");
                //error(err,"bad max_thread number");
                return -1;
            }
            err = create_all(BITMAP_AVERAGE); //Initialisation de toutes les variables utilent au programme
            if(err !=0)
            {
              return -1;
            }
            err = readfile(argc,argv,3,2);
            if(err == -1)
            {
                printf("error during read file\n");
                clean_all(); //free de toutes les variables cree par malloc
                return -1;
            }
            bitmapallfractalhigh(listhigh);
            clean_all(); //free de toutes les variables cree par malloc
            return 0;
        }
        else //Non presence du parametre '--maxthreads' dans les arguments
        {
            max_thread = -1;
            err = create_all(BITMAP_AVERAGE); //Initialisation de toutes les variables utilent au programme
            if(err !=0)
            {
              return -1;
            }
            err = readfile(argc,argv,1,2);
            if(err == -1)
            {
                printf("error during read file\n");
                clean_all(); //free de toutes les variables cree par malloc
                return -1;
            }
            bitmapallfractalhigh(listhigh);
            clean_all(); //free de toutes les variables cree par malloc
           return 0;
        }
    }
    return -1;
}

/*
 * @pre /
 * @post Supprime toutes les variables, initialisées avec malloc, de la memoire avant de fermer
 * le processus
 */
void clean_all()
{
  printf("Function clean all variables \n");
  if(accesname != NULL)
  {
    printallname(accesname);
    freelistname(accesname);
  }
  if(listfractal != NULL)
  {
    buf_clean(listfractal);
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
  if(listhigh != NULL)
  {
    deleteallfractalhigh(listhigh);
  }
}

/*
 * @pre etat == 1 || etat ==2
 * @post Cette fonction permet d'initialiser toutes les variables (globals) nécessaires au fonctionnement du programme
 * Return -1 si une des variables n'a pas pu être initialisée et affiche un message d'erreur.
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
    err = buf_init(listfractal, 100); //quelle taille ?
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
      printf("error during end semaphore creation\n");
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
      printf("error during otherfile semaphore creation\n");
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
      printf("error during endoflecture semaphore creation\n");
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

    listhigh = (struct listfractalhigh *)malloc(sizeof(struct listfractalhigh));
    if(listhigh == NULL){
      printf("listhigh malloc fail \n");
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
    err = sem_init(&(listhigh->acces), 0, 1);      /* Au debut, n slots vides */
    if(err !=0)
    {
      printf("error during semaphore high creation\n");
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
      free(listhigh);
      return -1;
    }
    listhigh->head = NULL;
    listhigh->average = INT_MIN;

    ////////////////////////////////////////////////////////////////////
    //Separtion des cas ou '-d' est present
    ////////////////////////////////////////////////////////////////////
    if(etat == BITMAP_ALL) //Option avec -d
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
        sem_destroy(&(listhigh->acces));
        free(listhigh);
        return -1;
      }
      if(max_thread == -1)
      {
        err = buf_init(buffer, 100);
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
          sem_destroy(&(listhigh->acces));
          free(listhigh);
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
        sem_destroy(&(listhigh->acces));
        free(listhigh);
        buf_clean(buffer);
        return -1;
      }
      otherproducteur->number = 0;
      err = sem_init(&(otherproducteur->acces), 0, 1);
      if(err == -1){
        printf("error during otherproducteur semaphore creation\n");
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
        sem_destroy(&(listhigh->acces));
        free(listhigh);
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
        sem_destroy(&(listhigh->acces));
        free(listhigh);
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
        sem_destroy(&(listhigh->acces));
        free(listhigh);
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
        sem_destroy(&(listhigh->acces));
        free(listhigh);
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
      //Rien à faire en plus
    }
    return 0;
}

/*
 * @pre list != NULL
 * @post Affiche tous les noms présent dans la liste chainée "list"
 */
void printallname(struct nameacceslist *list)
{
  sem_wait(&(list->acces));
  struct name *current = list->head;
    while(current != NULL)
    {
      //printf("%s \n",current->name);
      current = (current->next);
    }
  sem_post(&(list->acces));
}

/*
 * @pre argc > 1 && argv !=NULL && begin >1 && (type==1 || type ==2)
 * @post Les fichiers passe en arguments ont ete lu et ferme correctement
 * ERREUR : 0 = lecture de tous les fichiers effectue correctement
 *         -1 = erreur
 * Affiche un message d'erreur en cas d'erreur.
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
    (otherfile->number)++; // Incrémente de 1 le nombre de thread de lecture
    sem_post(&(otherfile->acces));
  }
  if(type == BITMAP_ALL) // type 1 = avec parametre '-d'
  {
      err = thread_all();
      if(err == -1)
      {
          printf("error during thread calculating\n");
          return -1;
      }
  }
  else // type 2 = sans parametre '-d'
  {
      err = thread_moyenne();
      if(err == -1)
      {
          printf("error during thread calculating\n");
          return -1;
      }
  }
  //Recuperation des threads de lecture
  for(int i = begin; i < argc;i++)
  {
    err = pthread_join(lecteur[i-begin], NULL);
    if(err!=0) //erreur lors d'un thread
    {
        printf("lecture pthread_join() end with error\n");
        setendofprogram(end);
    }
  }
  return isendofprogram(end); //recupere la valeur de la structure end : 0 = pas d'erreur -1 = erreur
}

/*
 * @pre accepte seulement un pointeur vide qui sera typecast en entier
 * @post lecture lit dans un fichier et créées avec les lignes valides des structure fractal
 * Ne retourne rien
 */


 void * lecture(void* parametre)
 {
	printf("Lecture du fichier : %s \n",(char *)parametre);
	int err=0;
	char* filename = (char *)parametre;
	int file = open(filename,O_RDONLY);
	if(file==-1)
	{
		printf("error during file opening : %s\n",filename);
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
				printf("error during file closing : %s\n",filename);
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
            char *nametolist = (char*)malloc(sizeof(char)*(strlen(newfract->name)+1));
            if(nametolist == NULL)
            {
              fractal_free(newfract);
            }
            else
            {
              strcpy(nametolist,newfract->name);
  						err = addtolistname(nametolist,accesname);
  						if(err != 0)
  						{
  							if(close(file)==-1)
  							{
  								printf("error during file closing : %s\n",filename);
  							}
  							printf("fail during add name to list\n");
  							munmap(fileptr->msg, fileptr->memload);
  							sem_wait(&(otherfile->acces));
  							(otherfile->number)--;
  							sem_post(&(otherfile->acces));
  							pthread_exit(NULL);
  						}
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
					printf("error during file closing : %s\n",filename);
				}
				munmap(fileptr->msg, fileptr->memload);
				sem_wait(&(otherfile->acces));
				(otherfile->number)--;
				sem_post(&(otherfile->acces));
				pthread_exit(NULL);

			}
		}
		if(close(file)==-1)
		{
			printf("error during file closing : %s\n",filename);
		}
		munmap(fileptr->msg, fileptr->memload);
		sem_wait(&(otherfile->acces));
		(otherfile->number)--;
		sem_post(&(otherfile->acces));
		pthread_exit(NULL);
	}
 }


/*
 * @pre line != NULL
 * @post Cree une structure fractal dont les parametres sont contenu dans la chaine de caracter "line"
 * ERREUR retour NULL si line ne contient pas les informations correspondant a une fractal
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
     if(a>= -1.0 && a <= 1.0 && b>= -1.0 && b <= 1.0)
     {
       if (width != 0 && height != 0)
       {
         struct fractal* newfract = fractal_new(name, width, height, a, b);
         return newfract;
       }
       return NULL;
     }
     return NULL;
   }
   else
   {
     return NULL; //invalide line
   }
}

/*
 * @pre name != NULL &&  list != NULL
 * @post Verifie si le nom de la fractal se trouve dans la liste chainee
 *       0 = n'existe pas encore
 *      -1 = existe déjà
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
 * @pre name != NULL && list != NULL
 * @post le nom a ete ajoute a la fin de la liste chainee
 */
int addtolistname(char* name, struct nameacceslist *list)
{
  if(list == NULL)
  {
    return -1;
  }
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
 * @pre name != NULL && list != NULL
 * @post Le noeud correspondant au nom a ete retire de la liste chainee si elle s'y trouvait
 * Retour 0 si supprime correctement
 *       -1 en cas d'erreur ou si le nom ne s'y trouvait pas
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
        free(current->name);
        free(current);
        list->head = NULL;
        sem_post(&(list->acces));
        return 0;
    }
    else
    {
      struct name *suivant = current->next;
      free(current->name);
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
        free(current->next->name);
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
 * @pre list != NULL
 * @post Supprime tous les noeuds de la liste chainee
 */
void freelistname(struct nameacceslist *list)
{
  struct name *head = list->head;
  if(head!=NULL)
  {
    struct name *current = head;
    struct name *suivant = head->next;
    while(current->next != NULL)
    {
      //printf("%s supprimé ... \n",current->name);
      free(current->name);
      free(current);
      current = suivant;
      suivant = current->next;
    }
    //printf("%s supprimé ... \n",current->name);
    free(current->name);
    free(current);
  }
  sem_destroy(&(list->acces));
  free(list);
}

/*
 * @pre buff!=NULL, n>0
 * @post a construit un buffer partagé contenant n slots
 * Return  : 0 = la création du buffer à été éffectuée avec succès.
 *          -1 = erreur lors d'une initialisation à une variable.
 * Une erreur est affiché en cas d'erreur
 */
int buf_init(struct buff *buf, int n)
{
    int err;
    buf->buf = (struct fractal **)malloc(sizeof(struct fractal*)*n); //cree un tableau de n slot
    if((buf->buf)==NULL)
    {
       printf("error during buf image creation\n");
       return -1;
    }
    buf->length = n;                       /* Buffer content les entiers */
    buf->begin = buf->end = 0;        /* Buffer vide si front == rear */
    err = sem_init(&(buf->empty), 0, n);      /* Au debut, n slots vides */
    if(err !=0)
    {
      printf("error during semaphore creation\n");
      free(buf->buf);
      return -1;
    }
    err = sem_init(&(buf->full), 0, 0);      /* Au debut, rien a consommer */
    if(err !=0)
    {
      printf("error during semaphore creation\n");
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
    printf(" Buffer is empty  == -1 sinon 0 :%d \n",ret); //Permet de vérifier si le buffer est vide ou non
    while(ret == 0 && f != NULL)
    {
      printf("Il reste une fractal : %s \n",fractal_get_name(f)); //affiche les fractals n'ayant pas été supprimée
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
 * Retourne une fractal non null
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
 * @pre buffer != NULL && f != NULL
 * @post Verifie si il y a des elements dans le Buffer"buffer"
 * Si oui, il prends le premier element et ajoute le pointeur dans f et retourne 0
 * Si non,  initialise *f à NULL et retourne -1
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
 * @pre f != NULL
 * @post initialise f->value à la valeur -1
 */
void setendofprogram(struct programend *f)
{
  sem_wait(&(f->acces));
  f->value = -1;
  sem_post(&(f->acces));
}

/*
 * @pre f != NULL
 * @post retourne la valeur stocké dans la structure f
 */
int isendofprogram(struct programend *f)
{
  sem_wait(&(f->acces));
  int val = f->value;
  sem_post(&(f->acces));
  if (val != -1)
  {
	  return 0;
  }
  return val;
}

/*
 * @pre buffer != NULL && f!=NULL
 * @post Cette fonction verifie plusieurs condition pour determiner si les producteurs doivent ou non arreter d'être créé.
 * 1) Verifie si il reste des fractals dans le Buffer "buffer". Si oui la varables **f stocke la structure
 * 2) Verifie si il reste encore des threads de lecture actifs.
 * Retour =
 *   - 0 si une(ou les deux) des deux conditions est vraie
 *   - -1 si les deux conditions sont fausses
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
    //printf("Fin du programme pour les producteurs \n");
    return -1;
  }
}
/*
 * @pre buffer != NULL && f!=NULL
 * @post Cette fonction verifie plusieurs condition pour determiner si les consommateurs doivent ou non arreter d'être créé.
 * 1) Verifie si il reste des fractals dans le Buffer "buffer". Si oui la varables **f stocke la structure
 * 2) Verifie le signal d'arrêt des producteurs
 * 3) Verifie si il y a toujours des producteurs actifs
 * Retour =
 *   - 0 si une(o pluisuers) des troisconditions est vraie
 *   - -1 si les trois conditions sont fausses
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
    //printf("Fin du programme pour les consommateurs \n");
    pthread_mutex_unlock(&verification);
    return -1;
  }
}


/*
 * @pre f != NULL && frac != NULL && buffer != NULL && (type == 1 || type == 2)
 * @post Cette fonction permet de stocker les fractals de plus haute moyenne.
 * Si la moyenne de "frac" est égale à la fractal déjà présente dans la liste chainée "f", alors elle l'ajoute à cette Liste
 * Si fractalHigh ne contient pas encore de fractal, alors ajoute la fractal à la liste chainée "f" et change la valeur de f->average
 * Si la moyenne de la fractal est inférieur àa la moyenne déjà présente deux options sont possibles :
 *      - type == 1 alors la fractal est ajouté au buffer "buffer"
 *      - type == 2 alors la fractal est supprimée et sa mémoire libèrée
 * Si la moyenne de la nouvelle fractal "frac" est supérieur à la valeur moyenne de la fractal déjà présente alors deux options sont possibles :
 *      - type == 1, alors toutes les fractals présentes dans la liste chainée "f" sont retirées de cette liste et insèrée dans le buffer "buffer" et la nouvelle fractal est insèrée dans
 *      la liste chainée
 *      - type == 2, les fractals sont retirées de la liste chainée et supprimée (mémoire free) et la nouvelle fractal est insèrée dans la liste chainée.
 * Retour : 0 = l'opération s'est correctement éffectuée
 *         -1 = echec
 * Dans le cas d'une erreur, une erreur est affichée
 */
int HighAverageModify(struct listfractalhigh *f, struct fractal *frac, int average, struct buff *buffer,int type)
{
  if(f == NULL)
  {
    printf("listfractalhigh isn't initialize \n");
    return -1;
  }
  int err = 0;
  sem_wait(&(f->acces));
  int moyenne = f->average;
  if(average == moyenne) //il faut juste ajouter la fractal à la listfractalhigh
  {
    err = addtolistfractalhigh(f,frac);
    if(err == -1)
    {
      sem_post(&(f->acces));
      return -1;
    }
    sem_post(&(f->acces));
    return 0;
  }
  else if(average < moyenne) //Directement bitmapper la fractal
  {
    if(type == BITMAP_ALL)
    {
      buf_insert(buffer, frac);
    }
    else
    {
      fractal_free(frac);
    }
    sem_post(&(f->acces));
    return 0;
  }
  else // il faut vider toute la liste et bitmapper les fractals qui s'y trouvait et ensuite insèrer la nouvelle fractal dans la listfractalhigh
  {
    f->average = average;
      err = clean_listfractalhigh(f,buffer, type);
      if(err != 0)
      {
        sem_post(&(f->acces));
        return -1;
      }
      err = addtolistfractalhigh(f,frac);
      sem_post(&(f->acces));
      return 0;
  }
}

/*
 * @pre f != NULL && frac != NULL
 * @post Cette fonction ajoute la fractal "frac" à la fin de la liste chainée "f"
 * Retour : 0 = Opération effectuée avec succès.
 *         -1 = echec
 * En cas d'erreur, une erreur est affichée
 */
int addtolistfractalhigh(struct listfractalhigh *f, struct fractal *frac)
{
  if(f == NULL)
  {
    printf("Erreur listfractalhigh isn't initialize \n");
    return -1;
  }
  struct nodefractal *current= f->head;
  if(current == NULL)
  {
    struct nodefractal *new = (struct nodefractal*)malloc(sizeof(struct nodefractal));
    if(new == NULL)
    {
      printf("Erreur malloc fractal in listfractalhigh \n");
      return -1;
    }
    new->fract = frac;
    new->next = NULL;
    f->head = new;
    return 0;
  }
  else if (current->next == NULL) {
    struct nodefractal *new = (struct nodefractal*)malloc(sizeof(struct nodefractal));
    if(new == NULL)
    {
      printf("Erreur malloc fractal in listfractalhigh \n");
      return -1;
    }
    current->next = new;
    new->fract = frac;
    new->next = NULL;
    return 0;
  }
  else
  {
    while(current->next != NULL)
    {
      current = current->next;
    }
    struct nodefractal *new = (struct nodefractal*)malloc(sizeof(struct nodefractal));
    if(new == NULL)
    {
      printf("Erreur malloc fractal in listfractalhigh \n");
      return -1;
    }
    current->next = new;
    new->fract = frac;
    new->next = NULL;
    return 0;
  }
}

/*
 * @pre f != NULL && buffer != NULL && (type == 1 || type == 2)
 * @post Deux cas se présente :
 *       - type == 1, alors toutes les fractals présentes dans la liste chainée "f" sont retirées de cette liste et insèrée dans le buffer "buffer" et la nouvelle fractal est insèrée dans
 *      la liste chainée
 *       - type == 2, les fractals sont retirées de la liste chainée et supprimée (mémoire free) et la nouvelle fractal est insèrée dans la liste chainée.
 * Retour : - 0 = opération réussie
            - -1 = echec
 * En cas d'echec, une erreur est affichée
 */
int clean_listfractalhigh(struct listfractalhigh *f, struct buff *buffer, int type)
{
  if(f == NULL)
  {
    printf("Error listfractalhigh isn't initialize \n");
    return -1;
  }

  struct nodefractal *current = f->head;
  struct nodefractal *suivant = NULL;
  if(type == BITMAP_ALL) //type == 1
  {
      if(buffer == NULL)
      {
        printf("Error buffer isn't initialize \n");
        return -1;
      }
      while(current != NULL)
      {
        suivant = current->next;
        buf_insert(buffer, current->fract);
        free(current);
        current = suivant;
      }
      f->head = NULL;
  }
  else // type == 2
  {
    while(current != NULL)
    {
      suivant = current->next;
      fractal_free(current->fract);
      free(current);
      current = suivant;
    }
    f->head = NULL;
  }
  return 0;
}

/*
 * @pre f != NULL
 * @post Cette fonction permet de bitmapper toutes les fractals présentent dans la liste chainée "f"
 * Retour : - 0 = opération réussie
            - -1 = echec
 * En cas d'echec, une erreur est affichée
 */
int bitmapallfractalhigh(struct listfractalhigh *f)
{
  printf("ENTER BITMAP ALL FRACTALHIGH \n");
  int err = 0;
  int number = 0;
  sem_wait(&(f->acces));
  if(f == NULL)
  {
    printf("Error listfractalhigh isn't initialize \n");
    sem_post(&(f->acces));
    return -1;
  }
  struct nodefractal *current = f->head;
  if(current == NULL)
  {
    printf("No fractalHigh \n");
  }
  struct nodefractal *suivant = NULL;
  while(current != NULL)
  {
      suivant = current->next;
      printf("Le nom de la fractal est : %s \n",fractal_get_name(current->fract));
      char name[64+6+1];
      strcpy(name, "fractout_");
      strcat(name, fractal_get_name(current->fract));
      err = write_bitmap_sdl(current->fract, name);
      if(err != 0)
      {
        printf("Error with write bitmap function\n");
        f->head = current; //ne pas perdre la tête de la liste
        sem_post(&(f->acces));
        return -1;
      }
      fractal_free(current->fract);
      free(current);
      current = suivant;
      number++;
      f->head = current; //Permet de ne pas perdre la tête si il y a une erreur dans write_bitmap_sdl
  }
  sem_post(&(f->acces));
  return 0;
}

/*
 * @pre f != NULL
 * @post Cette fonction permet de vider la mémoire utiliser par "f" ainsi que de vérifier si il ne reste pas de fractal dedans. Dans le cas ou il en reste,
 * elles sont supprimées.
 */
void deleteallfractalhigh(struct listfractalhigh *f)
{
  int etat = 0;
  sem_wait(&(f->acces));
  if(f == NULL)
  {
    printf("Error listfractalhigh isn't initialize \n");
    sem_post(&(f->acces));
    return;
  }
  struct nodefractal *current = f->head;
  struct nodefractal *suivant = NULL;
  while(current != NULL)
  {
      etat = -2;
      suivant = current->next;
      fractal_free(current->fract);
      free(current);
      current = suivant;
  }
  sem_post(&(f->acces));
  sem_destroy(&(f->acces));
  free(f);
  if(etat == -2)
  {
    printf("listhigh is not empty \n");
  }
}

/*
 * @pre list != NULL
 * @post Supprime tout les noeuds present dans la liste chainée "list"
 */
void listthread_free(struct listthread *list)
{
  if(list == NULL)
  {
    return;
  }
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
	//printf("list = NULL\n");
    return NULL;
  }
  struct thread *head = list->head;
  if(head == NULL)
  {
	   //printf("head = NULL \n");
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
        printf("pthread_create fail\n");
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
        printf("pthread_create fail\n");
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
    for(int i=0;((i<max_thread || max_thread < 0) && i < 15 && (isendofprogram(endoflecture)==0) && (isendofprogram(end) == 0));i++) {
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
              printf("pthread end with error\n");
              setendofprogram(end);
              return -1;
          }
        }
    }
    return 0;
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
      if(j<15 && j<numberproducteur+1 && (isendofprogram(endofproducteur)==0) && (isendofprogram(end)==0) || j<1)
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
              printf("pthread end with error\n");
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
              printf("pthread end with error\n");
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
  int sum;
  while((isendofprogram(endoflecture) == 0) && (isendofprogram(end)== 0) && (verify_end(listfractal,&f) == 0))
  {
    if(f != NULL)
    {
        int val;
        sum = 0;
        for(int a=0; (a<(fractal_get_width(f)*fractal_get_height(f)));a++)
        {
            int x = a % (fractal_get_width(f));
            int y = a/(fractal_get_width(f));
            val = fractal_compute_value(f, x, y);
            fractal_set_value(f,x,y,val);
            sum = sum + val;
        }
        if(isendofprogram(end)!= 0)
        {
          fractal_free(f);
          pthread_exit(NULL);
        }
        sum = sum / (fractal_get_width(f))*(fractal_get_height(f));
        int retour = HighAverageModify(listhigh, f, sum, buffer,BITMAP_ALL);
        if(retour != 0)
        {
           setendofprogram(end);
           pthread_exit(NULL);
        }
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
          //removetolistname(fractal_get_name(f), accesname);
          fractal_free(f);
          pthread_exit(NULL);
        }
        sum = sum / (fractal_get_width(f))*(fractal_get_height(f));
        //printf("Une fractal terminee avec succes : %s \n",fractal_get_name(f));
        int retour = HighAverageModify(listhigh, f, sum, NULL,BITMAP_AVERAGE);
        if(retour != 0)
        {
           setendofprogram(end);
           pthread_exit(NULL);
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
     //printf("une fractal bitmaper de nom : %s \n",fractal_get_name(fract));
     write_bitmap_sdl(fract, fractal_get_name(fract));
     //removetolistname(fractal_get_name(fract), accesname);
     fractal_free(fract);
     fract = NULL;
   }
   else
   {
   }
 }
 pthread_exit(NULL);
}

/*	la fonction refresh sert a (re)charger le fichier/morceaux de fichier en mémoire, dépandant de le taille du fichier
 *	ceci est fait en modifiant certaine partie de la structure fileinfo
 *
 *	cela n'accepte que des structure fileinfo
 *
 *	retourne un entier,
 *	return 0 appel Réussi
 *	return -1 en cas d'erreur
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


/*	cette fonction cherche pour le premier retour de ligne apres le pointeur readhead et déplace le pointeur juste apres(ligne suivante)
 *
 *	elle n'accepte que des structure de type fileinfo
 *
 *	elle retourne un entier
 *	return 0 tout c'est bien passé
 *	return 1 on est arrivé en fin de fichier
 *	return -1 une erreur c'est produite
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

/*	cette fonction insere le fichier en memoire pour le lire, ensuite il renvoit la premiere ligne valide
 *
 *	elle prend comme arguments une structure de type fileinfo, un pointeur vers une liste de char et sa longeur(liste)
 *
 *	elle renvoit un entier en fonction de cas :
 *	0 tout vas bien on peut récuperer ce qu'il se trouve dans le buffer et rappeler la fonction
 *  -1 erreur, généralement critique, il faut arreter le thread appelant
 *  1 nous somme arrivé a la fin du fichier sans trouver de ligne valide, donc ignorer le buffer
 *  2 nous somme arrivé a la fin du fichier en trouvant une ligne valide, on peut donc uttiliser le buffer et arreter le thread appelant
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
				//printf("return code 1\n");
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
				//printf("return code 2\n");
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
