#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*	Create int fractals pattern in each filename
 *	each fractal has a different name and complex number
 *	each fractal will be 1280 * 720
 *	uses ASCII table reference
 *
 */


int main(int argc, char *argv[])
{
	int width = 1280;
	int heigth = 720;
	char name[64];
	char sp = ' ';
	char nl = '\n';
	if (argc < 3)
	{
		printf("Uses fractalGen [int] [filename] *[filename]");
	}
	int fd[argc-2];
	int controle = 0;
	for (int i = 2; i < argc; i++)
	{
		fd[i-2] = open(argv[i], O_RDWR);
		controle++;
		if (fd[i-2] == -1)
		{
			fprintf(stderr, "Error while opening File : %s", argv[i]);
			controle--;
		}
	}
	if (controle == 0)
	{
		fprintf(stderr, "No file opened")
		return -1;
	}
	for (int j = 0;j < argc-2; j++)
	{
		for (int i = 0; i < atoi(argv[1]);i++)
		{
			if (fd[j] > 0)
			{
				double a = ((rand() % 20)/10)-1;
				double b = ((rand() % 20)/10)-1;
				snprintf( name, snprintf( NULL, 0, "%d", i) + 1, "%d", i);
				int size = (int)((i/10)+1)
				printf("name :%s a :%e b :%e", name, a, b);
				write(fd[j], (void*)name, size);
				write(fd[j], (void*)&sp, 1);
				write(fd[j], (void*)&width, sizeof(int));
				write(fd[j], (void*)&sp, 1);
				write(fd[j], (void*)&heigth, sizeof(int));
				write(fd[j], (void*)&sp, 1);
				write(fd[j], (void*)&a, sizeof(double));
				write(fd[j], (void*)&sp, 1);
				write(fd[j], (void*)&b, sizeof(double));
				write(fd[j], (void*)&nl, 1);
			}
		}
	}
	for (int i = 2; i < argc; i++)
	{
		close(fd[i]);
	}
	return 0;
}