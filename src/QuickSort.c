/* Implementazione di QuickSort. */

#include "QuickSort.h"

SIDE neutralize(int* ar, int i_left, int i_right, int i_pivot)
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

void quickSortManager(int* ar, int i_arSize, int i_totalProcesses)
{
	
}
