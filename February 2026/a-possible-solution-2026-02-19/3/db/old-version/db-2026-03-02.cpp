#include "db.h"



/* initialize structures and state */
void DB::init() {
  int i;

#if (DEBUG > 1000)
      fprintf(stderr, "DB::init(): %s\n", sysname);
#endif
  

// initialize state
  
 available_items.resize(p -> prm.P);
  
  
 for (i=0; i < p -> prm.P; i++)
    {
      available_items[i] = (*(p -> ptr_UnifIntDist))(p -> RandomEngine)%((p -> prm.Q) + 1);
    }
  
  
   // define next event
  eventid = Event::receive_input;
  
  // define time of next event
  time_of_next_event = p -> prm.r + p -> prm.l;

  // schedule event in heap
  p -> eheap.push({time_of_next_event, pid});

  
}  // init()



void DB::send(double present_time)
{
  Msg m;
  
  msg2send.timestamp = present_time;
  
#if (DEBUG > 10000)
      fprintf(stderr,
	      "DB::send(): timestamp %lf, sender %s, receiver %s, msg: %s, present time %lf\n",
	      msg2send.timestamp,
	      msg2send.sender,
	      msg2send.receiver,
	      msg2send.msg, present_time);
#endif

      
   // p -> Chp2n[pid] -> push(msg2send);
    p -> sys -> sysproc[p -> dns[msg2send.receiver]] -> inputqueue.push(msg2send);

#if (DEBUG > 10000)
      fprintf(stderr,
	      "DB::send(%lf): back of %s input queue is: tmstp %lf, sender %s, receiver %s, msg %s\n",
	      present_time, msg2send.receiver,
	      p -> sys -> sysproc[p -> dns[msg2send.receiver]] -> inputqueue.back().timestamp,
	      p -> sys -> sysproc[p -> dns[msg2send.receiver]] -> inputqueue.back().sender,
	      p -> sys -> sysproc[p -> dns[msg2send.receiver]] -> inputqueue.back().receiver,
	      p -> sys -> sysproc[p -> dns[msg2send.receiver]] -> inputqueue.back().msg
	      );
#endif

      // printing, just fro debugging
#if (DEBUG > 10000)
ChMsg dbgqueue;
 int i = 0;
 
 dbgqueue = p -> sys -> sysproc[p -> dns[msg2send.receiver]] -> inputqueue;
 
 while (!(dbgqueue.empty()))
	{
       fprintf(stderr,
	      "DB::send(%lf): element %d of %s input queue is: tmstp %lf, sender %s, receiver %s, msg %s\n",
	      present_time,
	       i,
	      dbgqueue.front().receiver,
	      dbgqueue.front().timestamp,
	      dbgqueue.front().sender,
	      dbgqueue.front().receiver,
	      dbgqueue.front().msg
	      );

       i++;
       dbgqueue.pop();
    }
#endif

 
#if 0
 // next event
    eventid = Event::receive_input;
    
    // time of next event
    time_of_next_event = time_of_next_event + (p -> prm.r) + (p -> prm.l);

    // schedule next event in heap
    p -> eheap.push({time_of_next_event, pid});
#endif
    
#if (DEBUG > 1000)
      fprintf(stderr,
	      "DB::send(%lf): timestamp %lf, sender %s, receiver %s, msg: %s, time_of_next_event %lf\n",
	      present_time,
	      msg2send.timestamp,
	      msg2send.sender,
	      msg2send.receiver,
	      msg2send.msg, time_of_next_event);
#endif
      
} // send()


