#include "uav.h"
#include "system.h"



/* initialize structures and state */
void UAV::init() {
  int i;
  
  // initialize state
     for (i=0; i < 3; i++)
     {
       x[i] = (-(p -> prm.L)  + 2*(p -> prm.L)*(*(p -> ptr_UnifRealDist))(p -> RandomEngine));
     }

     
 // define next event
  eventid = Event::receive_input;
  
  // define time of next event
  time_of_next_event = p -> prm.T;

  // schedule event in heap
  p -> eheap.push({time_of_next_event, pid});

 
}  // init()



void UAV::receive(double present_time)
{
  int i;

  // update state

  // pick theta, phi

   ctr();

  // define v
  
   v[0] = (p -> prm.V)*sin(theta)*cos(phi);
   v[1] = (p -> prm.V)*sin(theta)*sin(phi);
   v[2] = (p -> prm.V)*cos(theta);
   
   // update x

   for (i=0; i < 3; i++)
     {
       x[i] = x[i] + v[i]*(p -> prm.T);
     }

   // update monitor
       // 1. Recupera il unique_ptr dal vettore 
    auto& basePtr = p->sys->sysproc[p->dns["monitor1"]];

    // 2. Esegui il downcast a Monitor* 
    Monitor* monitorPtr = static_cast<Monitor*>(basePtr.get());

   // 3. Chiama il metodo update 
   monitorPtr->update(pid);  
   
     
    // next event
    eventid = Event::receive_input;
    
    // time of next event
    time_of_next_event = time_of_next_event + (p -> prm.T);

    // schedule next event in heap
    p -> eheap.push({time_of_next_event, pid});
    
} // receive()


/*
*************************************
CONTROLLER
*************************************
*/


#if 0
void UAV::ctr()
{
  // random
   theta =  (std::numbers::pi_v<double>)*(*(p -> ptr_UnifRealDist))(p -> RandomEngine);
   phi = 2*(std::numbers::pi_v<double>)*(*(p -> ptr_UnifRealDist))(p -> RandomEngine);

}  // ctr()
#endif



void UAV::ctr()
{
  double value;
  double tmp;
  int a, b, i;
  double thetap, phip;
  vector<double> z;
  double Vel[3];
  double best_value;
  double best_theta, best_phi;

  z.resize(3);
  
   best_value = DBL_MIN;
   best_theta = 0;
   best_phi = 0;
   
   for (a = 0; a <= p -> prm.Q; a++)
   for (b = 0; b <= p -> prm.Q; b++)
     {
       thetap = (std::numbers::pi_v<double>)*(((double) a)/((double) (p -> prm.Q)));
       phip = 2*(std::numbers::pi_v<double>)*(((double) b)/((double) (p -> prm.Q)));
       
   // define Vel
       
       Vel[0] = (p -> prm.V)*sin(thetap)*cos(phip);
       Vel[1] = (p -> prm.V)*sin(thetap)*sin(phip);
       Vel[2] = (p -> prm.V)*cos(thetap);
       

   for (i=0; i < 3; i++)
     {
       z[i] = x[i] + Vel[i]*(p -> prm.T);
     }


    value = distance(z);

       if (value > best_value)
	 {
           best_value = value;
	   best_theta = thetap;
	   best_phi = phip;	   
	 }
     }  // for a, b

   theta = best_theta;
   phi = best_phi;
   
}  // action()



double UAV::distance(vector<double> z)
{
  double value = 0;
  double tmp;
  int j, k;
  char buf[100];
  int uavpid;
  
  for (j = 0; j < p -> prm.N; j++)
    {
      sprintf(buf, "uav%d", j+1);
      uavpid = p -> dns[buf];

    // 1. Recupera il unique_ptr dal vettore
    auto& basePtr = p->sys->sysproc[uavpid];
    // 2. Esegui il downcast a UAV* 
    UAV* uavPtr = static_cast<UAV*>(basePtr.get());

	 for (k = 0; k < 3; k++)
     {
      //      value = value + pow((x[adr(i, k)] - x[adr(j, k)]), 2);
 
     // 3. Chiama il metodo
     // uavPtr -> .....
       
      tmp = (z[k] - (uavPtr -> x[k]))/(2*(p -> prm.L));      
      value = value + pow(tmp, 2);
     }
	 
    }
  
  return(value);
}  // distance()
