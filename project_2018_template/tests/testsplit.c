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
#include "CUnit/Basic.h"



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
     int width = -2;
     int height = -2;
     float a = -2;
     float b = -2;
     width = atoi(*(splitedline+1));
     height = atoi(*(splitedline+2));
     a = atof(*(splitedline+3));
     b = atof(*(splitedline+4));
     printf("name :%s, width : %d, height : %d, a : %f, b : %f\n",name2,width, height, a ,b );

     if (width > 0 && height > 0 && (a <= 1 && a >= -1) && (b <= 1 && b >= -1))
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
   return NULL;
}


void test_split() {
  char *c1 = "fract1 1200 1200 0.2 0.2\n";
  char *c2 = "this line should fail\n";
  char *c3 = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n";
  char *c4 = "             \n";

  struct phrase *tet = split(c1);
  CU_ASSERT_STRING_EQUAL(tet->name, "fract1");
  CU_ASSERT_EQUAL(tet->width, 1200);
  CU_ASSERT_EQUAL(tet->height, 1200);
  CU_ASSERT_DOUBLE_EQUAL(tet->a, 0.2, 3);
  CU_ASSERT_DOUBLE_EQUAL(tet->b, 0.2, 3);
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


/*
 *  ce code est repris du site :http://cunit.sourceforge.net/example.html
 *
 *
 */
int main()
{
   CU_pSuite pSuite = NULL;

   /* initialize the CUnit test registry */
   if (CUE_SUCCESS != CU_initialize_registry())
      return CU_get_error();

   /* add a suite to the registry */
   pSuite = CU_add_suite("Suite_1", NULL, NULL);
   if (NULL == pSuite) {
      CU_cleanup_registry();
      return CU_get_error();
   }

   /* add the tests to the suite */
   /* NOTE - ORDER IS IMPORTANT - MUST TEST fread() AFTER fprintf() */
   if ((NULL == CU_add_test(pSuite, "test of Split", test_split)))
   {
      CU_cleanup_registry();
      return CU_get_error();
   }

   /* Run all tests using the CUnit Basic interface */
   CU_basic_set_mode(CU_BRM_VERBOSE);
   CU_basic_run_tests();
   CU_cleanup_registry();
   return CU_get_error();
}