void DB::receive(double present_time)
{
  Msg m;
  int item;
  int amount;
  int supplier;
  int server;
  int customer;
  int shippable;
  int transaction;
  int reqid; // request id: 0 server req info product i,
  
  if (inputqueue.empty())
    // busy waiting
    {
       eventid = Event::receive_input;
       // time of next event
       time_of_next_event = time_of_next_event + (p -> prm.r) + (p -> prm.l);

       // schedule next event in heap
       p -> eheap.push({time_of_next_event, pid});
       
       return;
    }

  // input queue nonempty

  // read all inputs 
       
  while (!inputqueue.empty())
    {
  // m = p -> Chn2p[pid] -> front();
      
   m = inputqueue.front();

   
#if (DEBUG > 10000)
   fprintf(stderr, "DB::receive(%lf): %lf, %s -> %s, msg: %s\n",
	   present_time, m.timestamp, m.sender, m.receiver, m.msg);
#endif
   
#if (DEBUG > 1000)
      fprintf(stderr,
	      "DB::receive(): present_time = %lf, m.timestamp %lf, m.receiver: %s, m.sender: %s, m.msg: %s\n",
	      present_time, m.timestamp, m.receiver, m.sender, m.msg);
#endif
      

    assert(strcmp(m.receiver, sysname) == 0);  // msg is for me


   // msg may be from server or from supplier
    // find out

    if (sscanf(m.sender, "server%d", &server) >= 1)
      // msg is from server
      {
  // read message with format: reqid customerid item amount
	sscanf(m.msg, "%d %d %d %d", &reqid, &transaction, &item, &amount);

#if (DEBUG > 1000)
      fprintf(stderr,
	      "DB::receive(%s): server %s, reqid %d, tr %d, item %d, amount %d, present_time = %lf,\n",
	      m.receiver, m.sender, reqid, transaction, item, amount, present_time);
#endif

    // check reqid
	switch (reqid)
	  {
	     case 0: // server is asking DB available amount of product item
	           shippable = MIN(available_items[item - 1], amount);
	           // update available items
	            available_items[item - 1] = available_items[item - 1] - shippable;

		    assert((item >= 1) && (item <= p -> prm.P));
		    
	            msg2send.timestamp = present_time;
	            sprintf(msg2send.sender,"%s", sysname);
	            sprintf(msg2send.receiver,"%s", m.sender);
		    sprintf(msg2send.msg,"%d %d %d",
		    transaction, item, shippable);
		    
#if (DEBUG > 1000)
      fprintf(stderr,
	      "DB::receive(%lf): msg2send: timestamp %lf, sender %s, receiver %s, msg: %s\n",
	      present_time, msg2send.timestamp, msg2send.sender, msg2send.receiver,
	      msg2send.msg);
#endif

                    inputqueue.pop();
		     // next event
                    time_of_next_event = time_of_next_event + (p -> prm.l) + (p -> prm.s);
		    send(time_of_next_event);
		    
#if 0
		     eventid = Event::send_output;
                     // time of next event
		     //assert(p -> prm.w <= 1);
		     //assert(p -> prm.s <= 1);
                     time_of_next_event = time_of_next_event + (p -> prm.w) + (p -> prm.s);
		     time_of_next_event = time_of_next_event 
                     // schedule next event in heap
                     p -> eheap.push({time_of_next_event, pid});
#endif
		     
		     
#if (DEBUG > 1000)
      fprintf(stderr,
	      "DB::receive(%lf): msg2send: send pid %d planned at time %lf\n",
	      present_time, pid, time_of_next_event);
#endif

             break;
		     
	  case 1:  // server notifies db amount that will be shipped
	    // shippable = MIN(available_items[item], amount);
	          // update available items
	          // available_items[item] = available_items[item] - shippable;
		  // there is no msg from DB to server
	    // nothing to do since items already locked

	            inputqueue.pop();
		    time_of_next_event = time_of_next_event + (p -> prm.l) + (p -> prm.s);
		    
#if 0
		    // next event
                     eventid = Event::receive_input;
                     // time of next event
	             time_of_next_event = time_of_next_event + (p -> prm.l) + (p -> prm.s);
                     // schedule next event in heap
                     p -> eheap.push({time_of_next_event, pid});
#endif
		     
	    break;

	  default:
	    fprintf(stderr, "DB::receive(): error: reqid %d does not exist\n", reqid);
            exit(1);
	    

	  } // switch()


      } // if then

	

   else  if (sscanf(m.sender, "supplier%d", &supplier) >= 1)
      // msg is from supplier
      {
 // read message with format: item amount
    sscanf(m.msg, "%d %d", &item, &amount);

#if (DEBUG > 10000)
      fprintf(stderr,
	      "DB::receive(%s): supplier %s, item %d, amount %d, present_time = %lf,\n",
	      m.receiver, m.sender, item, amount, present_time);
#endif


      // update available items
	available_items[item-1] = available_items[item-1] + amount;

	// nothign to send
		inputqueue.pop();
		time_of_next_event = time_of_next_event + (p -> prm.l) + (p -> prm.s);
		
#if 0
		// next event
       eventid = Event::receive_input;
       // time of next event
       time_of_next_event = time_of_next_event + (p -> prm.r) + (p -> prm.l);
      // schedule next event in heap
       p -> eheap.push({time_of_next_event, pid});
#endif
       
      }  // then     
   else  // error
       {
    fprintf(stderr, "DB::receive(): error; unexpected msg from %s\n", m.sender);
    exit(1);
       }

    }  // while (!inputqueue.empty())

      eventid = Event::receive_input;
                     // time of next event
		     //assert(p -> prm.w <= 1);
		     //assert(p -> prm.s <= 1);
                     // schedule next event in heap
      p -> eheap.push({time_of_next_event, pid});


		     
   return;

} // receive()




