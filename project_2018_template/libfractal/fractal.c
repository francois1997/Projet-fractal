#include <stdlib.h>
#include <stdio.h>
#include "fractal.h"

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
	stpcpy(newFract->name, name);
	
	/* newFract->image = (uint16_t **)malloc(sizeof(uint16_t *)*height);		//allocate memory for the table of pointer
	if(newFract->image == NULL)
	{
		free(newFract->name);
		free(newFract);
		return NULL;
	}
	
	*(newFract->image) = (uint16_t *)malloc(sizeof(uint16_t)*height*width);		//allocate memory for the whole image
	if (*(newFract->image) == NULL)
	{
		free(newFract->image);
		free(newFract->name);
		free(newFract);
		return NULL;
	}
	for (int i=1;i < height; i++)												//place the pointer of the first table towards their respective line
	{
		*((newFract->image)+i) = *(newFract->image)+i*width;
	} */
	
	newFract->image = (uint16_t *)malloc(sizeof(uint16_t)*height*width);		//allocate memory for the whole image
	if (*(newFract->image) == NULL)
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
	free(*(f->image));
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
	while (*(string + i) != '\0' && i =< 65)
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
