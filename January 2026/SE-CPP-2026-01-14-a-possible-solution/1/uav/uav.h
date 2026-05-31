#pragma once

#include "commons.h"
#include "global.h"
#include "channels.h"

class ProbCost
{
public:
  double prob;   // transition  probability
  double cost;   // transition cost
};

  
class UAV
{

public:

  UAV(Global *p, const char *mysysname);
  
  Global *p;  // pointer to global class

  char sysname[100];

  int pid;
  
  // time of next event
  double time = 0;


  // state
  vector<double> x;
   
// create system
//  void create();

// define initial state, output and update time
  void init();

  // define next state and  output and update time
  void next_state(double present_state);

  int adr(int i, int j);

private:

  
 // send time
  double time_output = 0.1;

  // receive time
  double time_input = 0.1;

  // state update computation time
  double time_state = 0;

  void update_state();
  
  double compute_velocity(int j);

  double compute_probability(int j);
};



