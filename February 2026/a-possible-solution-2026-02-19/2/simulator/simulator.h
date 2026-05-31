#ifndef snvme140987dhw
#define snvme140987dhw


#include "global.h"
#include "system.h"



class Simulator {

  
public:

  Simulator(Global *myp, System *mysys);
  
  Global *p;

  System *sys;

  double Horizon;

  FILE *logfp;

  int log = 0;
  
  char logfilename[100];


// run simulation 
  void run(const char *logname);
  int notermination();
  
  vector<int> SizeChp2n;
  vector<int> SizeChn2p;

  void initlog();
  void printlog();
  void closelog();
  void print_msg_info(int i, int chtype, ChMsg q);
};


#endif
