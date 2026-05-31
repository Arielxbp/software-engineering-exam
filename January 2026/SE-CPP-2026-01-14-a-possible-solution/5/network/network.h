#pragma once

#include "commons.h"
#include "global.h"
#include "channels.h"

class Network
{

public:

  Network(Global *p, const char *mysysname);

  Global *p;  // pointer to global class
  
  char sysname[100];
  
  // state
  int scanned = 0;

  int pid;
  
  int num_channels = 0;

  vector<int> scanner;
  
  // time of next update
  double time = 0;

/* create system */
//  void create();

// define initial state, output and update time
  void init();

  // define next state and  output and update time
  void next_state(double present_state);

private:

  void scan(double present_time);
 
 // computation time
  double ctime = 0;

 // sleep period 
  double stime = 0;

};



