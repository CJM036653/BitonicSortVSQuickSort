/* Implementazione di QuickSort. */
#include "QuickSort.h"

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <mpi.h>

SIDE neutralize(int* ar_left, int* ar_right, int i_pivot)
{
	int i_left, i_right;
	i_left = 0;
	i_right = 0;

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

void quickSortManager(int* ar, int i_arSize, int i_rank, int i_totalProcesses)
{
	/*********** FASE UNO ***********/
	int i_LN, i_RN, i_leftBlock, i_rightBlock, i_pivot;
	i_LN = 0;
	i_RN = i_arSize;
	i_leftBlock = 0;
	i_rightBlock = i_arSize - BLOCK_SIZE;

	int ar_left[BLOCK_SIZE];
	int ar_right[BLOCK_SIZE];
	SIDE lastNeutralizedSide = BOTH;

	int* ar_remainingBlocks = NULL;
	SIDE* ar_neutralizedSides = NULL;
	int* ar_sndParams = NULL;

	MPI_Request reqs[4];
	MPI_Request* reqs0;
	MPI_Status* status;

	if (i_rank == 0)
	{
		ar_remainingBlocks = malloc(sizeof(int) * i_totalProcesses);
		/* Inizializza a zero in modo che alla prima iterazione vengano dati due blocchi a ogni processore. */
		ar_neutralizedSides = calloc(i_totalProcesses, sizeof(SIDE));
		ar_sndParams = malloc(sizeof(int) * (i_totalProcesses << 1));
		reqs0 = malloc(sizeof(MPI_Request) * (i_totalProcesses << 1));
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
	}
	MPI_Bcast(&i_pivot, 1, MPI_INT, 0, MPI_COMM_WORLD);

	while (i_leftBlock < i_rightBlock)
	{
		MPI_Barrier(MPI_COMM_WORLD);
		if (i_rank == 0)
		{
			int i, j;
			j = 0;
			for (i = 0; i < i_totalProcesses; i++)
			{
				if (ar_neutralizedSides[i] == BOTH)
				{
					ar_sndParams[j] = i_leftBlock;
					MPI_Isend(&ar[i_leftBlock], BLOCK_SIZE, MPI_INT, i, j, MPI_COMM_WORLD, &reqs[0]);
					i_leftBlock += BLOCK_SIZE;
					ar_sndParams[++j] = i_rightBlock;
					MPI_Isend(&ar[i_rightBlock], BLOCK_SIZE, MPI_INT, i, j, MPI_COMM_WORLD, &reqs[1]);
					i_rightBlock -= BLOCK_SIZE;
					ar_remainingBlocks[i] = -1;
					++j;
					i_LN += BLOCK_SIZE;
					i_RN -= BLOCK_SIZE;
				}
				else if (ar_neutralizedSides[i] == LEFT)
				{
					ar_sndParams[j] = i_leftBlock;
					MPI_Isend(&ar[i_leftBlock], BLOCK_SIZE, MPI_INT, i, j, MPI_COMM_WORLD, &reqs[2]);
					i_leftBlock += BLOCK_SIZE;
					ar_remainingBlocks[i] = ar_sndParams[++j];
					++j;
					i_LN += BLOCK_SIZE;
				}
				else
				{
					ar_remainingBlocks[i] = ar_sndParams[j];
					ar_sndParams[++j] = i_rightBlock;
					MPI_Isend(&ar[i_rightBlock], BLOCK_SIZE, MPI_INT, i, j, MPI_COMM_WORLD, &reqs[3]);
					i_rightBlock -= BLOCK_SIZE;
					++j;
					i_RN -= BLOCK_SIZE;
				}
			}
		}
		MPI_Bcast(&i_leftBlock, 1, MPI_INT, 0, MPI_COMM_WORLD);
		MPI_Bcast(&i_rightBlock, 1, MPI_INT, 0, MPI_COMM_WORLD);

		if (lastNeutralizedSide == BOTH)
		{
			MPI_Recv(ar_left, BLOCK_SIZE, MPI_INT, MPI_ANY_SOURCE, (i_rank << 1), MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			MPI_Recv(ar_right, BLOCK_SIZE, MPI_INT, MPI_ANY_SOURCE, (i_rank << 1) + 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		}
		else if (lastNeutralizedSide == LEFT)
		{
			MPI_Recv(ar_left, BLOCK_SIZE, MPI_INT, MPI_ANY_SOURCE, (i_rank << 1), MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		}
		else
		{
			MPI_Recv(ar_right, BLOCK_SIZE, MPI_INT, MPI_ANY_SOURCE, (i_rank << 1) + 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		}

		lastNeutralizedSide = neutralize(ar_left, ar_right, i_pivot);
		MPI_Gather(&lastNeutralizedSide, 1, MPI_INT, ar_neutralizedSides, 1, MPI_INT, 0, MPI_COMM_WORLD);

		if (lastNeutralizedSide == BOTH)
		{
			MPI_Isend(ar_left, BLOCK_SIZE, MPI_INT, 0, (i_rank << 1), MPI_COMM_WORLD, &reqs[0]);
			MPI_Isend(ar_right, BLOCK_SIZE, MPI_INT, 0, (i_rank << 1) + 1, MPI_COMM_WORLD, &reqs[1]);
		}
		else if (lastNeutralizedSide == LEFT)
		{
			MPI_Isend(ar_left, BLOCK_SIZE, MPI_INT, 0, (i_rank << 1), MPI_COMM_WORLD, &reqs[0]);
		}
		else
		{
			MPI_Isend(ar_right, BLOCK_SIZE, MPI_INT, 0, (i_rank << 1) + 1, MPI_COMM_WORLD, &reqs[0]);
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
					MPI_Irecv(&ar[ar_sndParams[j]], BLOCK_SIZE, MPI_INT, i, j, MPI_COMM_WORLD, &reqs0[k]);
					++j;
					++k;
					MPI_Irecv(&ar[ar_sndParams[j]], BLOCK_SIZE, MPI_INT, i, j, MPI_COMM_WORLD, &reqs0[k]);
					++j;
					++k;
				}
				else if (ar_neutralizedSides[i] == LEFT)
				{
					MPI_Irecv(&ar[ar_sndParams[j]], BLOCK_SIZE, MPI_INT, i, j, MPI_COMM_WORLD, &reqs0[k]);
					j+=2;
					++k;
				}
				else
				{
					++j;
					MPI_Irecv(&ar[ar_sndParams[j]], BLOCK_SIZE, MPI_INT, i, j, MPI_COMM_WORLD, &reqs0[k]);
					++j;
					++k;
				}
			}
			MPI_Waitall(k, reqs0, status);
		}
	}

	if (i_rank == 0)
	{
		int i;
		for (i = 0; i < i_arSize; i++)
		{
			printf("%d", ar[i]);
		}
		printf("\n");
	}

	MPI_Barrier(MPI_COMM_WORLD);
}
