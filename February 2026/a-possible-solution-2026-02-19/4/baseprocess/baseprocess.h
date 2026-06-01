#pragma once

#include "commons.h"
#include "global.h"
#include "channels.h"

  
class BaseProcess
{

public:

  BaseProcess(Global *p, int mypid, const char *mysysname);
  //virtual ~BaseProcess();


  Global *p;          // pointer to global class
  char sysname[20];   // process name
  int pid;            // process pid
  ChMsg inputqueue;   // process input queue

  double time_of_next_event = 0;

// define initial state and time of next event
  virtual void init();

 // define next state and time of next event
 void next(double present_state);


protected:

  enum class Event {
    send_output,
    receive_input
   };

  Event eventid;
  
  virtual void create() {};
  virtual void send(double present_time) {};
  virtual void receive(double present_time) {};

};



