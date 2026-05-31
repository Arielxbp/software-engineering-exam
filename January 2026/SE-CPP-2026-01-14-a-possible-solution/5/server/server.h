#pragma once

#include "commons.h"
#include "global.h"
#include "channels.h"


  
class Server
{

public:

  Server(Global *p, const char *mysysname);
  
  Global *p;  // pointer to global class

  char sysname[20];

  int pid;

  int transaction;
  
  // time of next event
  double time = 0;
  
   
enum class Phase {
    send_output,
    receive_input
   };

  Phase ph;

  vector<int> available_items;
  
  Msg msg2send;
  

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



