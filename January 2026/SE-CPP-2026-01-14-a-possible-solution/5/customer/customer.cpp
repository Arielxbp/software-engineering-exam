#include "customer.h"
#include "system.h"


Customer::Customer(Global *myp, const char *mysysname)
{
#if (DEBUG > 1000)
  fprintf(stderr, "Customer::Customer(0x%lx, %s): begin\n",
	  (long unsigned int) p, mysysname);
#endif
  int i;
  
  p = myp;
  strcpy(sysname, mysysname);
  pid = p -> first_available_pid;

#if (DEBUG > 1000)
  fprintf(stderr, "Customer::Customer(%s, %d)\n", mysysname, pid);
#endif
  
  p -> dns.insert({sysname, pid});
  // update free pids
  p -> first_available_pid = p -> first_available_pid + 1;
    
  
#if (DEBUG > 1000)
  fprintf(stderr, "Customer::Customer(p, %s): end\n", mysysname);
#endif  
} // Customer()



/* initialize structures and state */
void Customer::init() {
  
  // next phase
  ph = Phase::send_output;
  // time of next event
  time = get_random_time();

  // type of event
  p -> event = 0;
  
}  // init()



void Customer::next_state(double present_time)
{
  if (present_time < time) return;
    
  assert(present_time >= time);
  

#if (DEBUG > 1000)
  fprintf(stderr, "Customer::next_state(): begin, time  = %lf\n", present_time);
#endif

  // define event type
  p -> event = 0;


  switch (ph) {

  case Phase::send_output :
      send(present_time);
      // next phase
      ph = Phase::receive_input;
      // time of next event
      time = time + time_input;
      break;

  case Phase::receive_input :
      receive(present_time);
      // next phase
      ph = Phase::send_output;
      // time of next event
      time = time + get_random_time();
      break;

  default :
    fprintf(stderr, "Customer::next_state(): error; phase %d does not exist\n",
	    static_cast<int>(ph));
    exit(1);
    break;
  }
      
#if (DEBUG > 1000)
  fprintf(stderr, "Customer::next_state(): end, time  = %lf\n", present_time);
#endif
  
} // next()



void Customer::send(double present_time)
{
  Msg m;
  int server, item, amount;
  char buf[100];
  Order ordbuf;
  
  // pick server at random in 0...S-1

  server = (*(p -> ptr_UnifIntDist))(p -> RandomEngine)%(p -> prm.S);

  // pick product at random in 0...P-1

  item = (*(p -> ptr_UnifIntDist))(p -> RandomEngine)%(p -> prm.P);

    // pick product amount at random in 1...Q

  amount = 1 + (*(p -> ptr_UnifIntDist))(p -> RandomEngine)%(p -> prm.Q);

  // new transaction
  p -> transaction = (p -> transaction) + 1;
  
  // send order to server

  m.timestamp = present_time;

  sprintf(m.sender, "%s", sysname);
  sprintf(m.receiver, "server%d", server + 1);
   
  // prepare message with format: transaction item amount
  sprintf(m.msg, "%d %d %d", p -> transaction, item, amount);

  // update orders
  ordbuf.server = server;
  ordbuf.item = item;
  ordbuf.asked = amount;
  
  pending_orders.insert({p -> transaction, ordbuf});
  
  // pending_orders[item] = pending_orders[item] + amount;

  // send message
  p -> Chp2n[pid] -> push(m);

#if (DEBUG > 1000)
  fprintf(stderr,
	  "Customer::send(%s): server %s, tr %d, item %d, amount %d, time  = %lf\n",
	  sysname, m.receiver, p -> transaction, item, amount,
	  present_time);
#endif

  return;
  
} // send()


void Customer::receive(double present_time)
{
  Msg m;
  int item;
  int amount;
  int sender;
  int transaction;
  Order ordbuf;
  
   if (p -> Chn2p[pid] -> empty()) return;

  // input queue nonempty

   m = p -> Chn2p[pid] -> front();

#if (DEBUG > 1000)
      fprintf(stderr,
"Customer::receive(): present_time = %lf, m.receiver == %s, m.sender == %s\n",
	      present_time, m.receiver, m.sender);
#endif
      

    assert(strcmp(m.receiver, sysname) == 0);  // msg is for me

    // only servers send to me
    assert(sscanf(m.sender, "server%d", &sender) >= 1);
    assert((sender >= 1) && (sender <= p -> prm.S));
      
  
   // read message with format: transaction item amount
    sscanf(m.msg, "%d %d %d", &transaction, &item, &amount);
	   
   // update pending orders
    ordbuf = pending_orders[transaction];
    p -> sys -> monitor1[0] -> update(ordbuf.asked, amount, present_time);

#if (DEBUG > 5)
    // for debug
    if (ordbuf.asked > amount)
      {
	mis++;
	mis_rate = ((double) mis)/present_time;
	fprintf(stderr, "Customer::receive(%lf): %s, %s, asked %d, received %d, mis %d, rate %lf\n",
		present_time, m.receiver, m.sender, ordbuf.asked, amount, mis, mis_rate);
      }
#endif
    
#if (DEBUG > 5)
  fprintf(stderr,
	  "Customer::receive(%s): server %s, tr %d, item %d, amount %d, asked %d, time  = %lf\n",
	  sysname, m.sender, transaction, item, amount, ordbuf.asked,
	  present_time);
#endif

  // delete transaction
    pending_orders.erase(transaction);
    
    p -> Chn2p[pid] -> pop();
 
} // receive()




// nothing ot be done for update strate
void Customer::update_state(double present_time)
{
}  // update_state()



double Customer::get_random_time()
{

  // random value in [A, B]
  return ((p -> prm.A)  + ((p -> prm.B) - (p -> prm.A))*(*(p -> ptr_UnifRealDist))(p -> RandomEngine));

	  
}


