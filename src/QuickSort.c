/* Implementazione di QuickSort. */
#include "QuickSort.h"

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <mpi.h>

/* Inverte due blocchi di grandezza BLOCK_SIZE. */
static void swap(int* ar, int i_left, int i_right)
{
	int q, temp;
	for (q = 0; q < BLOCK_SIZE; q++)
	{
		temp = ar[i_left];
		ar[i_right] = ar[i_right];
		ar[i_right] = temp;
		++i_left;
		++i_right;
	}
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
    BOOL check = TRUE;

	int i_splitPoint;

	/*********** FASE UNO ***********/
	int i_LN, i_RN, i_leftBlock, i_rightBlock, i_pivot;
	i_LN = -BLOCK_SIZE;
	i_RN = i_arSize + BLOCK_SIZE;
	i_leftBlock = 0;
	i_rightBlock = i_arSize - BLOCK_SIZE;

	int ar_left[BLOCK_SIZE];
	int ar_right[BLOCK_SIZE];
	SIDE lastNeutralizedSide = BOTH;

	int* ar_remainingBlocks = NULL;
	SIDE* ar_neutralizedSides = NULL;
	int* ar_sndParams = NULL;
    ProcessState* ar_processStates = NULL;

	MPI_Request reqs[4];
	MPI_Request* reqs0;
    MPI_Request* reqProcState;
	MPI_Status* status;

	if (i_rank == 0)
	{
		ar_remainingBlocks = malloc(sizeof(int) * i_totalProcesses);
		/* Inizializza a zero in modo che alla prima iterazione vengano dati due blocchi a ogni processore. */
		ar_neutralizedSides = calloc(i_totalProcesses, sizeof(SIDE));
		ar_sndParams = malloc(sizeof(int) * (i_totalProcesses << 1));
        ar_processStates = calloc(sizeof(ProcessState), i_totalProcesses);

		reqs0 = malloc(sizeof(MPI_Request) * (i_totalProcesses << 1));
        reqProcState = malloc(sizeof(MPI_Request) * i_totalProcesses);
		status = malloc(sizeof(MPI_Status) * (i_totalProcesses << 1));

		/* Scelta del pivot. */
		srand(time(NULL));
		int i_1, i_2, i_3, i_min, i_max;
		i_1 = rand() % i_arSize;
		i_2 = rand() % i_arSize;
		i_3 = rand() % i_arSize;
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

		i_pivot = (i_min + i_max)/2;
        printf("Pivot: %d\n", i_pivot);
	}
	MPI_Bcast(&i_pivot, 1, MPI_INT, 0, communicator);

    ProcessState currentState = ACTIVE;
	while (i_leftBlock < i_rightBlock)
	{
		MPI_Barrier(communicator);
		if (i_rank == 0)
		{
			int i, j;
			j = 0;
			for (i = 0; i < i_totalProcesses; i++)
			{
                if (ar_processStates[i] == ACTIVE)
                {
    				if (ar_neutralizedSides[i] == BOTH)
    				{
                        if (i_leftBlock < i_rightBlock)
                        {
        					ar_sndParams[j] = i_leftBlock;
        					MPI_Isend(&ar[i_leftBlock], BLOCK_SIZE, MPI_INT, i, BLOCK_DISTRIBUTION_TAG_LEFT, communicator, &reqs[0]);
        					i_leftBlock += BLOCK_SIZE;
        					ar_sndParams[++j] = i_rightBlock;
        					MPI_Isend(&ar[i_rightBlock], BLOCK_SIZE, MPI_INT, i, BLOCK_DISTRIBUTION_TAG_RIGHT, communicator, &reqs[1]);
        					i_rightBlock -= BLOCK_SIZE;
        					ar_remainingBlocks[i] = -1;
        					++j;
        					i_LN += BLOCK_SIZE;
        					i_RN -= BLOCK_SIZE;
                        }
                        else
                        {
                            ar_processStates[i] = INACTIVE;
                        }
    				}
    				else if (ar_neutralizedSides[i] == LEFT)
    				{
                        if (i_leftBlock < i_rightBlock)
                        {
        					ar_sndParams[j] = i_leftBlock;
        					MPI_Isend(&ar[i_leftBlock], BLOCK_SIZE, MPI_INT, i, BLOCK_DISTRIBUTION_TAG_LEFT, communicator, &reqs[2]);
        					i_leftBlock += BLOCK_SIZE;
        					ar_remainingBlocks[i] = ar_sndParams[++j];
        					++j;
        					i_LN += BLOCK_SIZE;
                        }
                        else
                        {
                            ar_processStates[i] = OPERATION_PENDING; /* Il processo deve inviare il blocco non neutralizzato al root. */
                        }
    				}
    				else
    				{
                        if (i_leftBlock < i_rightBlock)
                        {
        					ar_remainingBlocks[i] = ar_sndParams[j];
        					ar_sndParams[++j] = i_rightBlock;
        					MPI_Isend(&ar[i_rightBlock], BLOCK_SIZE, MPI_INT, i, BLOCK_DISTRIBUTION_TAG_RIGHT, communicator, &reqs[3]);
        					i_rightBlock -= BLOCK_SIZE;
        					++j;
        					i_RN -= BLOCK_SIZE;
                        }
                        else
                        {
                            ar_processStates[i] = OPERATION_PENDING; /* Il processo deve inviare il blocco non neutralizzato al root. */
                        }
    				}
                    /* Invia ad ogni processo il proprio stato. */
                    MPI_Isend(&ar_processStates[i], 1, MPI_INT, i, PROCESS_STATE_TAG, communicator, &reqProcState[i]);
                }
			}
		}
		MPI_Bcast(&i_leftBlock, 1, MPI_INT, 0, communicator);
		MPI_Bcast(&i_rightBlock, 1, MPI_INT, 0, communicator);
        MPI_Recv(&currentState, 1, MPI_INT, 0, PROCESS_STATE_TAG, communicator, MPI_STATUS_IGNORE);

        if (currentState == ACTIVE)
        {
            /* Ricezione dei nuovi blocchi da neutralizzare. */
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

        /* Aggiornamento dei blocchi appena neutralizzati. */
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
        /* Aggiornamento dei blocchi non neutralizzati dei processi in disattivazione. */
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
    				}
    				else if (ar_neutralizedSides[i] == LEFT)
    				{
    					MPI_Irecv(&ar[ar_sndParams[j]], BLOCK_SIZE, MPI_INT, i, BLOCK_UPDATE_TAG_LEFT, communicator, &reqs0[k]);
    					j+=2;
    					++k;
    				}
    				else
    				{
    					++j;
    					MPI_Irecv(&ar[ar_sndParams[j]], BLOCK_SIZE, MPI_INT, i, BLOCK_UPDATE_TAG_RIGHT, communicator, &reqs0[k]);
    					++j;
    					++k;
    				}
                }
                else if (ar_processStates[i] == OPERATION_PENDING)
                {
                    if (ar_neutralizedSides[i] == LEFT)
    				{
                        ++j;
                        MPI_Irecv(&ar[ar_sndParams[j]], BLOCK_SIZE, MPI_INT, i, BLOCK_UPDATE_TAG_RIGHT, communicator, &reqs0[k]);
                        ++j;
                        ++k;
    				}
    				else
    				{
                        MPI_Irecv(&ar[ar_sndParams[j]], BLOCK_SIZE, MPI_INT, i, BLOCK_UPDATE_TAG_LEFT, communicator, &reqs0[k]);
                        j+=2;
                        ++k;
    				}
                }
			}
			MPI_Waitall(k, reqs0, status);
		}

        if (i_rank == 0)
        {
            printf("\n\ni_leftBlock = %d, i_rightBlock = %d\n", i_leftBlock, i_rightBlock);
            int i;
            for (i = 0; i < i_arSize / BLOCK_SIZE; ++i)
            {
                int j;
                for (j = 0; j < BLOCK_SIZE; ++j)
                {
                    printf("%3d ", ar[(BLOCK_SIZE * i) + j]);
                }
                printf("\n");
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
		/* Ordinamento dell'array ar_remainingBlocks. */
		int i, temp;
		for (i = 0; i < i_totalProcesses-1; i++)
		{
			int j;
			int i_min = ar_remainingBlocks[i];
			int i_minIndex = 0;
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
				++i_LN;
				--i_RN;
			}
		}
        i_splitPoint = i_LN;
	}
	MPI_Bcast(&i_splitPoint, 1, MPI_INT, 0, communicator);

    /*
	if (i_rank == 0)
	{
		int i;
		for (i = 0; i < i_totalProcesses; i++)
		{
			printf("RemainingBlock[%d]: %d\n", i, ar_remainingBlocks[i]);
		}
		printf("\n");
		printf("LN: %d, RN: %d\n", i_LN, i_RN);
		printf("i_splitPoint %d\n", i_splitPoint);
	}
    */
	return i_splitPoint;
}

