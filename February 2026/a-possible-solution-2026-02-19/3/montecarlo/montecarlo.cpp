#include "montecarlo.h"

/*

Thread 0: send order
Thread 1: check delivery

*/


 MonteCarlo::MonteCarlo(Global *myp, Simulator *mysim)
{
  p = myp;
  sim = mysim;

} // MonteCarlo()

double MonteCarlo::get_sample_value()
{

  double value;
  
  // 1. Recupera il unique_ptr dal vettore 
  auto& basePtr = p->sys->sysproc[p->dns["monitor1"]];

  // 2. Esegui il downcast a Monitor* 
  Monitor* monitorPtr = static_cast<Monitor*>(basePtr.get());

   // 3. Chiama il metodo 
   value = monitorPtr->MissedSells_rate;

  return (value);
}


/* initial state */
void MonteCarlo::run(const char *logname) {
  int i;
  double sample_value;
   
#if (DEBUG > 1)
      fprintf(stderr, "MonteCarlo::run(): C = %d, F = %d\n",
	      p -> prm.C, p -> prm.F);
#endif


      p -> avg = 0;
  
  for (i = 0; i < p -> prm.M; i++)
    {
      sim -> run(logname);
      sample_value = get_sample_value();
      
      p -> avg = (p -> avg)*(((double) i)/((double) (i+1)))
	         +
	(sample_value)/((double) (i+1));

#if (DEBUG > 1)
      fprintf(stderr, "MonteCarlo::run(%d): sample_value = %lf, avg so far = %lf\n",
	      i, sample_value, p -> avg);
#endif	       
    }

#if (DEBUG > 1)
      fprintf(stderr, "MonteCarlo::run(%d): sample_value = %lf, avg so far = %lf, M = %d\n",
	      i, sample_value, p -> avg, p -> prm.M);
#endif


      
  
}  // run()
