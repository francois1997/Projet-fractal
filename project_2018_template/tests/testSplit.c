#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <CUnit/CUnit.h>



struct phrase {
  char *name;
  int width;
  int height;
  float a;
  float b;
};



struct phrase * split(char* line)
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
     const char* name2 = *splitedline;
     int width = atoi(*(splitedline+1));
     int height = atoi(*(splitedline+2));
     float a = atof(*(splitedline+3));
     float b = atof(*(splitedline+4));

     if (width != 0 && height != 0)
     {

       struct phrase *ret = (struct phrase*)malloc(sizeof(struct phrase));
       if (ret == NULL)
       {
         return NULL;
       }
       char *name = (char*)malloc((strlen(name2)+1)*sizeof(char));
       if (name == NULL)
       {
         free(ret);
         return NULL;
       }
       strcpy(name, name2);

       ret->name = name;
       ret->width = width;
       ret->height = height;
       ret->a = a;
       ret->b = b;

       return ret;
     }
     else
     {
       return NULL;
     }
   }
}


void test_split() {
  char *c1 = "fract1 1200 1200 0.2 0.2\n";
  char *c2 = "this line should fail\n";
  char *c3 = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n";
  char *c4 = "             \n";

  struct phrase *tet = split(c1);
  CU_ASSERT_EQUAL(tet->name, "fract1");
  CU_ASSERT_EQUAL(tet->width, 1200);
  CU_ASSERT_EQUAL(tet->height, 1200);
  CU_ASSERT_EQUAL(tet->a, 0.2);
  CU_ASSERT_EQUAL(tet->b, 0.2);
  free(tet->name);
  free(tet);
  tet = NULL;
  tet = split(c2);
  CU_ASSERT_PTR_NULL(tet);
  tet = split(c3);
  CU_ASSERT_PTR_NULL(tet);
  tet = split(c4);
  CU_ASSERT_PTR_NULL(tet);



}
