#pragma once

#include "baseprocess.h"
#include "basesystem.h"

#include "channels.h"
#include "global.h"


class System : public BaseSystem
{

public:

   using BaseSystem::BaseSystem; // "Eredita" tutti i costruttori di BaseSystem
  
  // processes

 
  void create() override;  /* create system  */


};



