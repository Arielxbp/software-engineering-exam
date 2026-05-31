#include "monitor.h"
#include "system.h"
#include "uav.h"


// initialize system state
void Monitor::init()
{ 

  Collisions = 0;
  Collision_rate = 0;

}


void Monitor::update(int mypid)
{

#if (DEBUG > 500)
  fprintf(stderr, "Monitor::update(): mypid = %d\n", mypid);
#endif

  char buf[100];
  
  int i, j, cpid;
  
  for (j = 1; j <= p -> prm.N; j++)
    {
      sprintf(buf, "uav%d", j);
      cpid = p -> dns[buf];
      if (cpid > mypid)
	// check collision
	{
          if (distance(mypid, cpid) < p -> prm.D)
	    {
	      Collisions++;
	    } 
	}
    } // for j


  Collision_rate = ((double) Collisions)/(p -> sys -> time);

#if (DEBUG > 500)
  fprintf(stderr, "Monitor::update(): Collisions=%d, Collisions_rate=%lf, time=%lf\n",
	  Collisions, Collision_rate, p -> sys -> time);
#endif
  
} // update()



double Monitor::distance(int mypid, int farpid)
{
  int i;
  double value = 0;

   // 1. Recupera il unique_ptr dal vettore 
   auto& basePtr1 = p->sys->sysproc[mypid];
   auto& basePtr2 = p->sys->sysproc[farpid];

    // 2. Esegui il downcast a UAV* 
  UAV* Ptr1 = static_cast<UAV*>(basePtr1.get());
  UAV* Ptr2 = static_cast<UAV*>(basePtr2.get());

   // 3. Chiama il metodo 
   // monitorPtr1 -> ..... 
   // monitorPtr12 -> ..... 

   
  for (i = 0; i < 3; i++)
    {
      value = value +
      pow((Ptr1 -> x[i]) -
	  (Ptr2 -> x[i]), 2);
    }
  
  return (sqrt(value));
	  
}
