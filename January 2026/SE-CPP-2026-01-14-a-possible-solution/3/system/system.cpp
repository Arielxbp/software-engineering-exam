#include "system.h"
#include "network.h"
#include "customer.h"
#include "server.h"
#include "supplier.h"
#include "mon1.h"

System::System(Global *myp, const char *mysysname)
{

  p = myp;
  strcpy(sysname, mysysname);

  // system does not participate in the pid values
  pid = -1;
  
#if 0
  pid = p -> first_available_pid;
  p -> dns.insert({sysname, pid});
  // update free pids
  p -> first_available_pid = p -> first_available_pid + 1;
#endif
  
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
  
#if (DEBUG > 100)
  fprintf(stderr, "System::create(): begin\n");
#endif

  create_customer();
  p -> num_processes = p -> num_processes + p -> prm.C;

  create_server();
  p -> num_processes = p -> num_processes + p -> prm.S;

  create_supplier();
  p -> num_processes = p -> num_processes + p -> prm.F;


  monitor1.resize(1);
  monitor1[0] = make_unique<Mon1>(p, "monitor1");

#if (DEBUG > 100)
  fprintf(stderr, "System::create(): monitor created\n");
#endif

  // monitor is not a process  
#if 0  
  p -> num_processes = p -> num_processes + 1;
#endif
  

  // Create Channels
  
  p -> Chp2n = new ChMsg[p -> num_processes];
  p -> Chn2p = new ChMsg[p -> num_processes];

  
#if (DEBUG > 1000)
  fprintf(stderr, "System::create(): channels created\n");
#endif
  

  // network must be created at the end, so that num_processes is well defined
  net.resize(1);
  net[0] = make_unique<Network>(p, "network");

  
#if (DEBUG > 100)
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


  // customers
 for (i = 0; i < p -> prm.C; i++)
    {
      localtime = MIN(minlocaltime, customer1[i] -> time);
      if (localtime < minlocaltime)
      {
	 sprintf(buf, "customer%d", i+1);
         p -> pidmoves = p -> dns[buf];
         minlocaltime = localtime;    
      }
    }

   // servers
 for (i = 0; i < p -> prm.S; i++)
    {
      localtime = MIN(minlocaltime, server1[i] -> time);
      if (localtime < minlocaltime)
      {
	 sprintf(buf, "server%d", i+1);
         p -> pidmoves = p -> dns[buf];
         minlocaltime = localtime;    
      }
    }

    // suppliers
 for (i = 0; i < p -> prm.F; i++)
    {
      localtime = MIN(minlocaltime, supplier1[i] -> time);
      if (localtime < minlocaltime)
      {
	 sprintf(buf, "supplier%d", i+1);
         p -> pidmoves = p -> dns[buf];
         minlocaltime = localtime;    
      }
    }

#if 0
 // monitor
 
 localtime = MIN(minlocaltime, monitor1[0] -> time);

  if (localtime < minlocaltime)
    {
      p -> pidmoves = p -> dns["monitor1"];
      minlocaltime = localtime;
    }
#endif
  

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

  for (i = 0; i < p -> prm.C; i++)
    { customer1[i] -> init();}

#if (DEBUG > 1000)
  fprintf(stderr, "System::init(): customer initialized\n");
#endif

  for (i = 0; i < p -> prm.S; i++)
    { server1[i] -> init();}

#if (DEBUG > 1000)
  fprintf(stderr, "System::init(): server initialized\n");
#endif

  for (i = 0; i < p -> prm.F; i++)
    { supplier1[i] -> init();}

#if (DEBUG > 1000)
  fprintf(stderr, "System::init(): supplier initialized\n");
#endif

  monitor1[0] -> init();
		     
#if (DEBUG > 1000)
  fprintf(stderr, "System::init(): monitor initialized\n");
#endif
  
  time = time_of_next_event();

  // time next event
#if (DEBUG > 1000)
  fprintf(stderr, "System::init(): time = %lf, end\n", time);
#endif   		  
}  // init()



