#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <semaphore.h>
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

struct name{
  char* name;
  struct name *next;
};

struct nameacceslist{
  struct name *head;
  sem_t acces;
};

struct fractalHigh{
  int average;
  struct fractal *high;
};

struct numberlecteur{
  int number;
  sem_t acces;
};

struct fractal * split(char* line);
int addtolistname(char* name, struct nameacceslist *list);
void freelistname(struct nameacceslist *list);
void * lecture(void* parametre);
int readfile(int argc, char *argv[], int begin);
int verifyduplicatename(char* name, struct nameacceslist *list);
int thread_moyenne();
int thread_all();
int buf_init(struct buff *buffer, int n);
void buf_clean(struct buff *buffer);
void buf_insert(struct buff *buffer, struct fractal *item);
struct fractal* buf_remove(struct buff *buffer);
void *producer(void *parametre);
void *producermoyenne(void *parametre);
void *consumer(void *parametre);
int main(int argc, char *argv[]);
