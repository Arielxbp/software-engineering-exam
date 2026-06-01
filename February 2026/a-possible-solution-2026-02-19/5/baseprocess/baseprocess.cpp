#include "baseprocess.h"


BaseProcess::BaseProcess(Global *myp, int mypid, const char *mysysname)
{
#if (DEBUG > 1000)
  fprintf(stderr, "BaseProcess::BaseProcess(0x%lx, %s): begin\n",
	  (long unsigned int) p, mysysname);
#endif
    
  p = myp;
  strcpy(sysname, mysysname);
  pid = mypid;

#if (DEBUG > 10000)
  fprintf(stderr, "BaseProcess::BaseProcess(%s, %d)\n", mysysname, pid);
#endif
  
  p -> dns.insert({sysname, pid});

  time_of_next_event = 0;  // init time

  
  //create structures, if any
  create();
  
  
#if (DEBUG > 10000)
  fprintf(stderr, "BaseProcess::BaseProcess(p, %s): end\n", mysysname);
#endif  
} // BaseProcess()


/* initialize structures and state */
void BaseProcess::init() {

  // empty input queue
while (!inputqueue.empty()) {
    inputqueue.pop();
}

  // initialize state
  
 // define next event
  eventid = Event::send_output;
  
  // define time of next event
  time_of_next_event = 0.1;

  // schedule event in heap
  p -> eheap.push({time_of_next_event, pid});

 
}  // init()




void BaseProcess::next(double present_time)
{

  switch (eventid) {

  case Event::send_output :
      send(present_time);
      break;

  case Event::receive_input :
      receive(present_time);
      break;

  default :
    fprintf(stderr, "BaseProcess::next(): error; Event ID %d does not exist\n",
	    static_cast<int>(eventid));
    exit(1);
    break;
  }
      
} // next()
