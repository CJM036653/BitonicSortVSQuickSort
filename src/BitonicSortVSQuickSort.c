/* Programma principale, avvia i due algoritmi di sorting. */
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "BitonicSort.h"
#include "QuickSort.h"

#define INPUT_SIZE 100 /* Numero di elementi dell'input. */

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
  	Genera un array di interi casuali.

  	***Parametri***
    	int i_size
      		Grandezza dell'array.

  	***Restituzione***
    	Puntatore all'array generato.
    	NULL se l'allocazione e' fallita.
*/
int* generateRandomArray(int i_size);

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
  	/* Salva l'ID del processo in rank. */
  	int i_rank;
  	MPI_Comm_rank(MPI_COMM_WORLD, &i_rank);

  	/* Il processo 0 genera l'input per gli algoritmi. */
	int* ar_bitonic;
	int* ar_quick;
  	if (i_rank == 0)
  	{
		ar_bitonic = generateRandomArray(INPUT_SIZE);
		if (ar_bitonic == NULL)
		{
			printf(S_ALLOCATION_FAILED);
			MPI_Abort(MPI_COMM_WORLD, ALLOCATION_FAILED );
			//return ALLOCATION_FAILED;
		}

		ar_quick = copyArray(ar_bitonic, INPUT_SIZE);
		if (ar_quick == NULL)
		{
			printf(S_ALLOCATION_FAILED);
			free(ar_bitonic);
			MPI_Abort(MPI_COMM_WORLD, ALLOCATION_FAILED );
			//return ALLOCATION_FAILED;
		}
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

int* generateRandomArray(int i_size)
{
  	srand(time(NULL)); /* Imposta il seed per la generazione di numeri casuali. */

  	/* Allocazione dello spazio di memoria. */
  	int* ar = NULL;
  	ar = malloc(sizeof(int) * i_size);
  	if (ar == NULL) return NULL;

  	/* Popolamento dell'array. */
  	int i = 0;
  	for (i; i < i_size; i++)
  	{
    	ar[i] = rand();
  	}
  	return ar;
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
