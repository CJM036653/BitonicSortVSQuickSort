/* Dichiarazioni relative al QuickSort. */
#ifndef QUICK_SORT_H
#define QUICK_SORT_H

#include <mpi.h>

/* Definizione del tipo booleano. */
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

#define BLOCK_SIZE 4092 /* Grandezza dei singoli blocchi. */
#define MAX_PROCESSORS 32 /* Numero massimo di processori supportati. */

/* Tag di comunicazione. */
typedef enum
{
    START_INDEX_TAG = 1,
    SECTION_LENGTH_TAG,
    UPDATED_ARRAY_TAG,
    CONTINUE_TAG,
    NEW_ARRAY_TAG,
    ROOT_TAG,
    PROCESS_STATE_TAG,
    BLOCK_DISTRIBUTION_TAG_LEFT,
    BLOCK_DISTRIBUTION_TAG_RIGHT,
    BLOCK_UPDATE_TAG_LEFT,
    BLOCK_UPDATE_TAG_RIGHT,
    FINAL_UPDATE_TAG,
    RANK_TAG
} CommunicationTag;

/* Stato di un processo nella fase 1. */
typedef enum
{
    ACTIVE, /* Processo attivo nell'elaborazione. */
    OPERATION_PENDING, /* Operazione necessaria (es. consegna dell'output). */
    INACTIVE /* Processo inattivo. */
} ProcessState;

/* Stato e rango di un processo nella fase 3. */
typedef struct
{
    int i_rank; /* Rango del processo. */
    BOOL b_continue; /* TRUE se il processo rimane nella fase 3, FALSE se passa alla fase 4. */
} ContinueState;


/*******************FUNZIONI*******************/

/*
	Primitiva di neutralizzazione.

	*** Parametri ***
		int* ar_left
			Puntatore all'array di input sinistro.
		int* ar_right
            Puntatore all'array di input destro.
		int i_pivot
			Valore del pivot.

	*** Restituzione ***
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

int* quickSortManager(int* ar, int i_arSize, int i_rank, int i_totalProcesses);

int* quickSort(int* ar, int i_arSize);

int partition(int* ar, int left, int right);

#endif
