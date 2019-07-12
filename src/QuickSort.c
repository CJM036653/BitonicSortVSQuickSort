/* Implementazione di QuickSort. */
#include "QuickSort.h"

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <mpi.h>
#include <math.h>

/* Dealloca le risorse. */
static void freeMemory(void* ar1, void* ar2, void* ar3, void* ar4, void* ar5, void* ar6, void* ar7, void* ar8)
{
    free(ar1);
    free(ar2);
    free(ar3);
    free(ar4);
    free(ar5);
    free(ar6);
    free(ar7);
    free(ar8);
}

/* Inverte due blocchi di grandezza BLOCK_SIZE. */
static void swap(int* ar, int i_left, int i_right)
{
	int q, temp;
	for (q = 0; q < BLOCK_SIZE; q++)
	{
		temp = ar[i_left];
		ar[i_left] = ar[i_right];
		ar[i_right] = temp;
		++i_left;
		++i_right;
	}
}

/* Scelta del pivot. */
static int pivotChoice(int* ar, int i_arSize, int i_offset)
{
    int i_1, i_2, i_3, i_min, i_max;
    i_1 = (rand() % i_arSize) + i_offset;
    i_2 = (rand() % i_arSize) + i_offset;
    i_3 = (rand() % i_arSize) + i_offset;
    i_1 = ar[i_1];
    i_2 = ar[i_2];
    i_3 = ar[i_3];

    if (i_1 > i_2)
    {
        i_max = (i_1 > i_3 ? i_1 : i_3);
        i_min = (i_2 < i_3 ? i_2 : i_3);
    }
    else
    {
        i_max = (i_2 > i_3 ? i_2 : i_3);
        i_min = (i_1 < i_3 ? i_1 : i_3);
    }

    return (i_min + i_max)/2;
}

SIDE neutralize(int* ar_left, int* ar_right, int i_pivot)
{
	int i_left = 0; /* Posizione nel blocco di sinistra. */
    int i_right = 0; /* Posizione nel blocco di destra. */

	do
	{
		/* Trova il primo elemento maggiore del pivot. */
		while (i_left < BLOCK_SIZE && ar_left[i_left] <= i_pivot)
		{
			++i_left;
		}

		/* Trova il primo elemento minore del pivot. */
		while (i_right < BLOCK_SIZE && ar_right[i_right] >= i_pivot)
		{
			++i_right;
		}

		/* Se ci sono due elementi nel posto sbagliato, invertili. */
		if (i_left < BLOCK_SIZE && i_right < BLOCK_SIZE)
		{
			int temp;
			temp = ar_left[i_left];
			ar_left[i_left] = ar_right[i_right];
			ar_right[i_right] = temp;
			++i_left;
			++i_right;
		}
	} while(i_left < BLOCK_SIZE && i_right < BLOCK_SIZE);

	/* Verifica quale lato e' stato neutralizzato e ritorna. */
	if (i_left == BLOCK_SIZE)
	{
		if (i_right == BLOCK_SIZE)
		{
			return BOTH;
		}
		return LEFT;
	}
	return RIGHT;
}

