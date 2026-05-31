#include "mon1.h"
#include "system.h"



Mon1::Mon1(Global *myp, const char *mysysname)
{
  // not strictly needed since Mon1 is not a process
  p = myp;
  strcpy(sysname, mysysname);
  pid = -3;

//monitor is not  a process
#if 0
  pid = p -> first_available_pid;
  p -> dns.insert({sysname, pid});
  // update free pids
  p -> first_available_pid = p -> first_available_pid + 1;
#endif
  
} // Mon1


// initialize system state
void Mon1::init()
{ 
  int i;

#if (DEBUG > 1000)
  fprintf(stderr, "System::init(): begin\n");
#endif
  
  MissedSells = 0;
  MissedSells_rate = 0;
  
}


void Mon1::update(int asked, int received, double ptime)
{

#if (DEBUG > 5)
  fprintf(stderr, "Mon1::update(): asked=%d, received=%d, ptime=%lf\n",
	  asked, received, ptime);
#endif


  if (asked > received)
  // missed sell
  {
    MissedSells++;
    MissedSells_rate =  ((double) MissedSells)/ptime;
  }

#if (DEBUG > 5)
  fprintf(stderr, "Mon1::update(): MissedSells=%d, MissedSells_rate=%lf, ptime=%lf\n",
	  MissedSells, MissedSells_rate, ptime);
#endif
  
} // update()


