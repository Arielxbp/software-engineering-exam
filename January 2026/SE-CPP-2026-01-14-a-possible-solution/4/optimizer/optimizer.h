#pragma once

#include "commons.h"
#include "global.h"
#include "channels.h"
#include "simulator.h"
#include "montecarlo.h"


class Optimizer
{

public:

  Optimizer(Global *p);

  Global *p;  // pointer to global class

  Simulator *sim;

  MonteCarlo *mc;
  
  int minsol = 1;
  int maxsol = 100;
  
  void init(Simulator *sim, MonteCarlo *mc);

  void objfun();
    
  // run optimizer
  void run();

  void minimize(int Budget);
    
};