int phaseOneTwo(int* ar, int i_arSize, int i_rank, int i_totalProcesses, MPI_Comm communicator)
{
    int i_splitPoint; /* Soglia tra gli elementi minori o uguali del pivot e quelli maggiori o uguali.*/

	/*********** FASE UNO ***********/
	int i_LN, i_RN, i_leftBlock, i_rightBlock, i_pivot;
	i_LN = 0; /* Indice che contiene l'inizio dell'area centrale non neutralizzata. */
	i_RN = i_arSize; /* Indice che contiene il primo elemento fuori dall'area centrale non neutralizzata. */
	i_leftBlock = 0; /* Prossimo blocco sinistro da neutralizzare. */
	i_rightBlock = i_arSize - BLOCK_SIZE; /* Prossimo blocco destro da neutralizzare. */

	int ar_left[BLOCK_SIZE]; /* Blocco sinistro da neutralizzare. */
	int ar_right[BLOCK_SIZE]; /* Blocco destro da neutralizzare. */
	SIDE lastNeutralizedSide = BOTH; /* Ultimo lato neutralizzato. BOTH assicura che vengano assegnati due blocchi nuovi. */

    /* Array utilizzati dal root per la gestione dell'elaborazione. */
	int* ar_remainingBlocks = NULL; /* Indici dei blocchi non neutralizzati per ogni processo. */
	SIDE* ar_neutralizedSides = NULL; /* Ultimi blocchi neutralizzati dai processi. */
	int* ar_sndParams = NULL; /* Indici dei blocchi inviati ad ogni processo. */
    ProcessState* ar_processStates = NULL; /* Stati dei processi. */

	MPI_Request reqs[4];
	MPI_Request* reqs0;
    MPI_Request* reqProcState;
	MPI_Status* status;

	if (i_rank == 0)
	{
        /* Inizializza ar_remainingBlocks a indici non validi. */
        int i;
		ar_remainingBlocks = malloc(sizeof(int) * i_totalProcesses);
        if (ar_remainingBlocks == NULL) MPI_Abort(MPI_COMM_WORLD, ALLOCATION_FAILED_ERR);
        for(i = 0; i < i_totalProcesses; ++i)
        {
            ar_remainingBlocks[i] = -1;
        }

		/* Inizializza a zero in modo che alla prima iterazione vengano dati due blocchi a ogni processo. */
		ar_neutralizedSides = calloc(i_totalProcesses, sizeof(SIDE));

        /* Inizializza ar_sndParams a indici non validi. */
        int i_sndParams = i_totalProcesses << 1;
		ar_sndParams = malloc(sizeof(int) * i_sndParams);
        if (ar_sndParams == NULL)
        {
            freeMemory(ar, ar_remainingBlocks, ar_neutralizedSides, ar_sndParams, ar_processStates, reqs0, reqProcState, status);
            MPI_Abort(MPI_COMM_WORLD, ALLOCATION_FAILED_ERR);
        }
        for(i = 0; i < i_sndParams; ++i)
        {
            ar_sndParams[i] = -1;
        }

        /* Inizializza ar_processStates ad ACTIVE. */
        ar_processStates = calloc(sizeof(ProcessState), i_totalProcesses);

        /* Alloca le richieste di comunicazione. */
		reqs0 = malloc(sizeof(MPI_Request) * (i_totalProcesses << 1));
        reqProcState = malloc(sizeof(MPI_Request) * i_totalProcesses);
		status = malloc(sizeof(MPI_Status) * (i_totalProcesses << 1));

        /* Controllo sulle allocazioni. */
        if (ar_neutralizedSides == NULL || ar_processStates == NULL || reqs0 == NULL || reqProcState == NULL || status == NULL)
        {
            freeMemory(ar, ar_remainingBlocks, ar_neutralizedSides, ar_sndParams, ar_processStates, reqs0, reqProcState, status);
            MPI_Abort(MPI_COMM_WORLD, ALLOCATION_FAILED_ERR);
        }

        /*i_pivot = (ar[0] + ar[i_arSize - 1]) / 2;*/
		i_pivot = pivotChoice(ar, i_arSize, 0);
	}
	MPI_Bcast(&i_pivot, 1, MPI_INT, 0, communicator);

    /* Tutti i processi sono attualmente attivi. */
    ProcessState currentState = ACTIVE;
    int remainder = i_arSize % BLOCK_SIZE; /* Numero di elementi in eccesso ai blocchi. */

    /* Se sono disponibili almeno 2 blocchi procedi con la fase 1. */
    if (i_leftBlock < i_rightBlock - remainder)
    {
    	while (i_leftBlock <= i_rightBlock - remainder)
    	{
    		MPI_Barrier(communicator);
    		if (i_rank == 0)
    		{
    			int i, j;
    			j = 0;
                /* Invia a tutti i processi attivi i prossimi blocchi da neutralizzare.
                   Se non e' possibile inviare altri blocchi, il processo viene etichettato come INACTIVE o OPERATION_PENDING. */
    			for (i = 0; i < i_totalProcesses; ++i)
    			{
                    if (ar_processStates[i] == ACTIVE)
                    {
        				if (ar_neutralizedSides[i] == BOTH)
        				{
                            if (i_leftBlock < i_rightBlock - remainder)
                            {
            					ar_sndParams[j] = i_leftBlock;
            					MPI_Isend(&ar[i_leftBlock], BLOCK_SIZE, MPI_INT, i, BLOCK_DISTRIBUTION_TAG_LEFT, communicator, &reqs[0]);
            					i_leftBlock += BLOCK_SIZE;
            					ar_sndParams[++j] = i_rightBlock;
            					MPI_Isend(&ar[i_rightBlock], BLOCK_SIZE, MPI_INT, i, BLOCK_DISTRIBUTION_TAG_RIGHT, communicator, &reqs[1]);
            					i_rightBlock -= BLOCK_SIZE;
            					++j;
                            }
                            else
                            {
                                ar_processStates[i] = INACTIVE;
                            }
        				}
        				else if (ar_neutralizedSides[i] == LEFT)
        				{
                            if (i_leftBlock <= i_rightBlock - remainder)
                            {
            					ar_sndParams[j] = i_leftBlock;
            					MPI_Isend(&ar[i_leftBlock], BLOCK_SIZE, MPI_INT, i, BLOCK_DISTRIBUTION_TAG_LEFT, communicator, &reqs[2]);
            					i_leftBlock += BLOCK_SIZE;
            					j+=2;
                            }
                            else
                            {
                                ar_processStates[i] = OPERATION_PENDING; /* Il processo deve inviare il blocco non neutralizzato al root. */
                            }
        				}
        				else
        				{
                            if (i_leftBlock <= i_rightBlock - remainder)
                            {
            					ar_sndParams[++j] = i_rightBlock;
            					MPI_Isend(&ar[i_rightBlock], BLOCK_SIZE, MPI_INT, i, BLOCK_DISTRIBUTION_TAG_RIGHT, communicator, &reqs[3]);
            					i_rightBlock -= BLOCK_SIZE;
            					++j;
                            }
                            else
                            {
                                ar_processStates[i] = OPERATION_PENDING; /* Il processo deve inviare il blocco non neutralizzato al root. */
                            }
        				}
                    }
                    /* Invia ad ogni processo il proprio stato. */
                    MPI_Isend(&ar_processStates[i], 1, MPI_INT, i, PROCESS_STATE_TAG, communicator, &reqProcState[i]);
    			}
    		}
            /* Sincronizza le variabili di controllo del ciclo e ricevi lo stato aggiornato. */
    		MPI_Bcast(&i_leftBlock, 1, MPI_INT, 0, communicator);
    		MPI_Bcast(&i_rightBlock, 1, MPI_INT, 0, communicator);
            MPI_Recv(&currentState, 1, MPI_INT, 0, PROCESS_STATE_TAG, communicator, MPI_STATUS_IGNORE);

            /* I processi attivi ricevono i blocchi da neutralizzare. */
            if (currentState == ACTIVE)
            {
                /* Ricezione. */
            	if (lastNeutralizedSide == BOTH)
            	{
            		MPI_Recv(ar_left, BLOCK_SIZE, MPI_INT, MPI_ANY_SOURCE, BLOCK_DISTRIBUTION_TAG_LEFT, communicator, MPI_STATUS_IGNORE);
            		MPI_Recv(ar_right, BLOCK_SIZE, MPI_INT, MPI_ANY_SOURCE, BLOCK_DISTRIBUTION_TAG_RIGHT, communicator, MPI_STATUS_IGNORE);
            	}
            	else if (lastNeutralizedSide == LEFT)
            	{
            		MPI_Recv(ar_left, BLOCK_SIZE, MPI_INT, MPI_ANY_SOURCE, BLOCK_DISTRIBUTION_TAG_LEFT, communicator, MPI_STATUS_IGNORE);
            	}
            	else
            	{
            		MPI_Recv(ar_right, BLOCK_SIZE, MPI_INT, MPI_ANY_SOURCE, BLOCK_DISTRIBUTION_TAG_RIGHT, communicator, MPI_STATUS_IGNORE);
            	}
                /* Neutralizzazione. */
                lastNeutralizedSide = neutralize(ar_left, ar_right, i_pivot);
            }

            /* Raccolta dei risultati. */
    		MPI_Gather(&lastNeutralizedSide, 1, MPI_INT, ar_neutralizedSides, 1, MPI_INT, 0, communicator);

            /* Invia i blocchi appena neutralizzati. */
            if (currentState == ACTIVE)
            {
        		if (lastNeutralizedSide == BOTH)
        		{
        			MPI_Isend(ar_left, BLOCK_SIZE, MPI_INT, 0, BLOCK_UPDATE_TAG_LEFT, communicator, &reqs[0]);
        			MPI_Isend(ar_right, BLOCK_SIZE, MPI_INT, 0, BLOCK_UPDATE_TAG_RIGHT, communicator, &reqs[1]);
        		}
        		else if (lastNeutralizedSide == LEFT)
        		{
        			MPI_Isend(ar_left, BLOCK_SIZE, MPI_INT, 0, BLOCK_UPDATE_TAG_LEFT, communicator, &reqs[0]);
        		}
        		else
        		{
        			MPI_Isend(ar_right, BLOCK_SIZE, MPI_INT, 0, BLOCK_UPDATE_TAG_RIGHT, communicator, &reqs[0]);
        		}
            }
            /* Invia i blocchi non neutralizzati dei processi in disattivazione. */
            else if (currentState == OPERATION_PENDING)
            {
                if (lastNeutralizedSide == LEFT)
        		{
        			MPI_Isend(ar_right, BLOCK_SIZE, MPI_INT, 0, BLOCK_UPDATE_TAG_RIGHT, communicator, &reqs[0]);
        		}
        		else
        		{
        			MPI_Isend(ar_left, BLOCK_SIZE, MPI_INT, 0, BLOCK_UPDATE_TAG_LEFT, communicator, &reqs[0]);
        		}
            }

            /* Il processo root riceve tutti i dati inviati precedentemente. */
    		if (i_rank == 0)
    		{
    			int i;
    			int j = 0;
    			int k = 0;
    			for (i = 0; i < i_totalProcesses; i++)
    			{
                    if (ar_processStates[i] == ACTIVE)
                    {
        				if (ar_neutralizedSides[i] == BOTH)
        				{
        					MPI_Irecv(&ar[ar_sndParams[j]], BLOCK_SIZE, MPI_INT, i, BLOCK_UPDATE_TAG_LEFT, communicator, &reqs0[k]);
        					++j;
        					++k;
        					MPI_Irecv(&ar[ar_sndParams[j]], BLOCK_SIZE, MPI_INT, i, BLOCK_UPDATE_TAG_RIGHT, communicator, &reqs0[k]);
        					++j;
        					++k;
                            ar_remainingBlocks[i] = -1;
                            i_LN += BLOCK_SIZE;
                            i_RN -= BLOCK_SIZE;
        				}
        				else if (ar_neutralizedSides[i] == LEFT)
        				{
        					MPI_Irecv(&ar[ar_sndParams[j]], BLOCK_SIZE, MPI_INT, i, BLOCK_UPDATE_TAG_LEFT, communicator, &reqs0[k]);
                            ar_remainingBlocks[i] = ar_sndParams[++j];
        					++j;
        					++k;
                            i_LN += BLOCK_SIZE;
        				}
        				else
        				{
                            ar_remainingBlocks[i] = ar_sndParams[j];
        					++j;
        					MPI_Irecv(&ar[ar_sndParams[j]], BLOCK_SIZE, MPI_INT, i, BLOCK_UPDATE_TAG_RIGHT, communicator, &reqs0[k]);
        					++j;
        					++k;
                            i_RN -= BLOCK_SIZE;
        				}
                    }
                    else if (ar_processStates[i] == OPERATION_PENDING)
                    {
                        if (ar_neutralizedSides[i] == LEFT)
        				{
                            ++j;
                            MPI_Irecv(&ar[ar_sndParams[j]], BLOCK_SIZE, MPI_INT, i, BLOCK_UPDATE_TAG_RIGHT, communicator, &reqs0[k]);
                            ar_remainingBlocks[i] = ar_sndParams[j];
                            ++j;
                            ++k;
        				}
        				else
        				{
                            MPI_Irecv(&ar[ar_sndParams[j]], BLOCK_SIZE, MPI_INT, i, BLOCK_UPDATE_TAG_LEFT, communicator, &reqs0[k]);
                            ar_remainingBlocks[i] = ar_sndParams[j];
                            j+=2;
                            ++k;
        				}
                        ar_processStates[i] = INACTIVE;
                    }
    			}
    			MPI_Waitall(k, reqs0, status);
    		}
    	}
    }
	MPI_Barrier(communicator);

    /* Aggiorna gli ultimi blocchi non neutralizzati. */
    if (lastNeutralizedSide == LEFT)
    {
        MPI_Isend(ar_right, BLOCK_SIZE, MPI_INT, 0, (i_rank << 1) + 1, communicator, &reqs[0]);
    }
    else if (lastNeutralizedSide == RIGHT)
    {
        MPI_Isend(ar_left, BLOCK_SIZE, MPI_INT, 0, (i_rank << 1), communicator, &reqs[0]);
    }

    if (i_rank == 0)
    {
        int i;
        int j = 0;
        int k = 0;
        for (i = 0; i < i_totalProcesses; i++)
        {
            if (ar_neutralizedSides[i] == BOTH)
            {
                j+=2;
            }
            else if (ar_neutralizedSides[i] == LEFT)
            {
                ++j;
                MPI_Irecv(&ar[ar_sndParams[j]], BLOCK_SIZE, MPI_INT, i, j, communicator, &reqs0[k]);
                ++j;
                ++k;
            }
            else
            {
                MPI_Irecv(&ar[ar_sndParams[j]], BLOCK_SIZE, MPI_INT, i, j, communicator, &reqs0[k]);
                j+=2;
                ++k;
            }
        }
        MPI_Waitall(k, reqs0, status);
    }


	/*********** FASE DUE ***********/
	if (i_rank == 0)
	{
		/* Ordinamento dell'array ar_remainingBlocks tramite selection sort. */
		int i, temp;
		for (i = 0; i < i_totalProcesses-1; i++)
		{
			int j;
			int i_min = ar_remainingBlocks[i];
			int i_minIndex = i;
			for (j = i+1; j < i_totalProcesses; j++)
			{
					if (ar_remainingBlocks[j] < i_min)
					{
						 i_min = ar_remainingBlocks[j];
						 i_minIndex = j;
					}
			}
            temp = ar_remainingBlocks[i];
            ar_remainingBlocks[i] = i_min;
            ar_remainingBlocks[i_minIndex] = temp;
		}

		/* Salta i processori che hanno neutralizzato tutti i blocchi. */
		i = 0;
		int j = i_totalProcesses-1;
		while (ar_remainingBlocks[i] == -1)
		{
			++i;
		}

		/* Neutralizza i blocchi rimanenti. */
		while (i < i_totalProcesses &&
    		   i < j &&
    		   ar_remainingBlocks[i] < i_LN &&
    		   ar_remainingBlocks[j] >= i_RN)
		{
			lastNeutralizedSide = neutralize(&ar[ar_remainingBlocks[i]], &ar[ar_remainingBlocks[j]], i_pivot);
			if (lastNeutralizedSide == BOTH)
			{
				i_LN += BLOCK_SIZE;
				i_RN -= BLOCK_SIZE;
				++i;
				--j;
			}
			else if (lastNeutralizedSide == LEFT)
			{
				i_LN -= BLOCK_SIZE;
				++i;
			}
			else
			{
				i_RN -= BLOCK_SIZE;
				--j;
			}
		}

		/* Superato i_LN o i_RN riposiziona i blocchi non neutralizzati
			 dentro l'intervallo [i_LN, i_RN[. */
        int i_swapL, i_swapR;
        if (ar_remainingBlocks[i] < i_LN)
        {
            i_swapR = i_LN;
            i_swapL = ar_remainingBlocks[i];
        }
        else if (ar_remainingBlocks[j] >= i_RN)
        {
            i_swapL = i_RN - BLOCK_SIZE;
            i_swapR = ar_remainingBlocks[j];
        }

        while (i < i_totalProcesses && ar_remainingBlocks[i] < i_LN)
        {
            /* Verifica se i_swapR e' un blocco non neutralizzato. */
            int i_searchCounter = i+i;
            while (i_searchCounter < i_totalProcesses && i_swapR != ar_remainingBlocks[i_searchCounter])
            {
                ++i_searchCounter;
            }
            /* Se i_swapR e' gia' stato neutralizzato, prova con il blocco successivo. */
            if (i_searchCounter < i_totalProcesses)
            {
                i_swapR += BLOCK_SIZE;
            }
            /* Altrimenti inverti i_swapL e i_swapR e passa al blocco da invertire successivo. */
            else
            {
                swap(ar, i_swapL, i_swapR);
                i_swapL = ar_remainingBlocks[++i];
            }
        }

        while (j >= 0 && ar_remainingBlocks[j] >= i_RN)
        {
            /* Verifica se i_swapL e' un blocco non neutralizzato. */
            int i_searchCounter = j-1;
            while (i_searchCounter >= 0 && i_swapL != ar_remainingBlocks[i_searchCounter])
            {
                --i_searchCounter;
            }

            /* Se i_swapL e' gia' stato neutralizzato, prova con il blocco precedente. */
            if (i_searchCounter >= 0)
            {
                i_swapL -= BLOCK_SIZE;
            }
            /* Altrimenti inverti i_swapL e i_swapR e passa al blocco da invertire precedente. */
            else
            {
                swap(ar, i_swapL, i_swapR);
                i_swapR = ar_remainingBlocks[--j];
            }
        }

		/* Partizionamento degli elementi rimanenti in base al pivot. */
        --i_RN;
		while (i_LN < i_RN)
		{
			if (ar[i_LN] <= i_pivot)
			{
				++i_LN;
			}
			else if (ar[i_RN] >= i_pivot)
			{
				--i_RN;
			}
			else
			{
				temp = ar[i_LN];
				ar[i_LN] = ar[i_RN];
				ar[i_RN] = temp;
			}
		}
        i_splitPoint = i_LN;
        freeMemory(NULL, ar_remainingBlocks, ar_neutralizedSides, ar_sndParams, ar_processStates, reqs0, reqProcState, status);
	}

	MPI_Bcast(&i_splitPoint, 1, MPI_INT, 0, communicator);
	return i_splitPoint;
}

