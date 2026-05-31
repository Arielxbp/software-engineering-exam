#include "outputwriter.h"
#include "monitor.h"

/*

read Parameters

*/


OutputWriter::OutputWriter(Global *myp)
{
  p = myp;

} // ReadParameters()



/* result file */
void OutputWriter::write(System *sys, const char* filename) {

    FILE *fp;

    fp = fopen(filename, "w");
    if (!fp) exit(1);  // failure

  // print signature

    fprintf(fp, "2025-01-09-Mario-Rossi-1234567\n");

    // print outputs

    // just one simulation
    //fprintf(fp, "R %lg\n", p -> sys -> monitor1[0] -> MissedSells_rate);

    // for debug
    //printf("R %lg\n", p -> sys -> monitor1[0] -> MissedSells_rate);
    

       // 1. Recupera il unique_ptr dal vettore 
    auto& basePtr = p->sys->sysproc[p->dns["monitor1"]];

    // 2. Esegui il downcast a Monitor* 
    Monitor* monitorPtr = static_cast<Monitor*>(basePtr.get());

   // 3. Chiama il metodo update 
   //monitorPtr->update(pid);

   // when using montecarlo and/or optimizer
    fprintf(fp, "R %lg\n", monitorPtr -> MissedSells_rate);

    //printf("R %lg\n", p -> avg);
  
    fclose(fp);
    
#if (DEBUG > 1000)
printf("OutputWriter::write(): end\n");
#endif
    
}  // init()


