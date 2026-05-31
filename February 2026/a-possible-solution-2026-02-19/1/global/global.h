#pragma once

using namespace std;

#include "channels.h"
#include "heap.h"
#include "parameters.h"

#define PRINT_LOG_FILE 10000

class System;  // will be defined later

class Global {

public:

 Global(default_random_engine RandEng);

 unordered_map<string, int> dns =
    {
      //        {"network", 0}
    };

  TimeHeap eheap;
  
  int first_available_pid = 0;
  
  int transaction = 0;
  
  
  Parameters prm;
  
  System *sys;
  
  double avg;  // where Montecarlo values will be stored
  
  // soultion to opt problem
  int Budget;  // opt budget
  double R_opt;
  double J_opt;
  
  int num_processes = 0; // num processes excluding network
  
  //  void init();

  
#if 0
  // input channels to network
  ChMsg *Chp2n = NULL;

  // output channels from network
  ChMsg *Chn2p = NULL;

  vector<unique_ptr<ChMsg>> Chp2n;
  vector<unique_ptr<ChMsg>> Chn2p;
#endif

  
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
