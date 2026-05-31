#pragma once

#include "commons.h"
#include "global.h"
#include "channels.h"



// Collision Monitor  
class Mon1
{

public:

  Mon1(Global *p, const char *mysysname);

  //  ~MDP();
  
  Global *p;  // pointer to global class

  char sysname[100];

  int pid;

  // time of next event
  double time = 0;

  int collisions = 0;
  double collision_rate = 0;
  
   
// create system
  void create();

// define initial state, output and update time
  void init();

  // define next state and  output and update time
  void next_state(double present_time);

  int adr(int i, int j);

private:

 
  double distance(int i, int j);
    
};



