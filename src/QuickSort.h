/* Dichiarazioni relative al QuickSort. */
#ifndef QUICK_SORT_H
#define QUICK_SORT_H

#include <mpi.h>

#ifndef BOOL
#define BOOL int
#define TRUE 1
#define FALSE 0
#endif

/* Definizione del tipo SIDE, che indica il blocco neutralizzato. */
#define SIDE int
#define LEFT -1
#define BOTH 0
#define RIGHT 1

/* Grandezza dei singoli blocchi, in modo da avere 2 blocchi in cache. */
#define BLOCK_SIZE 16
/**/
#define PARAM_NUMBER 128
#define MAX_PROCESSORS 32 /* Numero massimo di processori supportati. */

/*******************FUNZIONI*******************/

/*
	Primitiva di neutralizzazione.

	***Parametri***
		int* ar
			Puntatore all'array di input.
		int i_left
			Indice del primo elemento del blocco di sinistra.
		int i_right
			Indice del primo elemento del blocco di destra.
		int i_pivot
			Valore del pivot.

	***Restituzione***
		LEFT se e' stato neutralizzato il blocco sinistro.
		RIGHT se e' stato neutralizzato il blocco destro.
		BOTH se sono stati neutralizzati entrambi i blocchi.
*/
SIDE neutralize(int* ar_left, int* ar_right, int i_pivot);

/*
	Fase 3 dell'algoritmo, richiama iterativamente le fasi 1 e 2.

	***Parametri***
		int* ar
			Puntatore all'array di input.
		int i_arSize
			Numero di elementi di ar.
		int i_totalProcesses
			Numero di processi attivi.
*/
int phaseOneTwo(int* ar, int i_arSize, int i_rank, int i_totalProcesses, MPI_Comm communicator);

void quickSortManager(int* ar, int i_arSize, int i_rank, int i_totalProcesses);
#endif
