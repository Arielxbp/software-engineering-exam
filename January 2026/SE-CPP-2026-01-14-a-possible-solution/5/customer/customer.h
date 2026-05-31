#pragma once

#include "commons.h"
#include "global.h"
#include "channels.h"

class Order
{
  public:
    int server;
    int item;
    int asked;
};

  
class Customer
{

public:

  Customer(Global *p, const char *mysysname);
  
  Global *p;  // pointer to global class

  char sysname[20];

  int pid;
  
  // time of next event
  double time = 0;

  Order ordervalue;

#if (DEBUG > 5)
  int mis = 0;
  double mis_rate = 0;
#endif
  
enum class Phase {
    send_output,
    receive_input
   };

  Phase ph;
  
  unordered_map<int, Order> pending_orders =
    {
    };

// define initial state, output and update time
  void init();

  // define next state and  output and update time
  void next_state(double present_state);


private:

  
 // send time
  double time_output = 0.01;

  // receive time
  double time_input = 0.01;

  void update_state(double present_time);
  void send(double present_time);
  void receive(double present_time);
  double get_random_time();
  

};



