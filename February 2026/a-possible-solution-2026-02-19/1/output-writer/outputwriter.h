#ifndef ncjsk20ghfkdpoiwe89tyebd
#define ncjsk20ghfkdpoiwe89tyebd

#include "commons.h"
#include "global.h"
#include "channels.h"
#include "system.h"


class OutputWriter
{

public:

  OutputWriter(Global *p);

  Global *p;  // pointer to global class

  FILE *fptr;
  
  // parameters

    
  void write(System *sys, const char* pfilename);

   
};


#endif
