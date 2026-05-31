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
  return (sim -> sys -> monitor1[0] -> collision_rate);
}


/* initial state */
void MonteCarlo::run(const char *logname) {
  int i;
  double sample_value;
   
  p -> avg = 0;
  
  for (i = 0; i < p -> prm.M; i++)
    {
      sim -> run(logname);
      sample_value = get_sample_value();
      
      p -> avg = (p -> avg)*(((double) i)/((double) (i+1)))
	         +
	(sample_value)/((double) (i+1));

#if (DEBUG > 1000)
      fprintf(stderr, "MonteCarlo::run(%d): sample_value = %lf, avg so far = %lf\n",
	      i, sample_value, p -> avg);
#endif	       
    }

#if (DEBUG > 1000)
      fprintf(stderr, "MonteCarlo::run(%d): sample_value = %lf, avg so far = %lf, M = %d\n",
	      i, sample_value, p -> avg, p -> prm.M);
#endif


      
  
}  // run()
