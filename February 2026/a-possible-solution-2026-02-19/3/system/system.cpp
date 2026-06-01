#include "system.h"

#include "customer.h"
#include "server.h"
#include "db.h"
#include "supplier.h"

#include "monitor.h"


/* create system  */
void System::create()
{ 
  int numproc;
   
   // calcola quanti processi ci sono
  numproc = (p -> prm.C) + (p -> prm.S) + (p -> prm.F) + 1 + 1;


#if (DEBUG > 1000)
      fprintf(stderr, "System::create(): numproc = %d \n", numproc);
#endif

      // riserva spazio nel vettore dei processi
  sysproc.resize(numproc); // Alloca la memoria tutta in una volta

  pidinit();   // init pid

#if (DEBUG > 1)
      fprintf(stderr, "System::create(): remove elements from dns map\n");
#endif

      p -> dns.clear();
      
  // aggiungi i processi

#if (DEBUG > 1)
      fprintf(stderr, "System::create(): creating %d processes\n", numproc);
#endif

      create_processes<Customer>(p->prm.C, "customer");
      create_processes<Server>(p->prm.S, "server");
      create_processes<Supplier>(p->prm.F, "supplier");
      create_processes<DB>(1, "db");
      create_processes<Monitor>(1, "monitor");
      
#if (DEBUG > 1000)
      fprintf(stderr, "System::create(): created %d processes: end\n", numproc);
#endif     
 
}



