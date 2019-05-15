/* Programma principale, avvia i due algoritmi di sorting. */
#include <mpi.h>

#include "BitonicSort.h"
#include "QuickSort.h"

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
BOOL CheckSorting(int* ar, int i_arSize);


/**********MAIN**********/
int main(int argc, char* argv[])
{
  MPI_Init(&argc, &argv);



  MPI_Finalize();
  return 0;
}


BOOL CheckSorting(int* ar, int i_arSize)
{
  int i = 0;
  for (i; i < i_arSize-1; i++)
  {
    if (ar[i] > ar[i+1]) return FALSE;
  }
  return TRUE;
}
