#pragma once

#include "commons.h"
#include "global.h"
#include "channels.h"

class InputReader
{

public:

  InputReader(Global *p);

  Global *p;  // pointer to global class

  FILE *fptr;
  
  // parameters

   
  void FirstRead(const char* pfilename);

   
};


