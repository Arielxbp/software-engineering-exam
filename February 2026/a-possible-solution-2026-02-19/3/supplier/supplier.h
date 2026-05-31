#pragma once

#include "commons.h"
#include "global.h"
#include "channels.h"

#include "baseprocess.h"


  
class Supplier : public BaseProcess
{

public:
 using BaseProcess::BaseProcess; // "Eredita" tutti i costruttori di BaseProcess

  
 // define initial state
  void init() override;

  // define next state
  // void next(double present_state) override;


  
private:
  
  void receive(double present_time) override;
  void send(double present_time) override;
  double get_random_time();
  

};



