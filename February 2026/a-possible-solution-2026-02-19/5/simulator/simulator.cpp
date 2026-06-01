#include "simulator.h"
#include "monitor.h"

Simulator::Simulator(Global *myp, System *mysys)
{

  p = myp;

  sys = mysys;

}



void Simulator::run(const char *logname)
{

#if (DEBUG > 100)
  fprintf(stderr, "Simulator::run(): begin\n");
#endif

  strcpy(logfilename, logname);

  // init monitors in p

  //p -> init();
  
  if (log) {initlog();}

  sys -> init();

#if (DEBUG > 100)
  fprintf(stderr, "Simulator::run(): sys -> init() done\n");
#endif
  
  if (log) {printlog();}

#if (DEBUG > 100)
  fprintf(stderr, "Simulator::run(): while begin\n");
#endif

  // while (sys -> time <=  p -> HORIZON)
    while (notermination())
    { 	
      sys -> next();
      if (log) {printlog();}
      
    }   // while ()

   
#if (DEBUG > 100)
  fprintf(stderr, "Simulator::run():  sys -> time = %lf\n", sys -> time);
#endif
  
  if (log) {closelog();}

#if (DEBUG > 100)
  fprintf(stderr, "Simulator::run(): end\n");
#endif

    
}  //  run()





void Simulator::initlog() {
  int i;
  
  logfp = fopen(logfilename, "w");
  if (!logfp)
    {
      fprintf(stderr, "Simulator::initlog(): failure\n");
      exit(1);
    }
  
  /* Print first line of log file s */

  //  fprintf(logfp, "# ");  // first line is a comment, skip with gnuplot

  fprintf(logfp, "present_time ");  // 
  fprintf(logfp, "MissedSells ");  // 
  fprintf(logfp, "MissedSells_rate\n");  // 
 
 


} // initlog()


    
void Simulator::printlog() {

  int i;
 
  /*  prg  */

  //  if (p -> event >= 0)
    // processes move
  //    {
  fprintf(logfp, "%lf ", sys -> time);  // present time

   // 1. Recupera il unique_ptr dal vettore 
  auto& basePtr = p->sys->sysproc[p->dns["monitor1"]];

  // 2. Esegui il downcast a Monitor* 
  Monitor* monitorPtr = static_cast<Monitor*>(basePtr.get());
 
  fprintf(logfp, "%d ",  monitorPtr->MissedSells);  
  fprintf(logfp, "%lg\n", monitorPtr->MissedSells_rate);  
  //    }

} // printlog()


void Simulator::closelog() {

#if (DEBUG > 1000)
  fprintf(stderr, "Simulator::closelog(): begin\n");
#endif
 
 fclose(logfp);

#if (DEBUG > 1000)
  fprintf(stderr, "Simulator::closelog(): end\n");
#endif 
}


int Simulator::notermination()
{

 
  //  return ((sys -> time < p -> prm.H) ? 1 : 0);

   return (sys -> time < p -> prm.H);
  
} // termination()
