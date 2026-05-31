#include "system.h"

#include "uav.h"
#include "monitor.h"


/* create system  */
void System::create()
{ 
  int numproc;
   
  // calcola quanti processi ci sono
  numproc = (p -> prm.N) + 1;

#if (DEBUG > 1000)
      fprintf(stderr, "System::create(): numproc = %d \n", numproc);
#endif

      // riserva spazio nel vettore dei processi
  sysproc.resize(numproc); // Alloca la memoria tutta in una volta

  pidinit();   // init pid

  // aggiungi i processi

#if (DEBUG > 1000)
      fprintf(stderr, "System::create(): creating %d UAV\n", p->prm.N);
#endif

      create_processes<UAV>(p->prm.N, "uav");
      

#if (DEBUG > 1000)
      fprintf(stderr, "System::create(): creating %d Monitors\n", 1);
#endif

      create_processes<Monitor>(1, "monitor");

#if (DEBUG > 1000)
      fprintf(stderr, "System::create(): created %d processes: end\n", numproc);
#endif     
 
}



