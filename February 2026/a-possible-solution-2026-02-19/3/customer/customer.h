#pragma once

#include "commons.h"
#include "global.h"
#include "channels.h"

#include "baseprocess.h"
#include "monitor.h"


  
class Customer : public BaseProcess
{
public:

  using BaseProcess::BaseProcess; // "Eredita" tutti i costruttori di BaseProcess


  Order ordervalue;

 // define initial state and time of next event
  void init() override;

 // define next state and time of next event
 // void next(double present_state) override;

private:
  
  
  unordered_map<int, Order> pending_orders =
    {
    };

  void send(double present_time);
  void receive(double present_time);
  double get_random_time();
  

};



