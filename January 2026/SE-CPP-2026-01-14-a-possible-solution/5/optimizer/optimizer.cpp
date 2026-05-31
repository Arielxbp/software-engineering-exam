#include "optimizer.h"

/*

Thread 0: send order
Thread 1: check delivery

*/

Optimizer::Optimizer(Global *myp)
{
  p = myp;
} // Optimizer()


void Optimizer::init(Simulator *mysim, MonteCarlo *mymc)
{
  sim = mysim;
  mc = mymc;
  
  minsol = (p -> prm.A)/2;
  maxsol = (p -> prm.B)*2;
  
} // MonteCarlo()

void Optimizer::objfun()
{
  // montecarlo simulation
   mc -> run("logfile.csv");

   p -> R_opt = p -> avg;
   p -> J_opt = (p -> prm.a)*(p -> prm.V) + (p -> prm.b)*(p -> R_opt);
   
   return;
}


/* initial state */
void Optimizer::minimize(int Budget) {
  int i;

  double best_obj_so_far = DBL_MAX;
  double best_sol_so_far;
  double best_rate_so_far;
  double z;
  
  for (i = 0; i < Budget; i++)
    {

      // pick solution at random

      // real values
      p -> prm.V = minsol +
      (maxsol - minsol)*(*(p -> ptr_UnifRealDist))(p -> RandomEngine);
      p -> prm.W = (p -> prm.V) + 5;

#if 0
      // int values
      p -> prm.F =
 minsol + (*(p -> ptr_UnifIntDist))(p -> RandomEngine)%(maxsol - minsol + 1);
#endif
      
      
      objfun();
       
      if (p -> J_opt < best_obj_so_far)
	{
	  best_sol_so_far = p -> prm.V;
	  best_obj_so_far = p -> J_opt;
          best_rate_so_far = p -> R_opt;
	  
#if (DEBUG > 10)
      fprintf(stderr, "Optimizer::minimize(%d): best_sol_so_far = %lf, best_obj_so_far = %lf, best_rate_so_far = %lf\n",
	      i, p -> prm.V, p -> J_opt, p -> R_opt);
#endif
	  
	}
    }
  
  // store opt results
  p -> prm.V = best_sol_so_far;
  p -> prm.W = (p -> prm.V) + 5;
  p -> J_opt = best_obj_so_far;
  p -> R_opt = best_rate_so_far;

#if (DEBUG > 10)
      fprintf(stderr,
	      "Optimizer::minimize():end:  best_sol_so_far = %lf, best_obj_so_far = %lf, best_rate_so_far = %lf\n",
	      p -> prm.V, p -> J_opt, p -> R_opt);
#endif  
}
