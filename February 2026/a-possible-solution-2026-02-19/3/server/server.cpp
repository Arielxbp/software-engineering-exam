#include "server.h"


/* initialize structures and state */
void Server::init() {
  int i;
  
#if (DEBUG > 1000)
      fprintf(stderr, "Server::init(): %s\n", sysname);
#endif


// initialize state
  // msg2send2.timestamp = -1;
  msg2send.timestamp = -1;
 
   // define next event
  eventid = Event::receive_input;
  
  // define time of next event
  time_of_next_event = p -> prm.r + p -> prm.z;

  // schedule event in heap
  p -> eheap.push({time_of_next_event, pid});

  
}  // init()



void Server::send(double present_time)
{

   int field[4];

    msg2send.timestamp = present_time;


#if (DEBUG > 100000)
   fprintf(stderr, "Server::send(%lf): timestamp: %lf, sender() %s, receiver() %s, msg %s\n",
	   present_time,
	   msg2send.timestamp,
           msg2send.sender,
	   msg2send.receiver,
	   msg2send.msg
	   );
#endif
   
   // p -> Chp2n[pid] -> push(msg2send);
    p -> sys -> sysproc[p -> dns[msg2send.receiver]] -> inputqueue.push(msg2send);

     // next event
    eventid = Event::receive_input;
    
    // time of next event
    time_of_next_event = time_of_next_event + (p -> prm.z) + (p -> prm.w);

    // schedule next event in heap
    p -> eheap.push({time_of_next_event, pid});

#if (DEBUG > 10)
   fprintf(stderr, "Server::send(%lf): timestamp: %lf, sender(%d) %s, receiver(%d) %s, msg %s, time_of_next_event = %lf\n",
	   present_time,
	   msg2send.timestamp,
	   p -> dns[msg2send.sender],
           msg2send.sender,
	   p -> dns[msg2send.receiver],
	   msg2send.receiver,
	   msg2send.msg,
	   time_of_next_event
	   );
#endif
   
} // send()


void Server::receive(double present_time)
{
  Msg m;
  int item;
  int amount;
  int supplier;
  int customer;
  int db;
  int shippable;
  int transaction;
  int reqid;
  int available;
  Order ordbuf;

#if (DEBUG > 10000)
   fprintf(stderr, "Server::receive(%lf): Begin\n", present_time);
#endif


   if (inputqueue.empty())
    // busy waiting
    {
       eventid = Event::receive_input;
       // time of next event
       time_of_next_event = time_of_next_event + (p -> prm.z) + (p -> prm.w);

       // schedule next event in heap
       p -> eheap.push({time_of_next_event, pid});
       
       return;
    }

  // input queue nonempty

  // m = p -> Chn2p[pid] -> front();
   m = inputqueue.front();

#if (DEBUG > 1000)
   fprintf(stderr, "Server::receive(%lf): %lf, %s -> %s, msg: %s\n",
	   present_time, m.timestamp, m.sender, m.receiver, m.msg);
#endif


#if (DEBUG > 10000)
      fprintf(stderr,
	      "Server::receive(): present_time = %lf, m.timestamp %lf, m.receiver: %s, m.sender: %s, m.msg: %s\n",
	      present_time, m.timestamp, m.receiver, m.sender, m.msg);
#endif
      

    assert(strcmp(m.receiver, sysname) == 0);  // msg is for me
   
   // msg may be from customer or from supplier
    // find out

    if (sscanf(m.sender, "customer%d", &customer) >= 1)
      // msg is from customer
      {
  // read message with format:  transaction item amount
    sscanf(m.msg, "%d %d %d", &transaction, &item, &amount);

       
#if (DEBUG > 10000)
      fprintf(stderr,
	      "Server::receive(%s): sender %s, customerid %d, timestamp %lf, tr %d, item %d, amount %d, present_time = %lf,\n",
	      m.receiver, m.sender, customer, m.timestamp, transaction, item, amount, present_time);
#endif

  // update orders
  ordbuf.process = customer;
  ordbuf.item = item;
  ordbuf.asked = amount;

  assert(ordbuf.process > 0);  // ids start from 1.
  pending_orders.insert({transaction, ordbuf});  

  // modify reqs: send to customer what it is possible to send
  
      // ask db for amount available items 

	// define msg to send to db
        msg2send.timestamp = present_time;
	sprintf(msg2send.sender,"%s", sysname);
	sprintf(msg2send.receiver,"db1");
	sprintf(msg2send.msg,"%d %d %d %d", 0, transaction, item, amount);

        //p -> Chn2p[pid] -> pop();

	inputqueue.pop();
 
       // next event
       eventid = Event::send_output;
       // time of next event
       time_of_next_event = time_of_next_event + (p -> prm.z) + (p -> prm.v);
      // schedule next event in heap
       p -> eheap.push({time_of_next_event, pid});
    
       return;
      }

 if (sscanf(m.sender, "db%d", &db) >= 1)
      // msg is from db
      {
	// db has sent available items to server, 

	// read message with format: transaction item available
	sscanf(m.msg, "%d %d %d %d", &reqid, &transaction, &item, &available);

	// available is already the min between  what is in the DB and the requested amount
	
	// transaction must be known
	assert(pending_orders.count(transaction) > 0);

	
#if (DEBUG > 10000)
      fprintf(stderr,
	      "Server::receive(%s): sender %s, timestamp %lf, msg: %s, transaction %d, item %d, available %d, present_time = %lf,\n",
	      m.receiver, m.sender, m.timestamp, m.msg, transaction, item, available, present_time);
#endif

      switch (reqid)
	{

	case 3: // notification available items

               ordbuf = pending_orders[transaction];
             assert(ordbuf.process > 0);  // ids start from 1.

	
                shippable = MIN(available, ordbuf.asked);

      
      // define msg to send to customer
	msg2send.timestamp = present_time;
	sprintf(msg2send.sender,"%s", sysname);
	assert(ordbuf.process > 0);  // ids start from 1.
	sprintf(msg2send.receiver,"customer%d", ordbuf.process);
	sprintf(msg2send.msg,"%d %d %d", transaction, item, shippable);

	
        //p -> Chn2p[pid] -> pop();

	inputqueue.pop();

	// delete transaction, 
        pending_orders.erase(transaction);
    
	
       // next event
       eventid = Event::send_output;
       // time of next event
       time_of_next_event = time_of_next_event + (p -> prm.z) + (p -> prm.v);
      // schedule next event in heap
       p -> eheap.push({time_of_next_event, pid});
    
       return;

	case 2:   // ack from db

	  inputqueue.pop();

               // next event
       eventid = Event::receive_input;
       // time of next event
       time_of_next_event = time_of_next_event + (p -> prm.z) + (p -> prm.v);
      // schedule next event in heap
       p -> eheap.push({time_of_next_event, pid});

       return;


	default:
	  
	    fprintf(stderr, "Server::receive(): error: reqid %d does not exist\n", reqid);

            exit(1);
	    
	}  // switch ()
      
      } // msg is from db

 // error
 
     fprintf(stderr, "Server::receive(): error; msg from %s\n", m.sender);
     exit(1);
      


       return;

} // receive()




