#include "uav.h"


UAV::UAV(Global *myp, const char *mysysname)
{
#if (DEBUG > 1000)
  fprintf(stderr, "UAV::UAV(0x%lx, %s): begin\n",
	  (long unsigned int) p, mysysname);
#endif
  
  p = myp;
  strcpy(sysname, mysysname);
  pid = p -> first_available_pid;
  p -> dns.insert({sysname, pid});
  // update free pids
  p -> first_available_pid = p -> first_available_pid + 1;
    
  // create structures

  x.resize(3);

#if (DEBUG > 1000)
  fprintf(stderr, "UAV::UAV(p, %s): end\n", mysysname);
#endif  
} // UAV()



/* initialize structures and state */
void UAV::init() {
  int j;
  
  /*
    Al tempo t = 0 i valori xk,i (0) vengono scelti uniformemente a random
nell’intervallo [−L, L].
  */

    for (j=0; j < 3; j++)
      {
	x[j] = -(p -> prm.L)  + (2*(p -> prm.L))*(*(p -> ptr_UnifRealDist))(p -> RandomEngine);
      }
  
  
  time = 0;
  time_state = p -> prm.T;
 
  // set time of next event
  time = time + time_state;

   // define event type
  p -> event = 0;
  
}  // init()



void UAV::next_state(double present_time) {
  
  if (present_time < time) return;
    
  assert(present_time >= time);
  
   // define event type
  p -> event = 0;
  
#if (DEBUG > 1000)
  fprintf(stderr, "UAV::next_state(): UAV moves, time  = %lf\n", present_time);
#endif

      update_state();
      
      time = time + time_state;
      
} // next()


void UAV::update_state()
 {
   int i, j;

   for (j=0; j < 3; j++)
      {
	x[j] =  x[j] + compute_velocity(j)*time_state;
      }


 } // update_state()
 

double UAV::compute_velocity(int j)
 {
   double prob;
     
  // p in [0, 1] at random
  prob = (*(p -> ptr_UnifRealDist))(p -> RandomEngine);

  if (prob <= compute_probability(j))
	  {
 	    return(p -> prm.V);
	  }
	else
	  {
	    return(-(p -> prm.V));
	  }
      

 } // update_state()
 

double UAV::compute_probability(int j)
 {
    
   return (exp(-(p -> prm.A)*((x[j] + (p -> prm.L))/(2*(p -> prm.L)))));
     
 } // compute_probability()


