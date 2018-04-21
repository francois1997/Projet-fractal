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
  printf("%s",argv[begin]);
  void * ret = lecture((void*)(argv[begin]));
  //printf("%d",*(int *)(ret));
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
            //buf_insert(listfractal, newfract);
            if(err != 0)
            {
              if(close(file)==-1)
              {
                      printf("error during file closing : %s",filename);
              }
              printf("fail during add name to list");
              return (void*)-1;
            }
            //}
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
