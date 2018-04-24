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
#include<stdint.h>

//#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <SDL/SDL.h>



void printallname(struct nameacceslist *list);


int max_thread;
struct buff *buffer;
struct buff *listfractal;
struct nameacceslist *accesname;
struct numberlecteur *otherfile;

int main(int argc, char *argv[])
{
    int err;
    if(argc <=1)
    {
          //error(err,"not enouth arguments");
          return -1;
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
            err = thread_all();
            if(err == -1)
            {
                printf("error during thread calculating");
                return -1;
            }
            return 0;
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
            err = thread_all();
            if(err == -1)
            {
                printf("error during thread calculating");
                return -1;
            }
            return 0;
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
            err = thread_moyenne();
            if(err == -1)
            {
                printf("error during thread calculating");
                return -1;
            }
            return 0;
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
            err = thread_moyenne();
            if(err == -1)
            {
                printf("error during thread calculating");
                return -1;
            }
           return 0;
        }
    }
    return -1;
}

//int executefunction()
//{
//  pthread_t lecture;
//  pthread_t producteur;
//  err=pthread_create(&lecture,NULL,(void *)&producermoyenne,NULL);
//if(err!=0)
//  {
//    printf("pthread_create fail");
//    return -1;
//}
//err=pthread_create(&producteur,NULL,(void *)&producermoyenne,NULL);
//if(err!=0)
//{
//    printf("pthread_create fail");
//    return -1;
//}
//}






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


//MALLOC : listname,listfractal(buffer), semaphore acces,
int readfile(int argc, char *argv[], int begin)
{
  listfractal = (struct buff *)malloc(sizeof(struct buff));
  if(listfractal == NULL)
  {
    printf("error during buffer creation");
    return -1;
  }
  int err = buf_init(listfractal, 100); //quelle taille ?
  if(err != 0)
  {
    free(listfractal);
    return -1;
  }

  accesname = (struct nameacceslist *)malloc(sizeof(struct nameacceslist));
  if(accesname == NULL)
  {
    buf_clean(listfractal);
    free(listfractal);
    return -1;
  }
  accesname->head = NULL;
  err = sem_init(&(accesname->acces), 0, 1);
  if(err == -1){
    buf_clean(listfractal);
    free(listfractal);
    freelistname(accesname);
    return -1;
  }
  pthread_t lecteur[argc-begin];
  otherfile = (struct numberlecteur * )malloc(sizeof(struct numberlecteur));
  otherfile->number = 0;
  err = sem_init(&(otherfile->acces), 0, 1);
  if(err == -1){
    buf_clean(listfractal);
    free(listfractal);
    freelistname(accesname);
    free(otherfile);
    return -1;
  }
  for(int i = begin; i < argc;i++)
  {
    char * filename = *(argv+i);
    err = pthread_create(&lecteur[i],NULL,(void *)&lecture,(void *)(*(argv+i)));
    printf("%s thread creare \n",filename);
    if(err != 0)
    {
      buf_clean(listfractal);
      free(listfractal);
      sem_destroy(&(accesname->acces));
      freelistname(accesname);
      free(otherfile);
      return -1;
    }
    sem_wait(&(otherfile->acces));
    (otherfile->number)++;
    sem_post(&(otherfile->acces));
  }
  for(int i = begin; i < argc;i++)
  {
    err = pthread_join(lecteur[i], NULL);
    sem_wait(&(otherfile->acces));
    (otherfile->number)--;
    sem_post(&(otherfile->acces));
    if(err!=0)
    {
        buf_clean(listfractal);
        free(listfractal);
        sem_destroy(&(accesname->acces));
        freelistname(accesname);
        free(otherfile);
        printf("lecture pthread end with error");
        return -1;
    }
  }
  return 0;
}


//MALLOC : fermer fichier file
void * lecture(void* parametre)
{
  printf("%s",(char *)parametre);
  int err=0;
  char* filename = (char *)parametre;
  int file = open(filename,O_RDONLY);
  if(file==-1)
  {
    printf("error during file opening : %s",filename);
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
                  pthread_exit(NULL);
          }
          if(etat < 0)
          {
            printf("error during file reading : %s",filename);
          }
          pthread_exit(NULL);
        }
        printf("%c",caractere);
        *(ligne+i) = caractere;
        i++;
      }
      printf("\n");
      if((*ligne != '#') && (*ligne != '\n')) //il ne s'agit pas d'une ligne de commentaire ni d'une ligne vide
      {
          struct fractal *newfract = split(ligne);
          if(newfract == NULL)
          {
            if(close(file)==-1)
            {
                   printf("error during file closing : %s",filename);
                   pthread_exit(NULL);
            }
            printf("fail during fractal build");
            pthread_exit(NULL);
          }
          else
          {
            buf_insert(listfractal, newfract);
            if(verifyduplicatename(newfract->name,accesname)==0)
            {
              err = addtolistname(newfract->name,accesname);
              //err = addtolistname("hello",accesname);
              if(err != 0)
              {
                if(close(file)==-1)
                {
                        printf("error during file closing : %s",filename);
                        pthread_exit(NULL);
                }
                printf("fail during add name to list");
                pthread_exit(NULL);
              }
            }
          }
      }
    }
    if(close(file)==-1)
    {
            printf("error during file closing : %s",filename);
            pthread_exit(NULL);
    }
    pthread_exit(NULL);
  }
}



//Malloc: fractal
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
   while(*(line+i) != '\n' && numberarg < 6 && position<65)
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
     struct fractal* newfract = fractal_new(name, width, height, a, b);
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
      sem_post(&(list->acces));
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



int removetolistname(char* name, struct nameacceslist *list)
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
      free(current->name);
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
        free(current->next->name);
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



