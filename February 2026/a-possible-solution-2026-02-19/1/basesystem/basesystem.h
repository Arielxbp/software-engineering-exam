#pragma once

#include "commons.h"
#include "global.h"
#include "channels.h"

#include "baseprocess.h"


class BaseSystem
{

public:

  BaseSystem(Global *p, const char *mysysname);
  // ~BaseSystem();
  
  Global *p;  // pointer to global class

  char sysname[20];

  double time = 0;   // current time
  
  // components

  vector<unique_ptr<BaseProcess>> sysproc;

  // define initial state
  void init();
    
  // update state
  void next();
  virtual void create() {} ;  /* create system  */

protected:
  void pidupd(); /* update pid */
  void pidinit(); /* init pid */


// template deve essere nel .h 
template <typename T>
void create_processes(int count, const char* prefix) {
        char buf[100];
	int i, pid;
    for (i = 0; i < count; ++i)
      {
        sprintf(buf, "%s%d", prefix, i + 1);       
        pid = p->first_available_pid;
	
#if (DEBUG > 1000)
	fprintf(stderr, "BaseSystem::create_processes(): creating process %s with pid %d: begin\n", buf, pid);
#endif	
        sysproc[pid] = make_unique<T>(p, pid, buf);
	
#if (DEBUG > 1000)
	fprintf(stderr, "BaseSystem::create_processes(): creating process %s with pid %d: done\n", buf, pid);
#endif
        pidupd();
      }
  }; // create_processes()

};



