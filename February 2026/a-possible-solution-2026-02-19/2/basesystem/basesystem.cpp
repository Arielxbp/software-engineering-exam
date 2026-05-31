
#include "basesystem.h"


BaseSystem::BaseSystem(Global *myp, const char *mysysname)
{

  p = myp;
  strcpy(sysname, mysysname);

#if (DEBUG > 1000)
  fprintf(stderr, "BaseSystem::BaseSystem(): done\n");
#endif

} // BaseSystem()

void BaseSystem::pidinit()
{
  p -> first_available_pid = 0;
  p -> num_processes = 0; 
} // pidupd()


void BaseSystem::pidupd()
{
p -> first_available_pid = p -> first_available_pid + 1;
p -> num_processes = p -> num_processes + 1;
} // pidupd()



// initialize system state
void BaseSystem::init() { 

#if (DEBUG > 1000)
  fprintf(stderr, "BaseSystem::init(): begin\n");
#endif

  int i;

  time = 0;
  
  create();

#if (DEBUG > 1000)
  fprintf(stderr, "BaseSystem::init(): init processes\n");
#endif

  for (i = 0; i < p -> num_processes; i++)
    {
#if (DEBUG > 1000)
      fprintf(stderr, "BaseSystem::init(): creating process with pid %d\n", i);
#endif   
      sysproc[i] -> init();

#if (DEBUG > 1000)
      fprintf(stderr, "BaseSystem::init(): done with pid %d\n", i);
#endif

    }


#if (DEBUG > 1000)
  fprintf(stderr, "BaseSystem::init(): end\n");
#endif   		  
}  // init()



void BaseSystem::next() {

  pair<double, int> top;

  // compute time
  
#if (DEBUG > 1000)
  fprintf(stderr, "BaseSystem::next_state(%lf): begin\n ", time);
#endif

  // Call all events occurring at the same min time
  
  if (!(p -> eheap.empty()))
    // define time
    {
      top = p -> eheap.top();
      time = top.first;
    }
  
  while (!(p -> eheap.empty())) {
        top = p -> eheap.top();
	
#if (DEBUG > 1000)
	fprintf(stderr, "Retrivied from Heap: (%lf, %d)\n", top.first, top.second);
#endif	
	if (top.first > time) break;  // too far in the future
        // top.first == time
        sysproc[top.second] -> next(top.first);
        p -> eheap.pop();
    }
    
} // next()



