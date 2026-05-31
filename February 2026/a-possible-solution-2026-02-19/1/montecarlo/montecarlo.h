#pragma once

#include "commons.h"
#include "global.h"
#include "channels.h"
#include "simulator.h"

#include "monitor.h"

class MonteCarlo
{

public:

  MonteCarlo(Global *p, Simulator *sim);

  Global *p;  // pointer to global class

  Simulator *sim;
  
 // run montecarlo simulations
  void run(const char *logname);

private:
  
  double get_sample_value();
  
};