int* quickSortManager(int* ar, int i_arSize, int i_rank, int i_totalProcesses)
{
    if (i_rank == 0)
    {
        int i;
        for (i = 0; i < i_arSize / BLOCK_SIZE; ++i)
        {
            int j;
            for (j = 0; j < BLOCK_SIZE; ++j)
            {
                printf("%3d ", ar[(BLOCK_SIZE * i) + j]);
            }
            printf("\n");
        }
        printf("\n\n");
    }
    /*********** FASE TRE ***********/
    int* ar_currentAr = ar;
    int i_currentSize = i_arSize;
    int i_currentRank = i_rank;
    int i_groupSize = i_totalProcesses;
    //int i_processesInPhase3 = i_totalProcesses; /*RIPRISTINARE QUESTA RIGA PER L'ESECUZIONE VERA****************************************************************************/
    int i_processesInPhase3 = 1; /*TOGLIERE QUESTA RIGA***********************************************************************************************************************/
    int i_rootProcess = 0;
    MPI_Comm communicator;
    MPI_Comm_dup(MPI_COMM_WORLD, &communicator);
    char currentChar = 1;
    int i_startIndex = 0; /* Indice di partenza della sezione considerata da un processo. */
    int i_splitPoint;

    /* Array che tiene conto di quali processori sono i root dell'iterazione corrente. */
    int i_rootsNumber = 1; /* Numero di root dell'iterazione corrente. */
    char* ar_rootIndices = NULL;
	if (i_rank == 0)
	{
		ar_rootIndices = calloc(sizeof(char), i_totalProcesses);
	}

    do
    {
        /* Esegui fasi 1 e 2. */
        if (i_groupSize > 1)
        {
            i_splitPoint = phaseOneTwo(ar_currentAr, i_currentSize, i_currentRank, i_groupSize, communicator);
            if (i_rank == 0)
            {
                printf("Splitpoint: %d\n", i_splitPoint);
            }
        }

        /* Raccolta dei dati aggiornati. */
        MPI_Request reqStartIndex, reqLen, reqContinue;
        if (i_rank == i_rootProcess && i_rank != 0)
        {
            MPI_Isend(&i_startIndex, 1, MPI_INT, 0, START_INDEX_TAG, MPI_COMM_WORLD, &reqStartIndex);
            MPI_Isend(&i_currentSize, 1, MPI_INT, 0, SECTION_LENGTH_TAG, MPI_COMM_WORLD, &reqLen);
            MPI_Isend(ar_currentAr, i_currentSize, MPI_INT, 0, UPDATED_ARRAY_TAG, MPI_COMM_WORLD, &reqContinue);
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
        int i_L1 = i_splitPoint; /* Grandezza del blocco di sinistra. */
		int i_L2 = i_arSize - i_L1; /* Grandezza del blocco di destra. */
		int i_P2 = (i_L2 * i_groupSize)/i_arSize; /* Numero di processi della parte di destra. */
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

        /* Informa il processo 0 se il processo corrente resta nella fase 3 o passa alla fase 4.
           Il processo 0 deve rimanere nel ciclo per coordinare gli altri. */
        BOOL b_continue = TRUE;
        if (i_groupSize == 1 && i_rank != 0)
        {
            b_continue = FALSE;
            i_processesInPhase3 = 1;
        }

        MPI_Isend(&b_continue, 1, MPI_INT, 0, CONTINUE_TAG, MPI_COMM_WORLD, &reqContinue);

        /* Il processo 0 aggiorna il conto di quanti processi sono ancora nella fase 3. */
        if (i_rank == 0)
        {
            int i;
            int i_maxIter = i_processesInPhase3;
            for (i = 0; i < i_maxIter; ++i)
            {
                BOOL b_contTemp;
                MPI_Recv(&b_contTemp, 1, MPI_INT, MPI_ANY_SOURCE, CONTINUE_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                if (b_contTemp == FALSE)
                {
                    --i_processesInPhase3;
                }
            }
        }

        /* Se il processo resta nella fase 3, prepara la prossima iterazione. */
        /* Creazione del nuovo comunicatore. */
        MPI_Comm newcommunicator;
        MPI_Comm_split(communicator, i_rootProcess, 0, &newcommunicator);
        MPI_Comm_free(&communicator);
        communicator = newcommunicator;
        MPI_Comm_rank(communicator, &i_currentRank);

        if (b_continue && i_rank == i_rootProcess && i_rank != 0)
        {
            free(ar_currentAr);
            ar_currentAr = malloc(sizeof(int) * i_currentSize);
            if (ar_currentAr == NULL)
            {
                printf("Allocazione fallita (processo %d).\n", i_rank);
                return NULL;
            }
        }

        /* Comunica al processo 0 i root della prossima iterazione. */
        if (i_rank == i_rootProcess && b_continue && i_rank != 0)
        {
                MPI_Isend(&i_rank, 1, MPI_INT, 0, ROOT_TAG, MPI_COMM_WORLD, &reqContinue);
                MPI_Isend(&i_startIndex, 1, MPI_INT, 0, START_INDEX_TAG, MPI_COMM_WORLD, &reqStartIndex);
                MPI_Isend(&i_currentSize, 1, MPI_INT, 0, SECTION_LENGTH_TAG, MPI_COMM_WORLD, &reqLen);
                /* Ricevi il prossimo array da elaborare. */
                MPI_Irecv(ar_currentAr, i_currentSize, MPI_INT, 0, NEW_ARRAY_TAG, MPI_COMM_WORLD, &reqContinue);
        }
        else
        {
            int temp = -1;
            MPI_Isend(&temp, 1, MPI_INT, 0, ROOT_TAG, MPI_COMM_WORLD, &reqContinue);
        }

        if (i_rank == 0)
        {
            /* Riceve i root della prossima iterazione. */
            int i;
            int i_newRootsNumber = i_rootsNumber;
            int i_maxIter = i_rootsNumber << 1;
            ++currentChar;
            for (i = 1; i < i_maxIter; ++i)
            {
                int i_index;
                MPI_Recv(&i_index, 1, MPI_INT, MPI_ANY_SOURCE, ROOT_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                if (i_index != -1)
                {
                    ++i_newRootsNumber;
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

                    MPI_Isend(&ar[i_start], i_len, MPI_INT, i, NEW_ARRAY_TAG, MPI_COMM_WORLD, &reqContinue);
                }
            }
        }

        if (b_continue && i_rank == i_rootProcess)
        {
            MPI_Wait(&reqContinue, MPI_STATUS_IGNORE);
        }
    } while(i_processesInPhase3 > 1);

    if (i_rank == 0)
    {
        printf("\033[0;31m");
        BOOL check = FALSE;
        int i;
        for (i = 0; i < i_arSize / BLOCK_SIZE; ++i)
        {
            int j;
            for (j = 0; j < BLOCK_SIZE; ++j)
            {
                int nextIndex = (BLOCK_SIZE * i) + j;
                if (!check && nextIndex == i_splitPoint)
                {
                    printf("\033[0;32m");
                    check = TRUE;
                }
                printf("%3d ", ar[nextIndex]);
            }
            printf("\n");
        }
        printf("\033[0m");
    }

    return ar;
}