void System::next_state() {
  int i;
  
  // compute time
  
#if (DEBUG > 1000)
  fprintf(stderr, "System::next_state(%lf): begin\n ", time);
#endif
 
   // time of next event defined
 
   // update state 

  net[0] -> next_state(time);
     
#if (DEBUG > 1000)
  fprintf(stderr, "System::next_state(%lf): network done \n", time);
#endif

  // customer
  for (i = 0; i < p -> prm.C; i++)
   {customer1[i] -> next_state(time);}

#if (DEBUG > 1000)
  fprintf(stderr, "System::next_state(%lf): customer done \n", time);
#endif

  // server
  for (i = 0; i < p -> prm.S; i++)
   {server1[i] -> next_state(time);}

#if (DEBUG > 1000)
  fprintf(stderr, "System::next_state(%lf): servers done \n", time);
#endif

  // supplier
  for (i = 0; i < p -> prm.F; i++)
   {supplier1[i] -> next_state(time);}

#if (DEBUG > 1000)
  fprintf(stderr, "System::next_state(%lf): suppliers done \n", time);
#endif
  
   time = time_of_next_event();
  // time of next event defined
 
#if (DEBUG > 1000)
  fprintf(stderr, "System::next_state(%lf): end\n", time);
#endif
  	
} // next()




void System::create_customer()
{
#if (DEBUG > 100)
  fprintf(stderr, "System::create_customer(): begin\n");
#endif

  char buf[100];
  

  customer1.resize(p -> prm.C);

#if (DEBUG > 100)
  fprintf(stderr, "System::create_customer(): vector customer created\n");
#endif
   
  
  for (int i = 0; i < p -> prm.C; ++i)
    {
         sprintf(buf, "customer%d", i+1);
#if (DEBUG > 100)
	 fprintf(stderr,
		"System::create_customer(): buf = %s, customer1 size = %ld, i = %d, p = 0x%lx\n",
		 buf, customer1.size(), i, (long unsigned int) p);
#endif
             customer1[i] = make_unique<Customer>(p, buf);

#if (DEBUG > 1000)
	     fprintf(stderr, "System::create_customer(): customer1 %s created\n", buf);
#endif  
   }


#if (DEBUG > 100)
  fprintf(stderr, "System::create_customer(): %d customer created\n", p -> prm.C);
#endif
} // create_customer()



void System::create_server()
{
#if (DEBUG > 100)
  fprintf(stderr, "System::create_server(): begin\n");
#endif
  
  char buf[100];

  server1.resize(p -> prm.S);

#if (DEBUG > 100)
  fprintf(stderr, "System::create_server(): vector server1 created\n");
#endif
   
  
  for (int i = 0; i < p -> prm.S; ++i)
    {
         sprintf(buf, "server%d", i+1);
#if (DEBUG > 100)
	 fprintf(stderr,
		"System::create_server(): buf = %s, server1 size = %ld, i = %d, p = 0x%lx\n",
		 buf, server1.size(), i, (long unsigned int) p);
#endif
             server1[i] = make_unique<Server>(p, buf);

#if (DEBUG > 1000)
	     fprintf(stderr, "System::create_server(): server1 %s created\n", buf);
#endif  
   }

#if (DEBUG > 100)
  fprintf(stderr, "System::create_server(): %d server created\n", p -> prm.S);
#endif

} // create_server()


void System::create_supplier()
{
#if (DEBUG > 100)
  fprintf(stderr, "System::create_supplier(): begin\n");
#endif
  
  char buf[100];

  supplier1.resize(p -> prm.F);

#if (DEBUG > 100)
  fprintf(stderr, "System::create_supplier(): vector suppler1 created\n");
#endif
   
  
  for (int i = 0; i < p -> prm.F; ++i)
    {
         sprintf(buf, "supplier%d", i+1);
#if (DEBUG > 100)
	 fprintf(stderr,
		"System::create_supplier(): buf = %s, supplier1 size = %ld, i = %d, p = 0x%lx\n",
		 buf, supplier1.size(), i, (long unsigned int) p);
#endif
             supplier1[i] = make_unique<Supplier>(p, buf);

#if (DEBUG > 1000)
	     fprintf(stderr, "System::create_supplier(): supplier1 %s created\n", buf);
#endif  
   }

#if (DEBUG > 100)
  fprintf(stderr, "System::create_supplier(): %d supplier created\n", p -> prm.F);
#endif

} // create_server()
