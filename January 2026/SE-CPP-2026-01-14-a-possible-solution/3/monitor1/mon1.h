#pragma once

#include "commons.h"
#include "global.h"
#include "channels.h"


// this monitor is not a process by iteself.
// it is called by the customer

// Collision Monitor  
class Mon1
{

public:

  Mon1(Global *p, const char *mysysname);

  //  ~MDP();
  
  Global *p;  // pointer to global class

  char sysname[20];

  int pid;

  int MissedSells = 0;
  double MissedSells_rate = 0;

  void init();
  
  void update(int asked, int received, double ptime);
     
};



