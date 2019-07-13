/* Implementazione di BitonicSort. */

#include <stdio.h>
#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "mpi.h"
#include "BitonicSort.h"

#define MASTER 0        /* Master process ID */
#define MAX_RAND 100    /* Random numbers generated in range [0, MAX_RAND] */

/*Compare two values and swap base on direction:
true -> swap if a > b
false -> swap if a <= b*/
void compare(int *a, int *b, bool dir)
{
    if(dir == (*a > *b))
    {
        int t = *a;
        *a = *b;
        *b = t;
    }
}

/*Sort a sequence in ascending order if dir = 1 or descending if dir = 0*/
void bitonicMerge(int a[], int low, int dim, bool dir)
{
  if (dim > 1)
  {
    int k = dim / 2;
    for (int i = low; i < low + k; i++)
      compare(&a[i], &a[i+k], dir);
    bitonicMerge(a, low, k, dir);
    bitonicMerge(a, low + k, k, dir);
  }
}


void bitonicSort(int a[], int low, int dim, bool dir)
{
    if (dim > 1)
    {
        int k = dim / 2;

        /* sort in ascending order */
        bitonicSort(a, low, k, 1);
        /* sort in descending order */
        bitonicSort(a, low + k, k, 0);

        bitonicMerge(a, low, dim, dir);
    }
}

/*Sort an array in ascending order using bitonc sort*/
void sort(int a[], int dim)
{
    bitonicSort(a, 0, dim, 1);
}

void mergeAndSplit(int in[], int rank, int r_min, int r_max, int num_keys)
{
  int val;
  MPI_Status status;

  if(rank == r_min)
  {
    /* Min polarity */
    MPI_Send(&in[num_keys - 1], 1, MPI_INT, r_max, 0, MPI_COMM_WORLD);
    MPI_Recv(&val, 1, MPI_INT, r_max, 0, MPI_COMM_WORLD, &status);

    /* Local Detection */
    int c;
    for(c = 0; in[c] < val && c < num_keys; c++)
    { /*Move cursor to the position nearest val*/ }

    int buf_size = num_keys - c;
    int buf[buf_size];
    memcpy(buf, &in[c], (buf_size) * sizeof(*in));

    /* Partial Exchange */
    int ex[num_keys];
    MPI_Send(buf, buf_size, MPI_INT, r_max, 0, MPI_COMM_WORLD);
    MPI_Recv(ex, num_keys, MPI_INT, r_max, 0, MPI_COMM_WORLD, &status);

    int ex_size;
    MPI_Get_count(&status, MPI_INT, &ex_size);

    int i, j = 0, k = 0;
    /* Keep lowest values */
    for(i = num_keys - buf_size; i < num_keys && j < buf_size && k < ex_size; i++)
    {
      if(buf[j] < ex[k])
        in[i] = buf[j++];
      else
        in[i] = ex[k++];
    }
    for(; i < num_keys && j < buf_size; i++)
      in[i] = buf[j++];
    for(; i < num_keys && k < ex_size; i++)
      in[i] = ex[k++];
  }
  else if(rank == r_max)
  {
    /* Max polarity */
    MPI_Send(&in[0], 1, MPI_INT, r_min, 0, MPI_COMM_WORLD);
    MPI_Recv(&val, 1, MPI_INT, r_min, 0, MPI_COMM_WORLD, &status);

    /* Local Detection */
    int c;
    for(c = 0; in[c] <= val && c < num_keys; c++)
    { /*Move cursor to the position nearest val*/ }

    int buf_size = c;
    int buf[buf_size];
    memcpy(buf, &in[0], (c) * sizeof(*in));

    /* Partial Exchange */
    int ex[num_keys];
    MPI_Send(buf, buf_size, MPI_INT, r_min, 0, MPI_COMM_WORLD);
    MPI_Recv(ex, num_keys, MPI_INT, r_min, 0, MPI_COMM_WORLD, &status);

    int ex_size;
    MPI_Get_count(&status, MPI_INT, &ex_size);

    int i, j = buf_size - 1, k = ex_size - 1;
    /* Keep highest values */
    for(i = buf_size - 1; i >= 0 && j >= 0 && k >= 0; i--)
    {
      if(buf[j] > ex[k])
        in[i] = buf[j--];
      else
        in[i] = ex[k--];
    }
    for(; i >= 0 && j >= 0; i--)
      in[i] = buf[j--];
    for(; i >= 0 && k >= 0; i--)
      in[i] = ex[k--];
  }
  else
  {
    printf("Input rank error!");
    MPI_Abort(MPI_COMM_WORLD, 1);
  }
}

int main(int argc, char *argv[])
{
    int rank, size;
    int arraySize;
    double timerStart, timerEnd;

    if(argc > 1)
    {
      arraySize = atoi(argv[1]);
      if(!((arraySize &(arraySize - 1)) == 0))
      {
        printf("Array size must be a power of two!\n");
        return 0;
      }
    }
    else
    {
      printf("Usage: ./BitonicSort array_size (power of two)\n");
      return 0;
    }

    int array[arraySize];

    /* Initialize */
    MPI_Init(&argc, &argv);
    /* Get total number of processes */
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    /* Get this process rank */
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int num_keys = arraySize / size;

    if(arraySize % size != 0 || size % 2 != 0)
    {
      if(rank == MASTER)
        printf("Number of processors must be a multiple of two\n");
      MPI_Finalize();
      return 0;
    }

    if(rank == MASTER)
    {
      /* Reset Seed */
      srand(time(NULL));
      /* Generate random numbers in range [0, MAX_RAND] */
      for(int i = 0; i < arraySize; i++)
        array[i] = rand() % MAX_RAND;
    }

    int in[num_keys];

    /* Send to each process its input for Bitonic Sorting */
    MPI_Scatter(
      array,
      num_keys,
      MPI_INT,
      &in,
      num_keys,
      MPI_INT,
      MASTER,
      MPI_COMM_WORLD);

    MPI_Barrier(MPI_COMM_WORLD);

    /* Now each processor has its input data, the simulation can begin */

    /*Each processor sequentially sort its input array using
    bitonic sorting*/
    sort(in, num_keys);

    MPI_Barrier(MPI_COMM_WORLD);

    for (int k = 2; k <= size; k *= 2)
    {
      for (int j = k / 2; j > 0; j /= 2)
      {
        for (int i = 0; i < size; i++)
        {
          int rankxj = rank^j;
          if ((rankxj) > rank)
          {
            if ((rank&k)==0)
              mergeAndSplit(in, rank, rank, rankxj, num_keys);
            if ((rank&k)!=0)
              mergeAndSplit(in, rank, rankxj, rank, num_keys);
          }
          else
          {
            if ((rank&k)==0)
              mergeAndSplit(in, rank, rankxj, rank, num_keys);
            if ((rank&k)!=0)
              mergeAndSplit(in, rank, rank, rankxj, num_keys);
          }
        }
      }
    }

    MPI_Gather(
      in,
      num_keys,
      MPI_INT,
      array,
      num_keys,
      MPI_INT,
      MASTER,
      MPI_COMM_WORLD);

    if(rank == MASTER)
    {
      for(int i = 0; i < arraySize; i++)
        printf("%d ", array[i]);
      printf("\n\n");
    }

    MPI_Finalize();

    return 0;
}