/*
 * @pre buffer!=NULL, n>0
 * @post a construit un buffer partagé contenant n slots
 * MALLOC : buf, (empty, full)->semaphore
 */
int buf_init(struct buff *buf, int n)
{
    printf("INIT ...");
    int err;
    buf->buf = (struct fractal **)malloc(sizeof(struct fractal*)*n);
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

int buf_isempty(struct buff *buffer)
{
  int err = 0;
  err = sem_trywait(&(buffer->full));
  if(err != 0)
  {
    return -1;
  }
  else
  {
    sem_post(&(buffer->full));
    return 0;
  }
}

int verify_end(struct buff *buffer)
{

  int etat = buf_isempty(buffer);
  sem_wait(&(otherfile->acces));
  int etat2 = otherfile->number;
  sem_post(&(otherfile->acces));
  if(etat != 0 || etat2 != 0)
  {
    return 0;
  }
  else
  {
    printf("Fin du programme");
    return -1;
  }
}


/* @pre sbuf!=NULL
 * @post retire le dernier item du buffer partag�
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


//MALLOC : buffer, buff_init
int thread_moyenne()
{
    int err;
    buffer = (struct buff*)malloc(sizeof(struct buff));
    if(buffer == NULL)
    {
      return -1;
    }
    if(max_thread==-1)
    {
      max_thread = 6;
    }
    printf("nombre thread max = %d \n",max_thread);
    printf("Creation buffer \n");
    err = buf_init(buffer, max_thread);
    if(err != 0)
    {
        printf("Error during buffer creation");
        free(buffer);
        return -1;
    }
    printf("Buffer made");
    pthread_t producteur[max_thread];
    pthread_t consommateur[max_thread];
    for(int i=0;i<max_thread;i++) {
        err=pthread_create(&(producteur[i]),NULL,(void *)&producermoyenne,NULL);
        if(err!=0)
        {
            printf("pthread_create fail");
            buf_clean(buffer);
        }
    }

    int max = INT_MIN;
    struct fractal *big = NULL;
    for(int i=0; i<max_thread; i++)
    {
        void **ret;
        printf("Thread Producteur avant récupèré");
        err = pthread_join(producteur[i], ret);
        printf("FAIT1 :) \n");
        if(err!=0)
        {
            printf("pthread end with error");
            //Faire quelque chose !!
            return -1;
        }
        printf("Thread Producteur récupèré");
        if(ret == NULL)
        {
          printf("No return Value \n");
          return -1;
        }
        if((*ret) == NULL)
        {
          printf("No fractal return \n");
          return -1;
        }
        struct fractalHigh *high = (struct fractalHigh *)(*ret);
        if(high == NULL)
        {
            printf("Error during reception producteurmoyenne");
            buf_clean(buffer);
            return -1;
        }
        if((high->average)>max)
        {
            max = high->average;
            if(big != NULL)
            {
              fractal_free(big);
            }
            big = high->high;
            free(high);
        }
        else
        {
          fractal_free(high->high);
          free(high);
        }
    }
    printf("FAIT :) \n");
    err = write_bitmap_sdl(big, fractal_get_name(big));
    if(err != 0)
    {
      printf("Error with write bitmap function");
      return -1;
    }
    if(big != NULL)
    {
      fractal_free(big);
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
    if(max_thread == -1)
    {
      max_thread = 3;
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
        err=pthread_create(&(producteur[i]),NULL,(void *)&producer,NULL);
        if(err!=0)
            printf("pthread_create fail");
    }
    for(int i=0;i<max_thread;i++) {
        err=pthread_create(&(consommateur[i]),NULL,(void *)&consumer,NULL);
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
          printf("THREAD ALL récupèré \n");
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
void *producer(void *parametre)
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
void *producermoyenne(void *parametre)
{
  printf("Producermoyenne \n");
  int item;
  int moyenne;
  int max = INT_MIN;
  struct fractal *fractalhigh;
  int d = 1;
  int sum = 0;
  while(d>0) //ATTENTION IL FAUDRAIT RETIRER EN MEME TEMPS
  {
    printf("2");
    struct fractal *f = buf_remove(listfractal);
    int val;
    sum = 0;
    printf("largeur = %d et longueur = %d",fractal_get_width(f),(fractal_get_height(f)));
    for(int a=0; a<(fractal_get_width(f))*(fractal_get_height(f));a++)
    {
      //printf("%d \n",a);
        int x = a % (f->width);
        int y = a/(f->width);
        val = fractal_compute_value(f, x, y);
        sum = sum + val;
        fractal_set_value(f,x,y,val);
    }
    sum = sum / (fractal_get_width(f))*(fractal_get_height(f));
    printf("calcul \n");
    if(sum>max)
    {
        printf("option 1 \n");
        max = sum;
        printf("la valeur max vaut : %d \n",max);
        if(fractalhigh == NULL)
        {
          fractal_free(fractalhigh);
        }
        fractalhigh = f;
    }
    else
    {
      printf("option2 \n");
        if(f!=NULL)
        {
          fractal_free(f);
        }
    }
    d--;
  }
  if(fractalhigh == NULL)
  {
    printf("No high Fractal");
    pthread_exit(NULL);
  }
  struct fractalHigh * high2 = (struct fractalHigh *)malloc(sizeof(struct fractalHigh));
  if(high2 == NULL)
  {
    if(fractalhigh!=NULL)
    {
      fractal_free(fractalhigh);
      pthread_exit(NULL);
    }
  }
  high2->average = sum;
  high2->high = fractalhigh;
  printf("Le nom de la fractal est : %s \n",fractal_get_name(fractalhigh));
  pthread_exit((void *)high2);

}



// Consommateur
void *consumer(void *parametre)
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
