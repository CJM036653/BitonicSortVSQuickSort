/* Programma principale, avvia i due algoritmi di sorting. */
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>

#include "BitonicSort.h"
#include "QuickSort.h"

#define INPUT_SIZE 131072 /* Numero di elementi dell'input. */
#define MAX_VALUE 1000 /* Massimo valore assunto dai dati. */
#define MAX_READ_SIZE 16 /* Massima lunghezza di un intero scritto come testo. */
#define INITIAL_INPUT_SIZE 32768 /* Lunghezza inziale dell'array di input. */

#define ALLOCATION_FAILED 1 /* Codice di errore per allocazione fallita. */
#define S_ALLOCATION_FAILED "Allocazione fallita - Uscita dal programma.\n"

/**********FUNZIONI**********/

/*
  	Controlla che l'array di interi ar sia ordinato.

  	***Parametri***
    	int* ar
      		Puntatore all'inizio dell'array da controllare.
    	int i_arSize
      		Numero di elementi di ar.

  	***Restituzione***
    	TRUE
      		ar e' ordinato.
    	FALSE
      		ar non e' ordinato.
*/
BOOL checkSorting(int* ar, int i_arSize);

/*
	Copia un array di interi.

	***Parametri***
		int* ar
			Puntatore all'array da copiare.
		int i_size
			Grandezza dell'array da copiare.

	***Restituzione***
		Puntatore all'array copia.
		NULL se la copia fallisce.
*/
int* copyArray(int* ar, int i_size);


/**********MAIN**********/
int main(int argc, char* argv[])
{
	MPI_Init(&argc, &argv);
	/* Salva l'ID del processo in rank e il numero di processi in i_totalProcesses. */
	int i_rank, i_totalProcesses;
	MPI_Comm_rank(MPI_COMM_WORLD, &i_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &i_totalProcesses);
	int* ar_bitonic = NULL; /* Input per il bitonic sort. */
	int* ar_quick = NULL; /* Input per il quicksort. */

	/* Il processo 0 genera l'input per gli algoritmi e li avvia. */
  int i_inputSize;
	if (i_rank == 0)
	{
  	int fd;
    fd = open(argv[1], O_RDONLY);
    if (fd == -1)
    {
      //ERROR!!!
    }

    int i_readResult, i_currentChar, i_currentInt;
    i_currentChar = 0;
    i_inputSize = INITIAL_INPUT_SIZE;
    i_currentInt = i_inputSize - 1;
    char ar_readBuffer[MAX_READ_SIZE];
    ar_bitonic = malloc(i_inputSize * sizeof(int));
    if (ar_bitonic == NULL)
    {
      printf(S_ALLOCATION_FAILED);
      close(fd);
      MPI_Abort(MPI_COMM_WORLD, ALLOCATION_FAILED);
    }

    BOOL done = FALSE;
    while (!done)
    {
      i_readResult = read(fd, &(ar_readBuffer[i_currentChar]), sizeof(char));
      if (i_readResult == 0)
      {
        done = TRUE;
      }
      else
      {
        if (i_currentInt < 0)
        {
          i_currentInt = i_inputSize-1;
          i_inputSize = i_inputSize << 1;
          ar_bitonic = realloc(ar_bitonic, i_inputSize * sizeof(int));
          if (ar_bitonic == NULL)
          {
            printf(S_ALLOCATION_FAILED);
            close(fd);
            MPI_Abort(MPI_COMM_WORLD, ALLOCATION_FAILED);
          }
        }
        if (ar_readBuffer[i_currentChar] == '\n')
        {
          ar_readBuffer[i_currentChar] = 0;
          ar_bitonic[i_currentInt] = atoi(ar_readBuffer);
          i_currentChar = 0;
          --i_currentInt;
        }
        else
        {
          ++i_currentChar;
        }
      }
    }

    /* Padding con 0. */
    int i_paddingSize = i_currentInt+1;
    printf("PADDING: %d\n", i_paddingSize);
    while (i_currentInt >= 0)
    {
      ar_bitonic[i_currentInt] = 0;
      --i_currentInt;
    }

  	ar_quick = copyArray(ar_bitonic, INPUT_SIZE);
  	if (ar_quick == NULL)
  	{
  		printf(S_ALLOCATION_FAILED);
      close(fd);
  		free(ar_bitonic);
  		MPI_Abort(MPI_COMM_WORLD, ALLOCATION_FAILED);
  	}
    close(fd);
	}

  MPI_Barrier(MPI_COMM_WORLD);
	MPI_Bcast(&ar_quick, 1, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Bcast(&i_inputSize, 1, MPI_INT, 0, MPI_COMM_WORLD);
	quickSortManager(ar_quick, i_inputSize, i_rank, i_totalProcesses);

	if (i_rank == 0)
	{
		free(ar_bitonic);
		free(ar_quick);
	}
  	MPI_Finalize();
  	return 0;
}


BOOL checkSorting(int* ar, int i_arSize)
{
 	int i = 0;
  	for (i; i < i_arSize-1; i++)
  	{
		if (ar[i] > ar[i+1]) return FALSE;
  	}
  	return TRUE;
}

int* copyArray(int* ar, int i_size)
{
	int* ar_result = NULL;
	ar_result = malloc(sizeof(int) * i_size);
	if (ar_result == NULL) return NULL;

	int i = 0;
	for (i; i < i_size; i++)
	{
		ar_result[i] = ar[i];
	}
	return ar_result;
}
