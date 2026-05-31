#include "uav.h"
#include "system.h"


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
      
#if (DEBUG > 1000)
  fprintf(stderr, "UAV::next_state(): update_state done, time  = %lf\n", present_time);
#endif

  time = time + time_state;
      
} // next()


void UAV::update_state()
 {
   int i, j;
   
#if (DEBUG > 1000)
  fprintf(stderr, "UAV::update_state(): begin\n");
#endif

  action(x, velocity);
     
#if (DEBUG > 1000)
  fprintf(stderr, "UAV::update_state(): action done\n");
#endif

  for (j=0; j < 3; j++)
      {
	x[j] =  x[j] + velocity[j]*time_state;
      }

#if (DEBUG > 1000)
  fprintf(stderr, "UAV::update_state(): end\n");
#endif

 } // update_state()
 

/*
****************************
CONTROLLER
****************************
*/

double UAV::distance(vector<double> z)
{
  double value = 0;
  double tmp;
  int j, k;
  
  for (j = 0; j < p -> prm.N; j++)
	 for (k = 0; k < 3; k++)
    {
      //      value = value + pow((x[adr(i, k)] - x[adr(j, k)]), 2);
      tmp = (z[k] - p -> sys -> uav1[j] -> x[k])/(2*(p -> prm.L));      
      value = value + pow(tmp, 2);
    }

  return(value);
}  // distance()

  
double UAV::cost(vector<double> present_state, double V[3])
{
  double value = 0;
  double tmp;
  int j, k;
  vector<double> z;

  z.resize(3);
  
   for (k = 0; k < 3; k++)
    {
      z[k] = present_state[k] + V[k]*(p -> prm.T);
    }
    

   return(distance(z));
   
}  // cost()


void UAV::action(vector<double> present_state, double best_sol[3])
{
  double value = 0;
  double best_value;
  double tmp;
  int a, b, c;
  double z[3];
  double Vel[3];
  
#if (DEBUG > 1000)
  fprintf(stderr, "UAV::action(): begin\n");
#endif

  best_value = DBL_MAX;
 
   for (a = 0; a < 2; a++)
   for (b = 0; b < 2; b++)
   for (c = 0; c < 2; c++)
     {
       Vel[0] = -(p -> prm.V) + (a*2*(p -> prm.V));
       Vel[1] = -(p -> prm.V) + (b*2*(p -> prm.V));
       Vel[2] = -(p -> prm.V) + (c*2*(p -> prm.V));
       
       value = cost(present_state, Vel);

       if (value < best_value)
	 {
           best_value = value;
	   best_sol[0] = Vel[0];
	   best_sol[1] = Vel[1];
	   best_sol[2] = Vel[2];	   
	 }
       
       
     }

#if (DEBUG > 1000)
  fprintf(stderr, "UAV::action(): end\n");
#endif
  
} // action();
