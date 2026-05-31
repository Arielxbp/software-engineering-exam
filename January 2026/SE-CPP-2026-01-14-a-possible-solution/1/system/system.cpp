#include "system.h"


System::System(Global *myp, const char *mysysname)
{

  p = myp;
  strcpy(sysname, mysysname);
  pid = p -> first_available_pid;
  p -> dns.insert({sysname, pid});
  // update free pids
  p -> first_available_pid = p -> first_available_pid + 1;
    
#if (DEBUG > 1000)
  fprintf(stderr, "System::System(): pid = %d, done\n", pid);
#endif

  // create structures
} // System()


/* create system  */
void System::create()
{ 
  int i;
  char buf[100];
  
#if (DEBUG > 1000)
  fprintf(stderr, "System::create(): begin\n");
#endif

  uav1.resize(p -> prm.N);

#if (DEBUG > 1000)
  fprintf(stderr, "System::create(): vector uav1 created\n");
#endif
   
  
  for (int i = 0; i < p -> prm.N; ++i) {
         sprintf(buf, "uav%d", i+1);
#if (DEBUG > 1000)
	 fprintf(stderr,
		"System::create(): buf = %s, uav1 size = %ld, i = %d, p = 0x%lx\n",
		 buf, uav1.size(), i, (long unsigned int) p);
#endif
             uav1[i] = make_unique<UAV>(p, buf);

#if (DEBUG > 1000)
	     fprintf(stderr, "System::create(): uav1 %s created\n", buf);
#endif  
   }

#if (DEBUG > 1000)
  fprintf(stderr, "System::create(): uav created\n");
#endif

  p -> num_processes = p -> num_processes + p -> prm.N;
    
  monitor1.resize(1);
  monitor1[0] = make_unique<Mon1>(p, "CollisionMonitor");

#if (DEBUG > 1000)
  fprintf(stderr, "System::create(): monitor created\n");
#endif

  p -> num_processes = p -> num_processes + 1;

  p -> Chp2n = new ChMsg[p -> num_processes];
  p -> Chn2p = new ChMsg[p -> num_processes];

#if (DEBUG > 1000)
  fprintf(stderr, "System::create(): channels created\n");
#endif
  
#if 0
  net[0] -> create();

  for (i = 0; i < p -> prm.N; i++)
    { uav1[i] -> create();}

  monitor1[0] -> create();
#endif

  // network must be created at the end, so that num_processes is well defined
   net.resize(1);
  net[0] = make_unique<Network>(p, "network");
  
#if (DEBUG > 1000)
  fprintf(stderr, "System::create(): network created, p -> num_processes = %d\n",
	  p -> num_processes);
#endif
  // network does not count among num_processes  */
}





double System::time_of_next_event()
{
  int i;
  double minlocaltime, localtime;
  char buf[100];
  
  localtime = DBL_MAX;
  minlocaltime = localtime;
  localtime = MIN(minlocaltime, net[0] -> time);

  if (localtime < minlocaltime)
    {
      p -> pidmoves = p -> dns["network"];
      minlocaltime = localtime;
    }


 for (i = 0; i < p -> prm.N; i++)
    {
      localtime = MIN(minlocaltime, uav1[i] -> time);
      if (localtime < minlocaltime)
      {
	 sprintf(buf, "uav%d", i+1);
         p -> pidmoves = p -> dns[buf];
         minlocaltime = localtime;    
      }
    }


 localtime = MIN(minlocaltime, monitor1[0] -> time);

  if (localtime < minlocaltime)
    {
      p -> pidmoves = p -> dns["monitor1"];
      minlocaltime = localtime;
    }


   return(localtime);

} // time_of_next_event()



// initialize system state
void System::init() { 
  int i;

#if (DEBUG > 1000)
  fprintf(stderr, "System::init(): begin\n");
#endif


  net[0] -> init();

#if (DEBUG > 1000)
  fprintf(stderr, "System::init(): net initialized\n");
#endif

  for (i = 0; i < p -> prm.N; i++)
    { uav1[i] -> init();}

#if (DEBUG > 1000)
  fprintf(stderr, "System::init(): uav initialized\n");
#endif

  monitor1[0] -> init();

#if (DEBUG > 1000)
  fprintf(stderr, "System::init(): monitor initialized\n");
#endif

  
  time = time_of_next_event();

  // time next event
#if (DEBUG > 1000)
  fprintf(stderr, "System::init(): end\n");
#endif   		  
}  // init()


void System::next_state() {
  int i;
  
  // compute time
  
#if (DEBUG > 1000)
  fprintf(stderr, "System::next_state(%lf): ", time);
#endif
 
   // time of next event defined
 
   // update state 

  net[0] -> next_state(time);
     
  for (i = 0; i < p -> prm.N; i++)
   {uav1[i] -> next_state(time);}

  monitor1[0] -> next_state(time);

   time = time_of_next_event();
  // time of next event defined
 
#if (DEBUG > 1000)
  fprintf(stderr, "System::next_state(): time = %lf, ckp 3000\n", time);
#endif
  	
} // next()


