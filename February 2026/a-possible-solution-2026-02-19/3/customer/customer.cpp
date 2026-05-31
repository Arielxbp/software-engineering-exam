#include "customer.h"
#include "system.h"



/* initialize structures and state */
void Customer::init() {

#if (DEBUG > 1000)
      fprintf(stderr, "Customer::init(): %s\n", sysname);
#endif


      // initialize state
  
 // define next event
  eventid = Event::send_output;
  
  // define time of next event
  time_of_next_event = (p -> prm.w) + get_random_time();

  // schedule event in heap
  p -> eheap.push({time_of_next_event, pid});

 
}  // init()



void Customer::send(double present_time)
{
#if (DEBUG > 10000)
   fprintf(stderr, "Customer::send(%lf): begin\n", present_time);
#endif


   Msg m;
  int server, item, amount;
  char buf[100];
  Order ordbuf;
  
  // pick server at random in 1...S

  server = 1 + (*(p -> ptr_UnifIntDist))(p -> RandomEngine)%(p -> prm.S);

  // pick product at random in 1...P

  item = 1 + (*(p -> ptr_UnifIntDist))(p -> RandomEngine)%(p -> prm.P);

    // pick product amount at random in 1...Q

  amount = 1 + (*(p -> ptr_UnifIntDist))(p -> RandomEngine)%(p -> prm.Q);

  // new transaction
  p -> transaction = (p -> transaction) + 1;
  
  // send order to server

  m.timestamp = present_time;
  sprintf(m.sender, "%s", sysname);
  sprintf(m.receiver, "server%d", server);
   
  // prepare message with format: transaction item amount
  sprintf(m.msg, "%d %d %d", p -> transaction, item, amount);

  // update orders
  ordbuf.process = server;
  ordbuf.item = item;
  ordbuf.asked = amount;
  
  pending_orders.insert({p -> transaction, ordbuf});
  
  // pending_orders[item] = pending_orders[item] + amount;

#if (DEBUG > 1000)
   fprintf(stderr, "Customer::send(%lf): %lf, %s -> %s, msg: %s\n",
	   present_time, m.timestamp, m.sender, m.receiver, m.msg);
#endif

   // send message
  //  p -> Chp2n[pid] -> push(m);
  p -> sys -> sysproc[p -> dns[m.receiver]] -> inputqueue.push(m);

     // next event
    eventid = Event::receive_input;
    
    // time of next event
    time_of_next_event = time_of_next_event + (p -> prm.r);

    // schedule next event in heap
    p -> eheap.push({time_of_next_event, pid});

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
  
  if (inputqueue.empty())
    // go to send
    {
       eventid = Event::receive_input;
       
       // time of next event
       time_of_next_event = time_of_next_event + (p -> prm.w);

       // schedule next event in heap
       p -> eheap.push({time_of_next_event, pid});
       
       return;
    }

 // input queue nonempty

   m = inputqueue.front();

#if (DEBUG > 1000)
   fprintf(stderr, "%lf: %lf, %s -> %s, msg: %s [Customer::receive()]\n",
	   present_time, m.timestamp, m.sender, m.receiver, m.msg);
#endif
     
#if (DEBUG > 10000)
      fprintf(stderr,
"Customer::receive(): present_time = %lf, m.timestamp: %lf, m.receiver: %s, m.sender: %s, m.msg: %s\n",
	      present_time, m.timestamp, m.receiver, m.sender, m.msg);
#endif
      

    assert(strcmp(m.receiver, sysname) == 0);  // msg is for me

    // only servers send to me
    assert(sscanf(m.sender, "server%d", &sender) >= 1);
    assert((sender >= 1) && (sender <= p -> prm.S));
      
  
   // read message with format: transaction item amount
    sscanf(m.msg, "%d %d %d", &transaction, &item, &amount);
	   
   // update pending orders
    ordbuf = pending_orders[transaction];
    //p -> sys -> monitor1[0] -> update(ordbuf.asked, amount, present_time);
    // The following does not work since update is not in baseprocess.
    // p -> sys -> sysproc[p -> dns["monitor1"]] -> update(ordbuf.asked, amount, present_time);

    // 1. Recupera il unique_ptr dal vettore 
    auto& basePtr = p->sys->sysproc[p->dns["monitor1"]];

    // 2. Esegui il downcast a Monitor* 
    Monitor* monitorPtr = static_cast<Monitor*>(basePtr.get());

   // 3. Chiama il metodo update del monitor
   monitorPtr->update(ordbuf.asked, amount, present_time);

   
  // delete transaction
    pending_orders.erase(transaction);
    
    inputqueue.pop();

    // next event
    eventid = Event::send_output;
    
    // time of next event
    time_of_next_event = time_of_next_event + get_random_time() + (p -> prm.w);

    // schedule next event in heap
    p -> eheap.push({time_of_next_event, pid});
    
} // receive()



double Customer::get_random_time()
{

  // random value in [A, B]
  return ((p -> prm.A)  + ((p -> prm.B) - (p -> prm.A))*(*(p -> ptr_UnifRealDist))(p -> RandomEngine));

	  
}


