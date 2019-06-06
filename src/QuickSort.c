/* Implementazione di QuickSort. */
#include "QuickSort.h"

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <mpi.h>

SIDE neutralize(int* ar, int i_left, int i_right, int i_pivot, int* iPtr_remainingBlock)
{
	int i_leftLimit, i_rightLimit, i_oldLeft, i_oldRight;
	i_leftLimit = i_left + BLOCK_SIZE;
	i_rightLimit = i_right + BLOCK_SIZE;
	i_oldLeft = i_left;
	i_oldRight = i_right;

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
			*iPtr_remainingBlock = -1;
			return BOTH;
		}
		*iPtr_remainingBlock = i_oldRight;
		return LEFT;
	}
	*iPtr_remainingBlock = i_oldLeft;
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

	int* ar_remainingBlocks = NULL;
	SIDE* ar_neutralizedSides = NULL;
	int ar_params[2];
	int* ar_sndParams = NULL;

	if (i_rank == 0)
	{
		ar_remainingBlocks = malloc(sizeof(int) * i_totalProcesses);
		/* Inizializza a zero in modo che alla prima iterazione vengano dati due blocchi a ogni processore. */
		ar_neutralizedSides = calloc(i_totalProcesses, sizeof(SIDE));
		ar_sndParams = malloc(sizeof(int) * (i_totalProcesses << 1));

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
	}
	MPI_Bcast(&i_pivot, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(&ar_remainingBlocks, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(&ar_neutralizedSides, 1, MPI_INT, 0, MPI_COMM_WORLD);

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
					i_leftBlock += BLOCK_SIZE;
					ar_sndParams[++j] = i_rightBlock;
					i_rightBlock -= BLOCK_SIZE;
					ar_remainingBlocks[i] = -1;
					++j;
					i_LN += BLOCK_SIZE;
					i_RN -= BLOCK_SIZE;
				}
				else if (ar_neutralizedSides[i] == LEFT)
				{
					ar_sndParams[j] = i_leftBlock;
					i_leftBlock += BLOCK_SIZE;
					j += 2;
					ar_remainingBlocks[i] = RIGHT;
					i_LN += BLOCK_SIZE;
				}
				else
				{
					ar_sndParams[++j] = i_rightBlock;
					i_rightBlock -= BLOCK_SIZE;
					++j;
					ar_remainingBlocks[i] = LEFT;
					i_RN -= BLOCK_SIZE;
				}
			}
		}
		MPI_Scatter(ar_sndParams, 2, MPI_INT, ar_params, 2, MPI_INT, 0, MPI_COMM_WORLD);
		ar_neutralizedSides[i_rank] = neutralize(ar, ar_params[0], ar_params[1], i_pivot, (ar_remainingBlocks + i_rank));
	}
	MPI_Barrier(MPI_COMM_WORLD);
}
