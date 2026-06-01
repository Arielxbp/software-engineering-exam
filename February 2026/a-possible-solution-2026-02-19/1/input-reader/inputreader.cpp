#include "inputreader.h"

/*

read Parameters

*/


InputReader::InputReader(Global *myp)
{
  p = myp;

} // ReadParameters()



/* read parameters to create dynamic data */
void InputReader::FirstRead(const char* prmfilename) {

    char *line = NULL;   // buffer; getline() allocates memory if NULL
    size_t len = 0;      // initial size
    ssize_t read;        // number of chars read
    FILE *fp;

    
    int i = 0, j = 0;
    double prob = 0, cost = 0;
    
    fp = fopen(prmfilename, "r");
    if (!fp) exit(1);  // failure

#if (DEBUG > 100)
        printf("FirstRead(): Begin\n");
#endif

	while ((read = getline(&line, &len, fp)) != -1) {
      //       printf("Retrieved line (%zd chars): %s", read, line);


 	  sscanf(line, "T %lf", &(p -> prm.T));

	  sscanf(line, "H %lf", &(p -> prm.H));

	  sscanf(line, "M %d", &(p -> prm.M));

	  sscanf(line, "V %lf", &(p -> prm.V));

	  sscanf(line, "L %lf", &(p -> prm.L));

	  sscanf(line, "D %lf", &(p -> prm.D));

	  sscanf(line, "N %d", &(p -> prm.N));

	} // while


    free(line);     // MUST free buffer, ok even if line is NULL
    fclose(fp);
    

}  // read()




