#include "supplier.h"


Supplier::Supplier(Global *myp, const char *mysysname)
{
#if (DEBUG > 1000)
  fprintf(stderr, "Supplier::Supplier(0x%lx, %s): begin\n",
	  (long unsigned int) p, mysysname);
#endif
  int i;
  
  p = myp;
  strcpy(sysname, mysysname);
  pid = p -> first_available_pid;

#if (DEBUG > 1000)
  fprintf(stderr, "Supplier::Supplier(%s, %d)\n", mysysname, pid);
#endif
  
  p -> dns.insert({sysname, pid});
  // update free pids
  p -> first_available_pid = p -> first_available_pid + 1;
    
  // create structures
  
  
#if (DEBUG > 1000)
  fprintf(stderr, "Supplier::Supplier(p, %s): end\n", mysysname);
#endif  
} // Supplier()



/* initialize structures and state */
void Supplier::init() {
    
  time = 0;
  
  // next phase
  ph = Phase::send_output;
  // time of next event
  time = get_random_time();

  // type of event
  p -> event = 0;
  
}  // init()



void Supplier::next_state(double present_time)
{
  if (present_time < time) return;
    
  assert(present_time >= time);
  

#if (DEBUG > 1000)
  fprintf(stderr, "Supplier::next_state(): begin, time  = %lf\n", present_time);
#endif

  // define event type
  p -> event = 0;


  switch (ph) {

  case Phase::send_output :
      send(present_time);
      // next phase
      ph = Phase::send_output;
      // next time
      time = time + get_random_time();
      break;

  case Phase::receive_input :
      receive(present_time);
      // next phase
      ph = Phase::send_output;
      // next time
	time = time + time_input;
      break;

  default :
    fprintf(stderr, "Supplier::next_state(): error; phase %d does not exist\n",
	    static_cast<int>(ph));
    exit(1);
    break;
  }
      
#if (DEBUG > 1000)
  fprintf(stderr, "Supplier::next_state(): end, time  = %lf\n", present_time);
#endif
  
} // next()



void Supplier::send(double present_time)
{
  Msg m;
  int server, item, amount;
  char buf[100];
  
#if (DEBUG > 1000)
  fprintf(stderr, "Supplier::send(): begin, time  = %lf\n", present_time);
#endif

  // pick server at random in 0...S-1

  server = (*(p -> ptr_UnifIntDist))(p -> RandomEngine)%(p -> prm.S);

  // pick product at random in 0...P-1

  item = (*(p -> ptr_UnifIntDist))(p -> RandomEngine)%(p -> prm.P);

    // pick product amount at random in 1...Q

  amount = 1 + (*(p -> ptr_UnifIntDist))(p -> RandomEngine)%(p -> prm.Q);

  // send supply to server

  m.timestamp = present_time;

  sprintf(m.sender, "%s", sysname);
  sprintf(m.receiver, "server%d", server + 1);
   
#if (DEBUG > 1000)
  fprintf(stderr, "Supplier::send(): ckp 1000, time  = %lf\n", present_time);
#endif

  // prepare message with format: transaction item amount
  sprintf(m.msg, "0 %d %d", item, amount);
  
#if (DEBUG > 1000)
  fprintf(stderr, "Supplier::send(): ckp 2000, pid =%d,  time  = %lf\n", pid, present_time);
#endif

  // send message
  p -> Chp2n[pid] -> push(m);

#if (DEBUG > 1000)
  fprintf(stderr, "Supplier::send(): end, time  = %lf\n", present_time);
#endif

  return;
  
} // send()

// nothing ot be done 
void Supplier::receive(double present_time)
{


} // receive()





double Supplier::get_random_time()
{

  // random value in [A, B]
  return ((p -> prm.V)  + ((p -> prm.W) - (p -> prm.V))*(*(p -> ptr_UnifRealDist))(p -> RandomEngine));

	  
}


