/* Dichiarazioni relative al QuickSort. */
#ifndef QUICK_SORT_H
#define QUICK_SORT_H

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
#define BLOCK_SIZE 4096
/**/
#define PARAM_NUMBER 128

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
SIDE neutralize(int* ar, int i_left, int i_right, int i_pivot, SIDE* ar_result, int i_rank);

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
void quickSortManager(int* ar, int i_arSize, int i_rank, int i_totalProcesses);

/*
	Fasi 1 e 2 dell'algoritmo.

	***Parametri***
		int* ar
			Puntatore all'array di input.
		int i_leftBeginning
			Indice del primo elemento del primo blocco di sinistra.
		int i_leftBlocks
			Numero di blocchi provenienti dalla parte sinistra dell'array.
		int i_rightStart
			Indice del primo elemento del primo blocco di destra.
		int i_rightBlocks
			Numero di blocchi provenienti dalla parte destra dell'array.
		int i_pivot.
			Valore del pivot.

	***Restituzione***
		Indice iniziale del blocco non neutralizzato.
		-1 se tutti i blocchi sono stati neutralizzati.
*/
int phaseOne(int* ar, int i_leftStart, int i_leftBlocks, int i_rightStart, int i_rightBlocks, int i_pivot, int i_rank);

#endif
