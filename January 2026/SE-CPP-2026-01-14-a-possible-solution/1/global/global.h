#pragma once

using namespace std;

#include "channels.h"

#define PRINT_LOG_FILE 10000

class System;  // will be defined later

class Parameters {

public:
  
  int N = 1; // number of UAV
  int M = 7; // Montecarlo simulations
  double L = 10;  // size area
  double A = 10;  // exp rate
  double T = 1;  // time step
  double R = 2;  // sampling time collision rate
  double D = 2;  // diameter collision rate
  double H = 10;  // horizon
  double V = 3; // velocity

  
};

  
class Global {

public:

 Global(default_random_engine RandEng);

 unordered_map<string, int> dns =
    {
        {"network", 0}
    };

  int first_available_pid = 1;
  
  int pidmoves = 0;
  
  int num_proc_type = 1; // just uav

  int num_uav = 1;

  double avg;  // where MOntecarlo values will be stored
  
  Parameters prm;
  System *sys;
  
  int num_processes = 0; // num processes excluding network
  
#if 0
  int num_processes()
  {return (num_uav*num_proc_type);}
#endif
  
  int uav_adr(int i)
  {return(i*num_proc_type);}

  int proc_type(int i)
  {
    return(i%num_proc_type);
  }

  int chadr2procadr(int i)
  {

    assert((i - proc_type(i))/num_proc_type >= 0);
    assert((i - proc_type(i))%num_proc_type == 0);
 
    return((i - proc_type(i))/num_proc_type) ;
  }


  int event=-2;  // -2, no event, -1 network, >=0 processes

  //  void init();

  
    // input channels to network
  ChMsg *Chp2n;

  // output channels from network
  ChMsg *Chn2p;
  
  FILE *fp;

  const char *logname;

  // unsigned int seed;

  default_random_engine RandomEngine;
  // default_random_engine RandomEngine;
  
  // Create a random device and use it to generate a random seed
  //random_device myRandomDevice;
  // unsigned int myseed = myRandomDevice();

  // Initialize a default_random_engine with the seed
  //default_random_engine myRandomEngine(myseed);
 
  // Initialize a uniform_int_distribution to produce values between 60 and 120
  // uniform_int_distribution<int> myUnifIntDist(10, 120);

  // Initialize a uniform_real_distribution to produce values between 0 and 1
  // uniform_real_distribution<double> myUnifRealDist(0.0, 1.0);

  
   // Initialize a uniform_int_distribution to produce values between 0 and 100
 // uniform_int_distribution<int> random_state(0, 100);
 //  uniform_int_distribution<int> random_state = uniform_int_distribution<int>(0, 100) ;
  uniform_int_distribution<int> *ptr_UnifIntDist;

    // Initialize a uniform_real_distribution to produce values between 0 and 1
  // uniform_real_distribution<double> UnifRealDist = uniform_real_distribution<double>(0.0, 1.0);
  uniform_real_distribution<double> *ptr_UnifRealDist;


    
};


//#endif
