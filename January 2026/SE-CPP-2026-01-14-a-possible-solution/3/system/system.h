#pragma once

#include "channels.h"
#include "global.h"
#include "network.h"
#include "customer.h"
#include "server.h"
#include "supplier.h"
#include "mon1.h"


class System
{

public:

  System(Global *p, const char *mysysname);

  Global *p;  // pointer to global class

  char sysname[20];

  double time = 0;

  int pid;
  
  // components

  vector<unique_ptr<Network>> net;
  vector<unique_ptr<Customer>> customer1;
  vector<unique_ptr<Server>> server1;
  vector<unique_ptr<Supplier>> supplier1;
  vector<unique_ptr<Mon1>> monitor1;
  

/* create system  */
  void create();

  // define initial state
  void init();
    
  // update state
  void next_state();

private:
  double time_of_next_event();
  void create_customer();
  void create_server();
  void create_supplier();
};



