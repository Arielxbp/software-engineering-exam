#pragma once

#include "commons.h"
#include "global.h"
#include "channels.h"
#include "baseprocess.h"


// Collision Monitor  
class Monitor : public BaseProcess
{

public:

  using BaseProcess::BaseProcess; // "Eredita" tutti i costruttori di BaseProcess
  
  int MissedSells = 0;
  double MissedSells_rate = 0;

  void init() override;

 // define next state and time of next event
  
  void update(int asked, int received, double ptime);

};



