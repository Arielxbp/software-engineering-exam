#include "global.h"


Global::Global(default_random_engine RandEng) {
  
 // myseed = myRandomDevice();

 RandomEngine = RandEng;


      //ptr_random_state = new uniform_int_distribution<int>::param_type(0, state_size -1);
  ptr_UnifIntDist = new uniform_int_distribution<int>(0, 1000000);


  
      //ptr_UnifRealDist = new uniform_real_distribution<double>::param_type(0, 1)
  ptr_UnifRealDist = new uniform_real_distribution<double>(0, 1);

  // update range of random generator
  //random_state.param(std::uniform_int_distribution<int>::param_type(0, state_size -1));
  
  //UnifRealDist.param(std::uniform_real_distribution<double>::param_type(0, 1));


}  // Global() 



