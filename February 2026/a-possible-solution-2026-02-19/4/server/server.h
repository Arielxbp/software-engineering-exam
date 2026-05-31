#pragma once

#include "commons.h"
#include "global.h"
#include "channels.h"

#include "baseprocess.h"
#include "system.h"


class Server : public BaseProcess
{

public:

  using BaseProcess::BaseProcess; // "Eredita" tutti i costruttori di BaseProcess

// define initial state
  void init() override;

private:
  
  unordered_map<int, Order> pending_orders =
    {
    };
  
  // vector<int> available_items;
  
  Msg msg2send;
    
private:  
  void send(double present_time) override;
  void receive(double present_time) override;
  double get_random_time();
  

};



