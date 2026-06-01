#pragma once

#include "commons.h"
#include "global.h"
#include "channels.h"

#include "baseprocess.h"
#include "monitor.h"

  
class UAV : public BaseProcess
{
public:

  using BaseProcess::BaseProcess; // "Eredita" tutti i costruttori di BaseProcess

 // define initial state and time of next event
  void init() override;

 // define next state and time of next event
 // void next(double present_state) override;

  double x[3];
  double v[3];
  double theta;
  double phi;
  
private:
  
  void receive(double present_time) override;

};



