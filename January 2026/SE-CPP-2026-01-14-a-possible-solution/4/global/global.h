#pragma once

using namespace std;

#include "channels.h"

#define PRINT_LOG_FILE 10000

class System;  // will be defined later

class Parameters {

public:
  
  double H = 10;  // horizon
  double T = 1;  // time step, non used
  int M = 7; // Montecarlo simulations
  int S = 1; // number of server
  int C = 3; // number of servers
  int F = 3; // number of suppliers
  int P = 10;  // number of products
  int Q = 10;  // max num of items
  double A = 10;  // max quantity
  double B = 2;  // 
  double V = 2;  // 
  double W = 3; // 
  int G = 10;
  double a = 0.5;
  double b = 0.5;
  
};

  
class Global {

public:

 Global(default_random_engine RandEng);

 unordered_map<string, int> dns =
    {
      //        {"network", 0}
    };

  int first_available_pid = 0;
  
  int pidmoves = 0;
  
  int num_proc_type = 1; // just uav

  int transaction = 0;
  
  
  Parameters prm;
  System *sys;

  double avg;  // where Montecarlo values will be stored
  
  // soultion to opt problem
  int Budget;  // opt budget
  double R_opt;
  double J_opt;
  
  int num_processes = 0; // num processes excluding network
  

  int event=-2;  // -2, no event, -1 network, >=0 processes

  //  void init();

  
#if 0
  // input channels to network
  ChMsg *Chp2n = NULL;

  // output channels from network
  ChMsg *Chn2p = NULL;
#endif

  vector<unique_ptr<ChMsg>> Chp2n;
  vector<unique_ptr<ChMsg>> Chn2p;
  
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
