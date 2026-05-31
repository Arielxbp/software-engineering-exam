#pragma once

#include "channels.h"
#include "global.h"
#include "network.h"
#include "uav.h"
#include "mon1.h"


class System
{

public:

  System(Global *p, const char *mysysname);

  Global *p;  // pointer to global class

  char sysname[100];

  double time = 0;

  int pid;
  
  // components

  vector<unique_ptr<Network>> net;
  vector<unique_ptr<UAV>> uav1;
  vector<unique_ptr<Mon1>> monitor1;
  

/* create system  */
  void create();

  // define initial state
  void init();
    
  // update state
  void next_state();

private:
  double time_of_next_event();
  
};



