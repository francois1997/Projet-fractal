#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "libfractal/fractal.h"
#include "libfractal/main.h"
#include <pthread.h>
#include <errno.h>
#include <semaphore.h>
#include <stdbool.h>
#include <limits.h>




int max_thread;
struct buff *buffer;
struct buff *listfractal;
struct nameacceslist *accesname;
int main(int argc, char *argv[])
{
    int err;
    if(argc <=1)
    {
          //error(err,"not enouth arguments");
    }
    if(strcmp(argv[1],"-d")==0)
    {
        if(strcmp(argv[2],"--maxthreads")==0)
        {
            max_thread = atoi(argv[3]);
            if(max_thread < 1)
            {
                printf("bad max_thread number");
                //error(err,"bad max_thread number");
                return -1;
            }
            err = readfile(argc,argv,4);
            if(err == -1)
            {
                printf("error during read file");
                return -1;
            }
        }
        else
        {
            max_thread = -1; //Si pas de nombre maximum de thread
            err = readfile(argc,argv,2);
            if(err == -1)
            {
                printf("error during read file");
                return -1;
            }
        }
    }
    else
    {
        if(strcmp(argv[1],"--maxthreads")==0)
        {
            max_thread = atoi(argv[2]);
            if(max_thread < 1)
            {
                printf("bad max thread number");
                //error(err,"bad max_thread number");
                return -1;
            }
            err = readfile(argc,argv,3);
            if(err == -1)
            {
                printf("error during read file");
                return -1;
            }
        }
        else
        {
            max_thread = -1;
            err = readfile(argc,argv,1);
            if(err == -1)
            {
                printf("error during read file");
                return -1;
            }
        }
    }
}


//MALLOC : listname,listfractal(buffer), semaphore acces,
int readfile(int argc, char *argv[], int begin)
{
  int err = buf_init(listfractal, 100); //quelle taille ?
  if(err != 0)
  {
    return -1;
  }

  struct nameacceslist *listname = (struct nameacceslist *)malloc(sizeof(struct nameacceslist));
  if(listname == NULL)
  {
    buf_clean(listfractal);
    return -1;
  }
  listname->head = NULL;
  err = sem_init(&(listname->acces), 0, 1);
  if(err == -1){
    return -1;
  }
  pthread_t lecteur[argc-begin];
  int position = 0;
  for(int i = begin; i < argc;i++)
  {
    char * filename = *argv+i;
    err = pthread_create(&lecteur[position],NULL,&lecture,(void *)filename);
    if(err != 0)
    {
      buf_clean(listfractal);
      sem_destroy(&(listname->acces));
      freelistname(listname);
      return -1;
    }
  }
  return 0;
}

//MALLOC : fractal + fermer fichier file
void* lecture(void* parametre)
{
  printf("hello");
  int err;
  char* filename = (char *)parametre;
  int file = open(filename,O_RDONLY);
  if(file==-1)
  {
    if(close(file)==-1)
    {
            printf("error during file closing : %s",filename);
    }
    printf("error during file opening : %s",filename);
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
        printf("%c",caractere);
        if(etat == -1)
        {
          if(close(file)==-1)
          {
                  printf("error during file closing : %s",filename);
          }
          printf("error during file reading : %s",filename);
          return (void*)(-1);
        }
        *(ligne+i) = caractere;
        i++;
      }
      printf("\n");
      if(*ligne != '#' & *ligne != '\n') //il ne s'agit pas d'une ligne de commentaire ni d'une ligne vide
      {
          struct fractal *newfract = split(ligne);
          if(newfract == NULL)
          {
            if(close(file)==-1)
            {
                    printf("error during file closing : %s",filename);
            }
            printf("fail during fractal build");
            return (void*)-1;
          }
          else
          {
            buf_insert(listfractal, newfract);
            if(err != 0)
            {
              if(close(file)==-1)
              {
                      printf("error during file closing : %s",filename);
              }
              printf("fail during add name to list");
              return (void*)-1;
            }
            }
          }
      }
    }
    if(close(file)==-1)
    {
            printf("error during file closing : %s",filename);
            return (void *)(-1);
    }
  }
}

