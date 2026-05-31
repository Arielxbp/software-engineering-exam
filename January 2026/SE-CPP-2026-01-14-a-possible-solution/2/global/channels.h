#pragma once

using namespace std;

#include "commons.h"


class Msg {
public:
  double timestamp;
  int sender;
  int receiver;
  char msg[100];
};

typedef queue<Msg> ChMsg;


