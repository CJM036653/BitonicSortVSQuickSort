/* Programma principale, avvia i due algoritmi di sorting. */
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>

#include "BitonicSort.h"
#include "QuickSort.h"

#define MAX_READ_SIZE 16 /* Massima lunghezza di un intero scritto come testo. */
#define INITIAL_INPUT_SIZE 128 /* Lunghezza inziale dell'array di input. */

#define ALLOCATION_FAILED -1 /* Codice di errore per allocazione fallita. */
#define S_ALLOCATION_FAILED "Allocazione fallita.\n"
#define INPUT_ERROR -2 /* Codice di errore per input errato. */
#define S_INPUT_ERROR "Utilizzo: BitonicVSQuickSort <input path>\n"
#define FILE_OPEN_ERROR -3 /* Codice di errore per apertura del file fallita. */
#define S_FILE_OPEN_ERROR "Impossibile aprire il file.\n"

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
	/* Controllo parametri di input. */
	if (argc < 2)
	{
		printf(S_INPUT_ERROR);
		MPI_Abort(MPI_COMM_WORLD, ALLOCATION_FAILED);
	}

	/* Salva l'ID del processo in rank e il numero di processi in i_totalProcesses. */
	int i_rank, i_totalProcesses;
	MPI_Comm_rank(MPI_COMM_WORLD, &i_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &i_totalProcesses);

	int* ar_bitonic = NULL; /* Input per il bitonic sort. */
	int* ar_quick = NULL; /* Input per il quicksort. */

	/* Il processo 0 legge l'input per gli algoritmi. */
    int i_inputSize; /* Grandezza dell'array contenente l'input. */
	if (i_rank == 0)
	{
        /* Apertura del file specificato in input. */
  	    int fd;
        fd = open(argv[1], O_RDONLY);
        if (fd == -1)
        {
          printf(S_FILE_OPEN_ERROR);
          MPI_Abort(MPI_COMM_WORLD, FILE_OPEN_ERROR);
        }

        int i_readResult; /* Risultato della chiamata read. */
        int i_currentChar = 0; /* Posizione attuale nel buffer di lettura. */
        int i_currentInt; /* Posizione attuale nell'array di input. */
        char ar_readBuffer[MAX_READ_SIZE]; /* Buffer di lettura. */

        i_inputSize = INITIAL_INPUT_SIZE;

        /* L'array viene riempito dalla fine, in modo da aggiungere un eventuale padding di zeri
           all'inizio, in modo che siano gia' ordinati nel caso di interi positivi. */
        i_currentInt = i_inputSize - 1;

        ar_bitonic = malloc(i_inputSize * sizeof(int));
        if (ar_bitonic == NULL)
        {
            printf(S_ALLOCATION_FAILED);
            close(fd);
            MPI_Abort(MPI_COMM_WORLD, ALLOCATION_FAILED);
        }

        /* Lettura dell'input. */
        BOOL done = FALSE;
        while (!done)
        {
            i_readResult = read(fd, &(ar_readBuffer[i_currentChar]), sizeof(char));
            /* Termina se viene raggiunto l'eof o se ci sono stati errori. */
            if (i_readResult <= 0)
            {
                done = TRUE;
            }
            else
            {
                /* Se l'array non e' sufficientemente grande, viene riallocato. */
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
                /* Viene letto un numero per riga. */
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
        close(fd);

        /* Padding con 0 per raggiungere la potenza di 2 successiva. */
        int i_paddingSize = i_currentInt+1;
        while (i_currentInt >= 0)
        {
            ar_bitonic[i_currentInt] = 0;
            --i_currentInt;
        }

        /* Copia dell'input per l'utilizzo su entrambi gli algoritmi. */
        ar_quick = copyArray(ar_bitonic, i_inputSize);
  	    if (ar_quick == NULL)
        {
	        printf(S_ALLOCATION_FAILED);
	        free(ar_bitonic);
	        MPI_Abort(MPI_COMM_WORLD, ALLOCATION_FAILED);
        }
	}

    MPI_Bcast(&i_inputSize, 1, MPI_INT, 0, MPI_COMM_WORLD);
    /* Avvio di QuickSort. */
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
