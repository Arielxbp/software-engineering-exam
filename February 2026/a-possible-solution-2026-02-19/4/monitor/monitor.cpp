#include "monitor.h"
#include "system.h"


// initialize system state
void Monitor::init()
{ 

  MissedSells = 0;
  MissedSells_rate = 0;
  p -> transaction = 0;
  
}


void Monitor::update(int asked, int received, double ptime)
{

#if (DEBUG > 5)
  fprintf(stderr, "Monitor::update(): asked=%d, received=%d, ptime=%lf\n",
	  asked, received, ptime);
#endif


  if (asked > received)
  // missed sell
  {
    //MissedSells++;
    MissedSells = MissedSells + (asked - received);
    MissedSells_rate =  ((double) MissedSells)/ptime;
  }

#if (DEBUG > 5)
  fprintf(stderr, "Monitor::update(): MissedSells=%d, MissedSells_rate=%lf, ptime=%lf\n",
	  MissedSells, MissedSells_rate, ptime);
#endif
  
} // update()

