/* Implementazione di QuickSort. */
#include "QuickSort.h"

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <mpi.h>

SIDE neutralize(int* ar, int i_left, int i_right, int i_pivot, SIDE* ar_result, int i_rank)
{
	int i_leftLimit, i_rightLimit;
	i_leftLimit = i_left + BLOCK_SIZE;
	i_rightLimit = i_right + BLOCK_SIZE;

	do
	{
		/* Trova il primo elemento maggiore del pivot. */
		while (i_left < i_leftLimit && ar[i_left] <= i_pivot)
		{
			++i_left;
		}

		/* Trova il primo elemento minore del pivot. */
		while (i_right < i_rightLimit && ar[i_right] >= i_pivot)
		{
			++i_right;
		}

		/* Se ci sono due elementi nel posto sbagliato, invertili. */
		if (i_left < i_leftLimit && i_right < i_rightLimit)
		{
			int temp;
			temp = ar[i_left];
			ar[i_left] = ar[i_right];
			ar[i_right] = temp;
			++i_left;
			++i_right;
		}
	} while(i_left < i_leftLimit && i_right < i_rightLimit);

	/* Verifica quale lato e' stato neutralizzato e ritorna. */
	if (i_left == i_leftLimit)
	{
		if (i_right == i_rightLimit)
		{
			return BOTH;
		}
		return LEFT;
	}
	return RIGHT;
}

void quickSortManager(int* ar, int i_arSize, int i_rank, int i_totalProcesses)
{
	MPI_Barrier(MPI_COMM_WORLD);

	int i_LN, i_RN, i_leftBlock, i_rightBlock;
	i_LN = 0;
	i_RN = i_arSize;
	i_leftBlock = 0;
	i_rightBlock = i_arSize - BLOCK_SIZE;

	int* ar_remainingBlocks = NULL;
	SIDE* ar_neutralizedSides = NULL;
	int ar_params[2];
	int* ar_sndParams = NULL;

	if (i_rank == 0)
	{
		ar_remainingBlocks = malloc(sizeof(int) * i_totalProcesses);
		ar_neutralizedSides = malloc(sizeof(SIDE) * i_totalProcesses);
		ar_sndParams = malloc(sizeof(int) * (i_totalProcesses << 1));
	}
	MPI_Bcast(&ar_remainingBlocks, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(&ar_neutralizedSides, 1, MPI_INT, 0, MPI_COMM_WORLD);

	while (i_leftBlock < i_rightBlock)
	{
		if (i_rank == 0)
		{
			int i, j;
			j = 0;
			for (i = 0; i < i_totalProcesses; i++)
			{
				if (ar_neutralizedSides[i] == LEFT || ar_neutralizedSides[i] == BOTH)
				{
					ar_sndParams[j] = i_leftBlock;
					i_leftBlock += BLOCK_SIZE;
				}
				++j;
				if (ar_neutralizedSides[i] == RIGHT || ar_neutralizedSides[i] == BOTH)
				{
					ar_sndParams[j] = i_rightBlock;
					i_rightBlock -= BLOCK_SIZE;
				}
				++j;
			}
		}

		neutralize()
	}

	MPI_Barrier(MPI_COMM_WORLD);

	int i_pivot;
	int i_blockSize;
	i_blockSize = i_arSize/(i_totalProcesses*2);
	printf("Block size: %d (process %d)\n", i_blockSize, i_rank);
	printf("Total processes: %d (process %d)\n", i_totalProcesses, i_rank);

	int ar_rcvParams[4];
	int* ar_sndParams = NULL;
	if (i_rank == 0)
	{
		srand(time(NULL));
		int i_1, i_2, i_3, i_min, i_max;
		i_1 = rand() % i_arSize;
		i_2 = rand() % i_arSize;
		i_3 = rand() % i_arSize;
		i_1 = ar[i_1];
		i_2 = ar[i_2];
		i_3 = ar[i_3];

		printf("Values: %d, %d, %d; (process %d)\n", i_1, i_2, i_3, i_rank);

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

		printf("Min, max, pivot: %d, %d, %d; (process %d)\n", i_min, i_max, i_pivot, i_rank);

		ar_sndParams = malloc(sizeof(int) * i_totalProcesses * 4);
		if (ar_sndParams == NULL) return;
		int i, j;
		j = 0;

		for (i = 0; i < i_totalProcesses; i++)
		{
			ar_sndParams[j] = i_blockSize*i;
			ar_sndParams[++j] = i_blockSize/BLOCK_SIZE;
			ar_sndParams[++j] = i_arSize - i_blockSize*(i+1);
			ar_sndParams[++j] = i_blockSize/BLOCK_SIZE;
			++j;
		}
	}
	MPI_Bcast(&i_pivot, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Scatter(ar_sndParams, 4, MPI_INT, ar_rcvParams, 4, MPI_INT, 0, MPI_COMM_WORLD);
	phaseOne(ar, ar_rcvParams[0], ar_rcvParams[1], ar_rcvParams[2], ar_rcvParams[3], i_pivot, i_rank);
}

int phaseOne(int* ar, int i_leftStart, int i_leftBlocks, int i_rightStart, int i_rightBlocks, int i_pivot, int i_rank)
{
	printf("phaseOne: %d, %d, %d, %d\n", i_leftStart, i_leftBlocks, i_rightStart, i_rightBlocks);
	int i_leftBlock, i_rightBlock;
	i_leftBlock = i_leftStart;
	i_rightBlock = i_rightStart + i_rightBlocks * BLOCK_SIZE;

	BOOL done;
	done = FALSE;
	while (!done)
	{
		SIDE neutralizedSide;
		neutralizedSide = neutralize(ar, i_leftBlock, i_rightBlock, i_pivot);

		if (neutralizedSide == LEFT || neutralizedSide == BOTH)
		{
			i_leftBlock += BLOCK_SIZE;
			if (i_leftBlock > i_leftStart + i_leftBlocks*)
		}
	}


	return 0;
}