struct fractal * split(char* line)
{
   int i = 0;
   int position=0;
   int place=0;
   int numberarg=0;
   char *splitedline[5];
   char current[sizeof(char)*64];
   while(*(line+i) != '\n' && numberarg < 6)
   {
     if(*(line+i) == ' ')
     {
       *(current+position) = '\0';
       *(splitedline+place) = current;
       position = 0;
       place++;
       numberarg++;
     }
     else
     {
       *(current+position) = *(line+i);
     }
     i++;
   }
   if(numberarg==5)
   {
     char* name = *splitedline;
     int width = atoi(*(splitedline+1));
     int height = atoi(*(splitedline+2));
     double a = atof(*(splitedline+3));
     double b = atof(*(splitedline+4));
     struct fractal *newfract = fractal_new(name, width, height, a, b);
     return newfract;
   }
   else
   {
     return NULL; //invalide line
   }
}

int verifyduplicatename(char* name, struct nameacceslist *list)
{
  sem_wait(&(list->acces));
  struct name *current = list->head;
  while(current != NULL)
  {
    if(strcmp((name),current->name)==0)
    {
      return -1;
    }
    current = (current->next);
  }
  sem_post(&(list->acces));
  return 0;
}


// MALLOC : node,
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



int removetolistname(char* name, struct nameacceslist *list)
{
  sem_wait(&(list->acces));
  struct name *current = list->head;
  if(current == NULL)
  {
    sem_post(&(list->acces));
    return -1; //not found
  }
  if(strcmp(current->name,name)==0)
  {
    if(current->next == NULL)
    {
        free(name);
        free(head);
        list->head = NULL;
        sem_post(&(list->acces));
        return 0;
    }
    else
    {
      free(name);
      struct name *suivant = head->next;
      free(head);
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
        free(name);
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

void freelistname(struct nameacceslist *list)
{
  struct name *head = list->head;
  if(head!=NULL)
  {
    struct name *current = head;
    struct name *suivant = head->next;
    while(current->next != NULL)
    {
      free(current->name);
      free(current);
      current = suivant;
      suivant = current->next;
    }
    free(current->name);
    free(current);
  }
  free(list);
}

int strcmp(char* first, char* second)
{
  int i = 0;
  if(strlen(first) != strlen(second))
  {
    return -1;
  }
  while(i<=65)
  {
     if(*(first+i) !=*(second+i))
     {
       return -1;
     }
     i++;
  }
  return 0;
}
/*
 * @pre buffer!=NULL, n>0
 * @post a construit un buffer partagé contenant n slots
 * MALLOC : buf, (empty, full)->semaphore
 */
int buf_init(struct buff *buffer, int n)
{
    int err;
    buffer->buf = (struct fractal **)malloc(sizeof(struct fractal*)*n);
    if((buffer->buf)==NULL)
    {
       return -1;
    }
    buffer->length = n;                       /* Buffer content les entiers */
    buffer->begin = buffer->end = 0;        /* Buffer vide si front == rear */
    err = sem_init(&(buffer->empty), 0, n);      /* Au debut, n slots vides */
    if(err !=0)
    {
      printf("error during semaphore creation");
      free(buffer->buff);
      return -1;
    }
    err = sem_init(&(buffer->full), 0, 0);      /* Au debut, rien a consommer */
    if(err !=0)
    {
      printf("error during semaphore creation");
      free(buffer->buff);
      sem_destroy(&(buffer->empty)
      return -1;
    }
}



/*
 * @pre fract!=NULL
 * @post libere le buffer et la structure
 */
void buf_clean(struct buff *buffer)
{
    free(buffer->buf);
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


/* @pre sbuf!=NULL
 * @post retire le dernier item du buffer partag�
 */
struct fractal* buf_remove(struct buff *buffer)
{
    sem_wait(&(buffer->full));
    pthread_mutex_lock(&buffer->mutex);
    struct fractal *retour = ((buffer->buf)[(buffer->begin)+1]);
    buffer->begin = (buffer->begin + 1)%(buffer->length);
    pthread_mutex_unlock(&buffer->mutex);
    sem_post(&(buffer->empty));
    return retour;
}


//MALLOC : buffer, buff_init
int thread_moyenne()
{
    int err;
    buffer = (struct buff*)malloc(sizeof(struct buff));
    if(buffer == NULL)
    {
      return -1;
    }
    err = buf_init(buffer, max_thread);
    if(err != 0)
    {
        printf("Error during buffer creation");
        free(buffer);
        return -1;
    }
    pthread_t producteur[max_thread];
    pthread_t consommateur[max_thread];
    for(int i=0;i<max_thread;i++) {
        err=pthread_create(&(producteur[i]),NULL,&producermoyenne,NULL);
        if(err!=0)
        {
            printf("pthread_create fail");
            buf_clean(buffer);
        }
    }

    int max = INT_MIN;
    struct fractal *big;
    for(int i =0; i<max_thread; i++)
    {
        void **ret;
        err = pthread_join(producteur[i], ret);
        if(err!=0)
        {
            printf("pthread end with error$");
            //Faire quelque chose !!
        }
        struct  fractalHigh *high = (struct fractalHigh*)(*ret);
        if((high->average)>max)
        {
            max = high->average;
            fractal_free(big);
            big = high->high;
            free(high);
        }
        else
        {
          fractal_free(high->high);
          free(high);
        }
    }

    buf_clean(buffer);
    return 0;
}


//MALLOC : buffer, buff_init
int thread_all()
{
    buffer = (struct buff*)malloc(sizeof(struct buff));
    if(buffer == NULL)
    {
      return -1;
    }
    int err = buf_init(buffer, max_thread);
    if(err != 0)
    {
        printf("Error during buffer creation");
        free(buffer);
        return 1;
    }
    pthread_t producteur[max_thread];
    pthread_t consommateur[max_thread];


    for(int i=0;i<max_thread;i++) {
        err=pthread_create(&(producteur[i]),NULL,&producer,NULL);
        if(err!=0)
            printf("pthread_create fail");
    }
    for(int i=0;i<max_thread;i++) {
        err=pthread_create(&(consommateur[i]),NULL,&consumer,NULL);
        if(err!=0)
            printf("pthread_create");
    }
    for(int i =0; i<max_thread; i++)
    {
        err = pthread_join(producteur[i], NULL);
        if(err!=0)
        {
            printf("pthread end with error");
        }
    }
    for(int i =0; i<max_thread; i++)
    {
        err = pthread_join(consommateur[i], NULL);
        if(err!=0)
        {
            printf("pthread end with error");
        }
    }
    buf_clean(buffer);
    return 0;
}



// Producteur
void *producer(void)
{
  while(true)
  {
        struct fractal *f = buf_remove(listfractal);
        int val;
        for(int a=0; a<(fractal_get_width(f)*fractal_get_height(f));a++)
        {
            int x = a % (fractal_get_width(f));
            int y = a/(fractal_get_width(f));
            val = fractal_compute_value(f, x, y);
            fractal_set_value(f,x,y,val);
        }
        buf_insert(buffer, f);
  }
  pthread_exit(NULL);
}

// Producteur
void *producermoyenne(void)
{
  int item;
  int moyenne;
  int max = INT_MIN;
  struct fractal *fractalhigh;
  while(true)
  {
        struct fractal *f = buf_remove(listfractal);
        int val;
        int sum = 0;
        for(int a=0; a<(fractal_get_width(f))*(fractal_get_height(f));a++)
        {
            int x = a % (f->width);
            int y = a/(f->width);
            val = fractal_compute_value(f, x, y);
            sum = sum + val;
            fractal_set_value(f,x,y,val);
        }
        if(sum>max)
        {
            max = sum;
            fractal_free(fractalhigh);
            fractalhigh = f;
        }
        else
        {
            fractal_free(f);
        }
  }
  write_bitmap_sdl(fractalhigh, fractal_get_name(fractalhigh));
  pthread_exit(NULL);
}



// Consommateur
void *consumer(void)
{
 int item;
 while(true)
 {
    // section critique
    struct fractal *fract;
    fract = buf_remove(buffer);
    write_bitmap_sdl(fract, fractal_get_name(fract));
    fractal_free(fract);
 }
 pthread_exit(NULL);
}
