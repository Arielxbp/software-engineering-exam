

#include "main.h"


int main()
{

  REngine r;
  Global p(r.RandomEngine);
  System sys(&p, "sys");
  Simulator q(&p, &sys);
  InputReader Ireader(&p);
  OutputWriter Owriter(&p); 
  MonteCarlo mc(&p, &q);
  // Optimizer opt(&p);
  
  int i;
  
  /*  prg  */
  
#if (DEBUG > 0)
  setvbuf(stdout, (char*) NULL, _IONBF, 0);
  setvbuf(stderr, (char*) NULL, _IONBF, 0);
#endif

#if (DEBUG > 1000)
  fprintf(stderr, "main(): begin\n");
#endif
  
 // init global
 // p.init();
    p.sys = &sys;

  
#if (DEBUG > 1000)
  fprintf(stderr, "main(): p.init() done\n");
#endif

  // read parameters
  Ireader.FirstRead("parameters.txt");

#if (DEBUG > 1000)
  fprintf(stderr, "main(): parameters read\n");
#endif


  // create system 
  sys.create();
  
#if (DEBUG > 1000)
  fprintf(stderr, "main(): sys.create() done\n");
#endif



  // create MDP

  //   for (i = 0; i < p.num_mdp; i++)
  //    { sys.mdp1[i] -> create();}
   


  // no log with mc or opt
  q.log = 1;

  // run simulation
  //q.run("sim.log");

  // run montecarlo
  mc.run("sim.log");
  
#if (DEBUG > 1000)
  fprintf(stderr, "main(): simulation done\n");
#endif
  
  // opt.init(&q, &mc);
  // opt.minimize(p.Budget);
  
  Owriter.write(&sys, "results.txt");

#if (DEBUG > 1000)
  fprintf(stderr, "main(): output printed\n");
#endif

  return(0);
  
  
}  /*  main()  */

