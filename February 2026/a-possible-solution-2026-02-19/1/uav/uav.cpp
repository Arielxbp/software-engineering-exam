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
  
   theta =  (std::numbers::pi_v<double>)*(*(p -> ptr_UnifRealDist))(p -> RandomEngine);
   phi = 2*(std::numbers::pi_v<double>)*(*(p -> ptr_UnifRealDist))(p -> RandomEngine);

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




