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

#define BLOCK_SIZE 16 /* Grandezza dei singoli blocchi. */
#define MAX_PROCESSORS 32 /* Numero massimo di processori supportati. */

/* Soglia della dimensione di un array sotto la quale viene eseguito insertion sort invece di quick sort. */
#define QUICK_THRESHOLD 30

#define ALLOCATION_FAILED_ERR -1

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
	Funzione che contiene le fasi 1 e 2 dell'algoritmo.

	*** Parametri ***
		int* ar
			Puntatore all'array di input.
		int i_arSize
			Numero di elementi di ar.
        int i_rank
            Rango del processo.
		int i_totalProcesses
			Numero di processi attivi.
        MPI_Comm communicator
            Comunicatore del gruppo di processi corrente.

    *** Restituzione ***
        Indice dello split point. A sinistra rimangono tutti gli elementi minori o uguali del pivot, a destra tutti
        quelli maggiori o uguali.
*/
int phaseOneTwo(int* ar, int i_arSize, int i_rank, int i_totalProcesses, MPI_Comm communicator);

/*
    Funzione che contiene le fasi 3 e 4 dell'algoritmo.
    Richiama iterativamente phaseOneTwo(...).

    *** Parametri ***
        int* ar
            Puntatore all'array di input.
        int i_arSize
            Numero di elementi di ar.
        int i_rank
            Rango del processo.
        int i_totalProcesses
            Numero di processi attivi.

    *** Restituzione ***
        Puntatore all'array ordinato.
*/
int* quickSortManager(int* ar, int i_arSize, int i_rank, int i_totalProcesses);

/*
    Quicksort sequenziale, utilizzato nella fase 4.

    *** Parametri ***
        int* ar
            Puntatore all'array di input.
        int i_arSize
            Numero di elementi di ar.

    *** Restituzione ***
        Puntatore all'array ordinato.
*/
int* quickSort(int* ar, int i_arSize);

/*
    Fase Divide del quicksort: scelto un pivot sposta gli elementi minori alla sua sinistra e quelli maggiori alla sua
    destra

    *** Parametri ***
        int* ar
            Puntatore all'array di input.
        int left
            Primo indice da considerare.
        int right
            Ultimo indice da considerare.

    *** Restituzione ***
        Indice del pivot.
*/
int partition(int* ar, int left, int right);
#endif
