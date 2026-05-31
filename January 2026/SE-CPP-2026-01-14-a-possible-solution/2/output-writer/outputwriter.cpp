#include "outputwriter.h"

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
    //fprintf(fp, "C %lg\n", p -> sys -> monitor1[0] -> collision_rate);
    

    // when using montecarlo
   fprintf(fp, "C %lg\n", p -> avg);
   
    fclose(fp);
    
#if (DEBUG > 1000)
printf("OutputWriter::write(): end\n");
#endif
    
}  // init()


