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
  
  int Collisions = 0;
  double Collision_rate = 0;

  void init() override;
  
  void update(int mypid);

private:
  
  double distance(int mypid, int farpid);

};



