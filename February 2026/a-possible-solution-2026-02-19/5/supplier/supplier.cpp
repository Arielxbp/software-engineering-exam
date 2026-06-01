#include "supplier.h"
#include "system.h"



/* initialize structures and state */
void Supplier::init() {

#if (DEBUG > 1000)
      fprintf(stderr, "Supplier::init(): %s\n", sysname);
#endif
      



// initialize state
  
 // define next event
  eventid = Event::send_output;
  
  // define time of next event
  time_of_next_event = get_random_time() + (p -> prm.w);

  // schedule event in heap
  p -> eheap.push({time_of_next_event, pid});


}  // init()




void Supplier::send(double present_time)
{
  Msg m;
  int server, item, amount;
  char buf[100];
  
#if (DEBUG > 10000)
  fprintf(stderr, "Supplier::send(): begin, time  = %lf\n", present_time);
#endif

  // pick server at random in 1...S

  // server = 1 + (*(p -> ptr_UnifIntDist))(p -> RandomEngine)%(p -> prm.S);

  // pick product at random in 1...P

  item = 1 + (*(p -> ptr_UnifIntDist))(p -> RandomEngine)%(p -> prm.P);

    // pick product amount at random in 1...Q

  amount = 1 + (*(p -> ptr_UnifIntDist))(p -> RandomEngine)%(p -> prm.Q);

  // send supply to server

  m.timestamp = present_time;

  sprintf(m.sender, "%s", sysname);
  sprintf(m.receiver, "db%d", 1);
   
#if (DEBUG > 10000)
  fprintf(stderr, "Supplier::send(): ckp 1000, time  = %lf\n", present_time);
#endif

  // prepare message with format: transaction item amount
  sprintf(m.msg, "%d %d", item, amount);
  
#if (DEBUG > 10000)
  fprintf(stderr, "Supplier::send(): ckp 2000, pid =%d, timestamp %lf, sender %s, receiver %s, msg %s, %lf\n",
	  pid,
	  m.timestamp,
	  m.sender, m.receiver, m.msg,
	  present_time);
#endif

  // send message
  //p -> Chp2n[pid] -> push(m);
  p -> sys -> sysproc[p -> dns[m.receiver]] -> inputqueue.push(m);

      // next event
    eventid = Event::receive_input;
    
    // time of next event
    time_of_next_event = time_of_next_event + ((p -> prm.V)/20.0);

    // schedule next event in heap
    p -> eheap.push({time_of_next_event, pid});

#if (DEBUG > 1000)
  fprintf(stderr, "Supplier::send(%lf): pid =%d, timestamp %lf, sender %s, receiver %s, msg %s, time_of_next_event %lf\n",
	  present_time,
	  pid,
	  m.timestamp,
	  m.sender, m.receiver, m.msg,
	  time_of_next_event);
#endif
  


  return;
  
} // send()

void Supplier::receive(double present_time)
{
 Msg m;
 int db;
 
  if (inputqueue.empty())
    // busy waiting
    {
       eventid = Event::receive_input;
       // time of next event
       time_of_next_event = time_of_next_event + ((p -> prm.V)/20.0);

       // schedule next event in heap
       p -> eheap.push({time_of_next_event, pid});
       
       return;
    }

   // input queue nonempty

  // ack received
  // m = p -> Chn2p[pid] -> front();
   m = inputqueue.front();

#if (DEBUG > 10)
  fprintf(stderr, "Supplier::receive(%lf): timestamp %lf, sender(%d) %s, receiver(%d) %s, msg %s\n",
	  present_time,
	  m.timestamp,
	  p -> dns[m.sender],
	  m.sender,
	  p -> dns[m.receiver],
	  m.receiver,
	  m.msg
	  );
#endif

  
  assert(strcmp(m.receiver, sysname) == 0);  // msg is for me

  if (sscanf(m.sender, "db%d", &db) >= 1)
      // msg is from db, ok
      {
	inputqueue.pop();
	// next event
        eventid = Event::send_output;
        time_of_next_event = time_of_next_event + get_random_time();
        // schedule next event in heap
        p -> eheap.push({time_of_next_event, pid});
	return;
      }
  else // error
    {
     fprintf(stderr,
	      "Supplier::receive(%lf): msg %s is from %s, whereas should be from db1\n",
	     present_time, m.msg, m.sender);
     exit(1);
    }
  
}  // receive()


double Supplier::get_random_time()
{

  // random value in [A, B]
  return ((p -> prm.V)  + ((p -> prm.W) - (p -> prm.V))*(*(p -> ptr_UnifRealDist))(p -> RandomEngine));

	  
}