int* quickSortManager(int* ar, int i_arSize, int i_rank, int i_totalProcesses)
{
    /*********** FASE TRE ***********/
    int* ar_currentAr = ar; /* Array assegnato a questo processo. */
    int i_currentSize = i_arSize; /* Grandezza di ar_currentAr. */
    int i_currentRank = i_rank; /* Rango del processo nel comunicatore parziale. */
    int i_groupSize = i_totalProcesses; /* Numero di processi nel comunicatore parziale. */
    int i_processesInPhase3 = i_totalProcesses; /* Numero di processi nella fase 3. */
    int i_rootProcess = 0; /* Root del comunicatore parziale rispetto a MPI_COMM_WORLD. */

    /* Comunicatore parziale, inizialmente corrispondente a MPI_COMM_WORLD. */
    MPI_Comm communicator;
    MPI_Comm_dup(MPI_COMM_WORLD, &communicator);

    char currentChar = 'a'; /* Carattere che identifica i root dell'iterazione corrente. */
    int i_startIndex = 0; /* Indice assoluto di partenza della sezione considerata da un processo. */
    int i_splitPoint; /* Split point corrente. */

    /* Stato attuale del processo. Inizialmente tutti procedono con la fase 3. */
    ContinueState st_currentState;
    st_currentState.i_rank = i_rank;
    st_currentState.b_continue = TRUE;

    /* Richieste di invio. */
    MPI_Request sndReq0, sndReq1, sndReq2, sndReq3, sndReq4, sndReq5, sndReq6, sndReq7, sndReq8, sndReq9, sndReq10, sndReq11, sndReq12;

    /* Array che tiene conto di quali processi sono i root dell'iterazione corrente. */
    char* ar_rootIndices = NULL;
	if (i_rank == 0)
	{
        srand(time(NULL));
		ar_rootIndices = calloc(i_totalProcesses, sizeof(char));
        if (ar_rootIndices == NULL)
        {
            free(ar);
            MPI_Abort(MPI_COMM_WORLD, ALLOCATION_FAILED_ERR);
        }
	}

    do
    {
        /* Esegui fasi 1 e 2. */
        if (i_groupSize > 1)
        {
            i_splitPoint = phaseOneTwo(ar_currentAr, i_currentSize, i_currentRank, i_groupSize, communicator);
        }

        /* Raccolta dei dati aggiornati. */
        if (i_rank == i_rootProcess && i_rank != 0)
        {
            MPI_Isend(&i_startIndex, 1, MPI_INT, 0, START_INDEX_TAG, MPI_COMM_WORLD, &sndReq0);
            MPI_Isend(&i_currentSize, 1, MPI_INT, 0, SECTION_LENGTH_TAG, MPI_COMM_WORLD, &sndReq1);
            MPI_Isend(ar_currentAr, i_currentSize, MPI_INT, 0, UPDATED_ARRAY_TAG, MPI_COMM_WORLD, &sndReq2);
            MPI_Wait(&sndReq2, MPI_STATUS_IGNORE); /* Attende che 0 riceva i dati, per non riallocare ar_currentAr troppo presto. */
        }
        if (i_rank == 0)
        {
            int i;
            for (i = 1; i < i_totalProcesses; ++i)
            {
                if (ar_rootIndices[i] == currentChar)
                {
                    int i_start, i_len;
                    MPI_Recv(&i_start, 1, MPI_INT, i, START_INDEX_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    MPI_Recv(&i_len, 1, MPI_INT, i, SECTION_LENGTH_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    MPI_Recv(&ar[i_start], i_len, MPI_INT, i, UPDATED_ARRAY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                }
            }
        }

        /* Dividi i processi tra le due parti dell'array. */
        if (i_groupSize > 1)
        {
            int i_L1 = i_splitPoint; /* Grandezza del blocco di sinistra. */
    		int i_L2 = i_currentSize - i_L1; /* Grandezza del blocco di destra. */
    		int i_P2 = round((i_L2 * i_groupSize)/i_currentSize); /* Numero di processi della parte di destra. */
            if (i_P2 == 0)
            {
                i_P2 = 1;
            }
            else if (i_P2 == i_groupSize)
            {
                i_P2 = i_groupSize - 1;
            }
    		int i_P1 = i_groupSize - i_P2; /* Numero di processi della parte di sinistra. */

            /* Se il processo corrente e' nella parte sinistra... */
    		if (i_rank >= i_rootProcess && i_rank < i_rootProcess + i_P1)
    		{
    			i_groupSize = i_P1;
                i_currentSize = i_L1;
    		}
            /* Se il processo corrente e' nella parte destra... */
    		else
    		{
    			i_rootProcess += i_P1;
    			i_groupSize = i_P2;
                i_currentSize = i_L2;
                i_startIndex += i_L1;
    		}
        }

        /* Informa il processo 0 se il processo corrente resta nella fase 3 o passa alla fase 4.
           Il processo 0 deve rimanere nel ciclo per coordinare gli altri. */
        if (i_groupSize == 1 && i_rank != 0)
        {
            st_currentState.b_continue = FALSE;
            i_processesInPhase3 = 1;
        }
        MPI_Isend(&(st_currentState.b_continue), 1, MPI_INT, 0, CONTINUE_TAG, MPI_COMM_WORLD, &sndReq3);

        /* Se il processo resta nella fase 3, prepara la prossima iterazione. */
        /* Creazione del nuovo comunicatore. */
        MPI_Comm newcommunicator;
        MPI_Comm_split(communicator, i_rootProcess, 0, &newcommunicator);
        MPI_Comm_free(&communicator);
        communicator = newcommunicator;
        MPI_Comm_rank(communicator, &i_currentRank);

        /* I root e i processi in uscita devono allocare lo spazio per il prossimo array. */
        if (i_rank != 0 && ((st_currentState.b_continue && i_rank == i_rootProcess) || !st_currentState.b_continue))
        {
            free(ar_currentAr);
            ar_currentAr = malloc(sizeof(int) * i_currentSize);
            if (ar_currentAr == NULL)
            {
                printf("Allocazione fallita (processo %d).\n", i_rank);
                MPI_Abort(MPI_COMM_WORLD, ALLOCATION_FAILED_ERR);
            }
        }

        /* Comunica al processo 0 root, processi in uscita e worker della prossima iterazione. */
        if (i_rank != 0)
        {
            MPI_Isend(&st_currentState, 1, MPI_2INT, 0, RANK_TAG, MPI_COMM_WORLD, &sndReq12);
            if (i_rank == i_rootProcess && st_currentState.b_continue)
            {
                MPI_Isend(&i_rank, 1, MPI_INT, 0, ROOT_TAG, MPI_COMM_WORLD, &sndReq4);
                MPI_Isend(&i_startIndex, 1, MPI_INT, 0, START_INDEX_TAG, MPI_COMM_WORLD, &sndReq5);
                MPI_Isend(&i_currentSize, 1, MPI_INT, 0, SECTION_LENGTH_TAG, MPI_COMM_WORLD, &sndReq6);
                /* Ricevi il prossimo array da elaborare. */
                MPI_Recv(ar_currentAr, i_currentSize, MPI_INT, 0, NEW_ARRAY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
            /* Se il processo passa alla fase 4, richiede il proprio array. */
            else if (!st_currentState.b_continue)
            {
                int temp = -1;
                MPI_Isend(&temp, 1, MPI_INT, 0, ROOT_TAG, MPI_COMM_WORLD, &sndReq7);
                MPI_Isend(&i_startIndex, 1, MPI_INT, 0, START_INDEX_TAG, MPI_COMM_WORLD, &sndReq8);
                MPI_Isend(&i_currentSize, 1, MPI_INT, 0, SECTION_LENGTH_TAG, MPI_COMM_WORLD, &sndReq9);
                MPI_Recv(ar_currentAr, i_currentSize, MPI_INT, 0, NEW_ARRAY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
            else
            {
                int temp = -1;
                MPI_Isend(&temp, 1, MPI_INT, 0, ROOT_TAG, MPI_COMM_WORLD, &sndReq10);
            }
        }

        if (i_rank == 0)
        {
            /* Riceve i root della prossima iterazione. */
            int i;
            ++currentChar;
            for (i = 1; i < i_processesInPhase3; ++i)
            {
                int i_index;
                MPI_Recv(&i_index, 1, MPI_INT, MPI_ANY_SOURCE, ROOT_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                if (i_index != -1)
                {
                    ar_rootIndices[i_index] = currentChar;
                }
            }

            /* Distribuisce ai root i loro array. */
            for (i = 1; i < i_totalProcesses; ++i)
            {
                if (ar_rootIndices[i] == currentChar)
                {
                    int i_start, i_len;
                    MPI_Recv(&i_start, 1, MPI_INT, i, START_INDEX_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    MPI_Recv(&i_len, 1, MPI_INT, i, SECTION_LENGTH_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                    MPI_Isend(&ar[i_start], i_len, MPI_INT, i, NEW_ARRAY_TAG, MPI_COMM_WORLD, &sndReq11);
                }
            }
        }

        /* Il processo 0 aggiorna il conto di quanti processi sono ancora nella fase 3. */
        if (i_rank == 0)
        {
            int i;
            int i_maxIter = i_processesInPhase3;
            MPI_Request lastRequest[MAX_PROCESSORS];
            for (i = 1; i < i_maxIter; ++i)
            {
                /* Se un processo sta uscendo dalla fase 3, inviagli il suo array da ordinare. */
                ContinueState st_state;
                MPI_Recv(&st_state, 1, MPI_2INT, MPI_ANY_SOURCE, RANK_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                if (!st_state.b_continue)
                {
                    --i_processesInPhase3;
                    int i_start, i_len;
                    MPI_Recv(&i_start, 1, MPI_INT, st_state.i_rank, START_INDEX_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    MPI_Recv(&i_len, 1, MPI_INT, st_state.i_rank, SECTION_LENGTH_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    MPI_Isend(&ar[i_start], i_len, MPI_INT, st_state.i_rank, NEW_ARRAY_TAG, MPI_COMM_WORLD, &lastRequest[i]);
                }
            }
        }
    } while(i_processesInPhase3 > 1);


    /*********** FASE QUATTRO ***********/
    printf("Inizio fase sequenziale: i_rank = %d, i_currentSize = %d\n", i_rank, i_currentSize);
    /* Ordinamento sequenziale indipendente per ogni processo. */
    ar_currentAr = quickSort(ar_currentAr, i_currentSize);
    MPI_Request finalRequest;
    if (i_rank != 0)
    {
        MPI_Request sndReq13, sndReq14;
        MPI_Isend(&i_startIndex, 1, MPI_INT, 0, START_INDEX_TAG, MPI_COMM_WORLD, &sndReq13);
        MPI_Isend(&i_currentSize, 1, MPI_INT, 0, SECTION_LENGTH_TAG, MPI_COMM_WORLD, &sndReq14);
        MPI_Isend(ar_currentAr, i_currentSize, MPI_INT, 0, FINAL_UPDATE_TAG, MPI_COMM_WORLD, &finalRequest);
    }
    else
    {
        int i, i_start, i_len;
        MPI_Request finalRequests[MAX_PROCESSORS];
        for (i = 1; i < i_totalProcesses; ++i)
        {
            int i_start, i_len;
            MPI_Recv(&i_start, 1, MPI_INT, i, START_INDEX_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(&i_len, 1, MPI_INT, i, SECTION_LENGTH_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Irecv(&ar[i_start], i_len, MPI_INT, i, FINAL_UPDATE_TAG, MPI_COMM_WORLD, &finalRequests[i]);
        }
        MPI_Waitall(i_totalProcesses-1, &finalRequests[1], MPI_STATUS_IGNORE);
    }

    return ar;
}


int partition(int* ar, int left, int right)
{
    int i_pivot;
    int i_start = left;
    int i_end = right;
    int i_arSize = right - left + 1;
    /* Se siamo sopra la soglia, scegli il pivot e prosegui con la divisione. */
    if (i_arSize > QUICK_THRESHOLD)
    {
        i_pivot = (ar[left] + ar[right]) / 2; /* Scelta semplice, fornisce buone prestazioni in pratica. */
    }
    /* Altrimenti insertion sort ha meno overhead di quick sort e risulta piu' efficiente. */
    else
    {
        /* Insertion sort. */
        int i;
        for (i = i_start + 1; i <= right; ++i)
        {
            int a = i - 1;
            int b = i;
            while (a >= i_start && ar[a] > ar[b])
            {
                int temp = ar[a];
                ar[a] = ar[b];
                ar[b] = temp;
                --a;
                --b;
            }
        }
        return -1;
    }
    /* Suddivisione degli elementi in base al pivot. */
    while (left <= right)
    {
        if (ar[left] < i_pivot)
        {
            ++left;
        }
        else if (ar[right] > i_pivot)
        {
            --right;
        }
        else
        {
            int temp = ar[left];
            ar[left] = ar[right];
            ar[right] = temp;
            left++;
            right--;
        }
    }

    return left;
}


int* quickSort(int* ar, int i_arSize)
{
    int startIndex = 0;
    int endIndex = i_arSize - 1;
    int top = -1;

    int* final = (int*)(malloc(sizeof(int) * i_arSize));

    final[++top] = startIndex;
    final[++top] = endIndex;

    while (top >= 0)
    {
        endIndex = final[top--];
        startIndex = final[top--];

        int p = partition(ar, startIndex, endIndex);

        if (p != -1)
        {
            if (p - 1 > startIndex)
            {
                final[++top] = startIndex;
                final[++top] = p - 1;
            }
            if (p < endIndex)
            {
                final[++top] = p;
                final[++top] = endIndex;
            }
        }
    }

    free(final);
    return ar;
}
