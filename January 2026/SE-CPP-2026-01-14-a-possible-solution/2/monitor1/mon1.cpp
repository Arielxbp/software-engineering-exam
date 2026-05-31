#include "mon1.h"
#include "system.h"



Mon1::Mon1(Global *myp, const char *mysysname)
{
 
  p = myp;
  strcpy(sysname, mysysname);
  pid = p -> first_available_pid;
  p -> dns.insert({sysname, pid});
  // update free pids
  p -> first_available_pid = p -> first_available_pid + 1;

  // define event type
  p -> event = 0;

  
} // UAV()

int Mon1::adr(int i, int j)
{
  return((p -> prm.N)*i + j);
}

/* initial state */
void Mon1::init() {
  int i, j;
  double rval;

  collisions = 0;
  collision_rate = 0;
  time = (p -> prm.T)/(2.0);

}  // init()



void Mon1::next_state(double present_time) {
  int i, j;
  
 if (present_time >= time)
    {

#if (DEBUG > 1000)
  fprintf(stderr, "Mon1::sampling(): Collision Monitor sampling time  = %lf\n", present_time);
#endif

  for (i=0; i < (p -> prm.N); i++)
    for (j=i+1; j < (p -> prm.N); j++)
      {
	if (distance(i, j) <= (p -> prm.D))
	  // collision
	  {
	    collisions++;
	  }
      }

     // time of next sampling
     time = time + (p -> prm.R);

     assert(time > 0);
     collision_rate = ((double) collisions)/time ;
    }
 
 
} // sampling()


double Mon1::distance(int i, int j)
{
  double value = 0;
  int k;
  
  for (k = 0; k < 3; k++)
    {
      //      value = value + pow((x[adr(i, k)] - x[adr(j, k)]), 2);
      value = value + pow(((p -> sys -> uav1[i] -> x[k]) -
			   (p -> sys -> uav1[j] -> x[k])), 2);
    }

  value = sqrt(value);

  return(value);
}

  
