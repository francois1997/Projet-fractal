#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "libfractal/fractal.h"
#include "libfractal/main.h"
#include <pthread.h>
#include <errno.h>
#include <semaphore.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>

struct buff{
    struct fractal **buf;          /* Buffer contenant toutes les fractals */
    int length;             /* Nombre de slots dans le buffer */
    int begin;         /* buf[(begin+1)%n] est le premier �l�ment */
    int end;          /* buf[end%n] est le dernier */
    pthread_mutex_t mutex;
    sem_t empty;       /* Nombre de places libres */
    sem_t full;       /* Nombre d'items dans le buffer */
};


int max_thread;
struct buff *buffer;


int thread_moyenne()
{
    int err;
    buffer = (struct buff*)malloc(sizeof(struct buff));
    err = buf_init(buffer, max_thread);
    if(err != 0)
    {
        printf("Error during buffer creation");
        buf_clean(buffer);
        return 1;
    }
    pthread_t producteur[max_thread];
    pthread_t consommateur[max_thread];
    for(int i=0;i<max_thread;i++) {
        err=pthread_create(&(producteur[i]),NULL,&producermoyenne(),NULL);
        if(err!=0)
            printf("pthread_create fail");
    }

    int max = INT_MIN;
    for(int i =0; i<max_thread; i++)
    {
        err = pthread_join(producteur[i], void **ret);
        if(err!=0)
        {
            printf("pthread end with error");
        }
        if((int)**ret > max)
        {
            max = (int)**ret;
        }
    }

    buf_clean(table);
    return 0;
}

int thread_all()
{
    /* **************************************************************
    ATTENTION MALLOC
    *****************************************************************/
    table = (struct fractal **)malloc(sizeof(struct fractal *)*max_thread);
    /* **************************************************************
    ******************************************************************/
    err = buf_init(table, max_thread);
    if(err != 0)
    {
        printf("Error during buffer creation");
        free(table);
        return 1;
    }
    pthread_t producteur[max_thread];
    pthread_t consommateur[max_thread];


    for(int i=0;i<max_thread;i++) {
        err=pthread_create(&(producteur[i]),NULL,&producer(),NULL);
        if(err!=0)
            printf("pthread_create fail");
    }
    for(int i=0;i<max_thread;i++) {
        err=pthread_create(&(consommateur[i]),NULL,&consumer(),NULL);
        if(err!=0)
            printf("pthread_create");
    }
    buf_clean(buffer);
    return 0;
}


/*
 * @pre sp!=NULL, n>0
 * @post a construit un buffer partag� contenant n slots
 */
void buf_init(struct buff *buffer, int n)
{
    buffer->buf = (struct fractal **)malloc(sizeof(struct fractal*)*n);
    buffer->length = n;                       /* Buffer content les entiers */
    buffer->begin = buffer->end = 0;        /* Buffer vide si front == rear */
    sem_init(&buffer->empty, 0, n);      /* Au d�but, n slots vides */
    sem_init(&buffer->full, 0, 0);      /* Au d�but, rien � consommer */
}



/*
 * @pre fract!=NULL
 * @post lib�re le buffer et la structure
 */
void buf_clean(struct buff *buffer)
{
    free(buffer->buf);
    free(buffer);
}

/* @pre buffer!=NULL
 * @post ajoute item � la fin du buffer partag�. Ce buffer est g�r�
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



// Producteur
void producer(void)
{
  while(true)
  {
        // Calculer toute la fractal avant de rendre la main  ?
        struct fractal *f = (struct fractal*)malloc(sizeof(struct fractal));
        for(int a=0; a<(fractal_get_width(f)*fractal_get_height(f));a++)
        {
            int x = a % (fractal_get_width(f));
            int y = a/(fractal_get_width(f));
            fractal_compute_value(f, x, y);
        }
        buf_insert(buffer, f);
  }
}

// Producteur
void producermoyenne(void)
{
  int item;
  int moyenne;
  int max = MIN_VALUE;
  struct fractal *fractalhigh;
  while(true)
  {
        // Calculer toute la fractal avant de rendre la main  ?
        struct fractal *f = (struct fractal*)malloc(sizeof(struct fractal));
        for(int a=0; a<taille;a++)
        {
            int x = a % (f->width);
            int y = a/(f->width);
            fractal_compute_value(f, x, y);
        }
        int sum;
        for(int i=0; i<taille;i++)
        {
            sum = sum + (f->image)+i;
            if(sum>max)
            {
                max = sum;
                fractalhigh = f;
            }
        }
  }
  return
}



// Consommateur
void consumer(void)
{
 int item;
 while(true)
 {

    // section critique
    struct fractal *fract;
    fract = buf_remove(buffer);

 }
}



int main(int argc, char *argv[])
{
    if(argc <=1)
    {
          //error(err,"not enouth arguments");
    }
    if(strcmp(argv[2],"-d"))
    {
        if(strcmp(argv[3],"--maxthreads"))
        {
            max_thread = atoi(argv[4]);
            if(max_thread < 1)
            {
                //error(err,"bad max_thread number");
            }
        }
        else
        {
            max_thread = -1; //Si pas de nombre maximum de thread
        }
    }
    else
    {
        if(strcmp(argv[2],"--maxthreads"))
        {
            max_thread = atoi(argv[3]);
            if(max_thread < 1)
            {
                //error(err,"bad max_thread number");
            }
        }
        else
        {
            max_thread = -1;
        }
    }
}
