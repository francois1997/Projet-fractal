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

void printallname(struct nameacceslist *list);







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





void printallname(struct nameacceslist *list)
{
  sem_wait(&(list->acces));
  struct name *current = list->head;
    while(current != NULL)
    {
      printf("%s",current->name);
      printf("1 \n");
      current = (current->next);
    }
    sem_post(&(list->acces));
}





void* lecture2(void * parametre)
{
  char* filename = (char*)parametre;
  int file = open(filename,O_RDONLY);
  char caractere = ' ';
  int etat = read(file,(void*)&caractere,sizeof(char));
  printf("%c",caractere);
  close(file);
  printf("%s",filename);
}

int readfile(int argc, char *argv[],int begin)
{
  int err = 0;
  printf("%s",argv[begin]);
  accesname = (struct nameacceslist *)malloc(sizeof(struct nameacceslist));
  if(accesname == NULL)
  {
    return -1;
  }
  accesname->head = NULL;
  err = sem_init(&(accesname->acces), 0, 1);
  if(err == -1){
    return -1;
  }
  void * ret = lecture((void*)(argv[begin]));
  printallname(accesname);
  //printf("%d",*(int *)(ret));
}

//MALLOC : fractal + fermer fichier file
void* lecture(void* parametre)
{
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
      if((*ligne != '#') && (*ligne != '\n')) //il ne s'agit pas d'une ligne de commentaire ni d'une ligne vide
      {
          struct fractal *newfract = split(ligne);
          //if(newfract == NULL)
          //{
          //  if(close(file)==-1)
          //  {
          //          printf("error during file closing : %s",filename);
          //  }
          //  printf("fail during fractal build");
          //  return (void*)-1;
          //}
          //else
          //{
            //buf_insert(listfractal, newfract);
            if(verifyduplicatename("hello",accesname)==0)
            {
              //err = addtolistname(newfract->name,accesname);
              err = addtolistname("hello",accesname);
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
            //}
          //}
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
     const char* name = *splitedline;
     int width = atoi(*(splitedline+1));
     int height = atoi(*(splitedline+2));
     double a = atof(*(splitedline+3));
     double b = atof(*(splitedline+4));
     struct fractal *newfract;// = fractal_new(name, width, height, a, b);
     //struct fractal* newfract = fractal_new(name, width, height, a, b);
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
