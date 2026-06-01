#ifndef caohyuw296730qwe
#define caohyuw296730qwe

#include "commons.h"


using namespace std;

class Msg {
public:
  double timestamp;
  int sender;
  int receiver;
  char msg[100];
};

typedef queue<Msg> ChMsg;


#endif
