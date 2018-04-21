#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
//#include "libfractal/fractal.h"
#include "libfractal/main.h"
#include <pthread.h>
#include <errno.h>
#include <semaphore.h>
#include <stdbool.h>
#include <limits.h>
#include<stdint.h>

void printallname(struct nameacceslist *list);






/////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
struct fractal {
  char *name;
	unsigned long width;
	unsigned long height;
	double a;
	double b;
	uint16_t *image;
};


struct fractal * fractal_new(const char *name, int width, int height, double a, double b);
void fractal_free(struct fractal *f);
const char *fractal_get_name(const struct fractal *f);
int fractal_get_value(const struct fractal *f, int x, int y);
void fractal_set_value(struct fractal *f, int x, int y, int val);
int fractal_get_width(const struct fractal *f);
int fractal_get_height(const struct fractal *f);
double fractal_get_a(const struct fractal *f);
double fractal_get_b(const struct fractal *f);
size_t strlen(const char *string);
int fractal_compute_value(struct fractal *f, int x, int y);
int write_bitmap_sdl(const struct fractal *f, const char *fname);
void strcpy(char *dest, const char *src);


struct fractal *fractal_new(const char *name, int width, int height, double a, double b)
{
  struct fractal *newFract = (struct fractal*)malloc(sizeof(struct fractal)); //allocate memory for fractal structure
	if (newFract == NULL)
		return NULL;

	newFract->name = (char *)malloc(strlen(name)+1);							//allocate memory for fractal name
	if (newFract->name == NULL)
	{
		free(newFract);
		return NULL;
	}
	strcpy(newFract->name, name);

	newFract->image = (uint16_t *)malloc(sizeof(uint16_t)*height*width);		//allocate memory for the whole image
	if ((newFract->image) == NULL)
	{
		free(newFract->image);
		free(newFract->name);
		free(newFract);
		return NULL;
	}


	newFract->width = width;
	newFract->height = height;
	newFract->a = a;
	newFract->b = b;

    return newFract;
}

void fractal_free(struct fractal *f)
{
  free(f->name);
	free(f->image);
	free(f);
}

const char *fractal_get_name(const struct fractal *f)
{
    return (const char *)(f->name);
}

int fractal_get_value(const struct fractal *f, int x, int y)
{
    if(x > f->width || y > f->height)
		return -1;


    return *((f->image) + x + (f->width)*y);
}

void fractal_set_value(struct fractal *f, int x, int y, int val)
{
    if(x > f->width || y > f->height)
	{
		printf("SEG_fault : set_value");
		exit(-1);
	}
	*((f->image) + x + (f->width)*y) = val;
}

int fractal_get_width(const struct fractal *f)
{
    return f->width;
}

int fractal_get_height(const struct fractal *f)
{
    return f->height;
}

double fractal_get_a(const struct fractal *f)
{
    return f->a;
}

double fractal_get_b(const struct fractal *f)
{
    return f->b;
}

size_t strlen(const char *string)
{
	if (string == NULL)
		return -1;

	int i =0;
	while (*(string + i) != '\0' && (i <= 65))
		i++;

	if (i == 66)
		return -1;

	return (size_t)i;
}

void strcpy(char *dest, const char *src)
{
	int i =0;
	while (*(src + i) != '\0')
	{
		*(dest + i) = *(src + i);
		i++;
	}
	*(dest + i) = '\0';
}
/////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////




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
      printf("%s \n",current->name);
      current = (current->next);
    }
    sem_post(&(list->acces));
}





int readfile(int argc, char *argv[],int begin)
{
  int err = 0;
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
  listfractal = (struct buff *)malloc(sizeof(struct buff));
  err = buf_init(listfractal, 100); //quelle taille ?
  if(err != 0)
  {
    return -1;
  }
  lecture((void*)(argv[begin]));
  printallname(accesname);
  struct fractal *rem = buf_remove(listfractal);
  printf("%s",fractal_get_name(rem));
  //printf("%d",*(int *)(ret));
}

//MALLOC : fermer fichier file
void lecture(void* parametre)
{
  printf("%s",(char *)parametre);
  int err=0;
  char* filename = (char *)parametre;
  int file = open(filename,O_RDONLY);
  if(file==-1)
  {
    printf("error during file opening : %s",filename);
    return;
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
                  return;
          }
          if(etat < 0)
          {
            printf("error during file reading : %s",filename);
          }
          return;
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
                   return;
            }
            printf("fail during fractal build");
            return;
          }
          else
          {
            buf_insert(listfractal, newfract);
            if(verifyduplicatename("hello",accesname)==0)
            {
              err = addtolistname(newfract->name,accesname);
              //err = addtolistname("hello",accesname);
              if(err != 0)
              {
                if(close(file)==-1)
                {
                        printf("error during file closing : %s",filename);
                        return;
                }
                printf("fail during add name to list");
                return;
              }
            }
          }
      }
    }
    if(close(file)==-1)
    {
            printf("error during file closing : %s",filename);
            return;
    }
    return;
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




int strcmp(char* first, char* second)
{
  int i = 0;
  if(strlen(first) != strlen(second))
  {
    return -1;
  }
  while(i<=65 && i<strlen(first))
  {
     if(*(first+i) != *(second+i))
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
      free(buffer->buf);
      return -1;
    }
    err = sem_init(&(buffer->full), 0, 0);      /* Au debut, rien a consommer */
    if(err !=0)
    {
      printf("error during semaphore creation");
      free(buffer->buf);
      sem_destroy(&(buffer->empty));
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
