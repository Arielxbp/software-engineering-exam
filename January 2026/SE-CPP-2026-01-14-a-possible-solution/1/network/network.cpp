#include "network.h"


Network::Network(Global *myp, const char *mysysname)
{

  p = myp;

  strcpy(sysname, mysysname);
  pid = p -> first_available_pid;
  p -> dns.insert({sysname, pid});
  // update free pids
  p -> first_available_pid = p -> first_available_pid + 1;

  // define event type
  p -> event = -1;  // network
    
#if (DEBUG > 1000)
  fprintf(stderr, "Network::Network(): p -> num_processes = %d\n",
	  p -> num_processes);
#endif

  // create structures, if any
  scanner.resize(p -> num_processes);
  
  
} // Network()

#if 0
/* create system */
void Network::create()
{
  scanner.resize(p -> num_processes);
}
#endif

/* initial state */
void Network::init() {

  int i;

   // set channels

#if (DEBUG > 1000)
  fprintf(stderr, "Network::init(): begin\n");
#endif


  num_channels = p -> num_processes;

  for (i=0; i < num_channels; i++)
    {
      scanner[i] = i;
    }
  
#if (DEBUG > 1000)
  fprintf(stderr, "Network::init(): scan\n");
#endif

  // scan
      ctime = 0.1;
      stime = 0.1; 
      time = ctime;  // time of next event

 // define event type
  p -> event = -1;  // network

#if (DEBUG > 1000)
  fprintf(stderr, "Network::init(): end\n");
#endif

  
}  // init()


void Network::next_state(double present_time) {

int i;

  if (present_time < time) return;

   assert(present_time >= time);
  
    // define event type
    p -> event = -1;  // network

    scan(present_time);

   // schedule next scan
   time = time + ctime + stime;
   
 
} // next()


	       
// thread 0
void Network::scan(double present_time)
{
  Msg m;
  
#if (DEBUG > 1000)
  fprintf(stderr, "Network::scan(): Network moves, time  = %lf\n", present_time);
#endif
  
   if (scanned >= num_channels)
     // init scan
     {
       scanned = 0;
       shuffle(scanner.begin(), scanner.end(), p -> RandomEngine);      
     }
   

   if (!(p -> Chp2n[scanner[scanned]].empty())) 
      // queue nonempty
   {
      m = p -> Chp2n[scanner[scanned]].front();
      p -> Chp2n[scanner[scanned]].pop();
     
      assert(m.sender == scanner[scanned]);
      
      p -> Chn2p[m.receiver].push(m);
   }

   scanned++;


    return;

  
}  // scan()




