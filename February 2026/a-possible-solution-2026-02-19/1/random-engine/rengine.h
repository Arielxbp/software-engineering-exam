#ifndef qbzkjhas8t48077
#define qbzkjhas8t48077

#include "commons.h"

using namespace std;

class REngine {

public:

  REngine();

  random_device myRandomDevice;
  seed_seq ssq{myRandomDevice()};

  // default_random_engine
  default_random_engine RandomEngine{ssq};

};


#endif
