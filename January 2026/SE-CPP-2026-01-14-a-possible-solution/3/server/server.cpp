#include "server.h"


Server::Server(Global *myp, const char *mysysname)
{
#if (DEBUG > 1000)
  fprintf(stderr, "Server::Server(0x%lx, %s): begin\n",
	  (long unsigned int) p, mysysname);
#endif
  int i;
  
  p = myp;
  strcpy(sysname, mysysname);
  pid = p -> first_available_pid;

#if (DEBUG > 1000)
  fprintf(stderr, "Server::Server(%s, %d)\n", mysysname, pid);
#endif
  
  p -> dns.insert({sysname, pid});
  // update free pids
  p -> first_available_pid = p -> first_available_pid + 1;
    
  // create structures
  available_items.resize(p -> prm.P);
  
  
#if (DEBUG > 1000)
  fprintf(stderr, "Server::Server(p, %s): end\n", mysysname);
#endif  
} // Server()



/* initialize structures and state */
void Server::init() {
  int i;
  
  time_input = 0.01;
  time_output = 0.01;
  
 for (i=0; i < p -> prm.P; i++)
    {
      available_items[i] = (*(p -> ptr_UnifIntDist))(p -> RandomEngine)%((p -> prm.Q) + 1);

#if (DEBUG > 1000)
      fprintf(stderr, "Server::Server(p, %s): available_items[%d] = %d\n", sysname, i, available_items[i]);
#endif

    }
  

  
 // next phase
  ph = Phase::receive_input;
  // time of next event
  time = time_input;

  // type of event
  p -> event = 0;
  
}  // init()



void Server::next_state(double present_time)
{
  if (present_time < time) return;
    
  assert(present_time >= time);
  

#if (DEBUG > 1000)
  fprintf(stderr, "Server::next_state(): begin, time  = %lf\n", present_time);
#endif

  // define event type
  p -> event = 0;


  switch (ph) {

  case Phase::send_output :
      send(present_time);
      break;

  case Phase::receive_input :
      receive(present_time);
      break;


  default :
    fprintf(stderr, "Server::next_state(): error; phase %d does not exist\n",
	    static_cast<int>(ph));
    exit(1);
    break;
  }
      
#if (DEBUG > 1000)
  fprintf(stderr, "Server::next_state(): end, time  = %lf\n", present_time);
#endif

} // next()


// nothing to be done for send
void Server::send(double present_time)
{
    p -> Chp2n[pid].push(msg2send);
    ph = Phase::receive_input;
    // time of next event
    time = time + time_input;
	  
} // send()


void Server::receive(double present_time)
{
  Msg m;
  int item;
  int amount;
  int supplier;
  int customer;
  int shippable;
  int transaction;
  
  if (p -> Chn2p[pid].empty())
    // busy waiting
    {
       ph = Phase::receive_input;
       time = time + time_input;     
       return;
    }

  // input queue nonempty

   m = p -> Chn2p[pid].front();

#if (DEBUG > 1000)
      fprintf(stderr,
	      "Server::receive(): present_time = %lf, m.receiver == %s, m.sender == %s\n",
	      present_time, m.receiver, m.sender);
#endif
      

    assert(strcmp(m.receiver, sysname) == 0);  // msg is for me

   // read message with format: transaction item amount
    sscanf(m.msg, "%d %d %d", &transaction, &item, &amount);

#if (DEBUG > 1000)
      fprintf(stderr,
	      "Server::receive(%s): customer %s, tr %d, item %d, amount %d, present_time = %lf,\n",
	      m.receiver, m.sender, transaction, item, amount, present_time);
#endif

      
   // msg may be from customer or from supplier
    // find out

    if (sscanf(m.sender, "customer%d", &customer) >= 1)
      // msg is from customer
      {
	shippable = MIN(available_items[item], amount);

	// update available items
	available_items[item] = available_items[item] - shippable;

	// define msg to send
	sprintf(msg2send.sender,"%s", sysname);
	sprintf(msg2send.receiver,"%s", m.sender);
	sprintf(msg2send.msg,"%d %d %d", transaction, item, shippable);

	ph = Phase::send_output;
        time = time + time_output;     
        p -> Chn2p[pid].pop();
       return;
      }


     if (sscanf(m.sender, "supplier%d", &supplier) >= 1)
      // msg is from supplier
      {
	// update available items
	available_items[item] = available_items[item] + amount;
	// nothign to send
	ph = Phase::receive_input;
        time = time + time_input;     
	p -> Chn2p[pid].pop();
       return;
      }
     else  // error
       {
          fprintf(stderr, "Server::receive(): error; msg from %s\n", m.sender);
          exit(1);
       }


       return;

} // receive()




double Server::get_random_time()
{

  // random value in [A, B]
  return ((p -> prm.A)  + ((p -> prm.B) - (p -> prm.A))*(*(p -> ptr_UnifRealDist))(p -> RandomEngine));

	  
}


