#pragma once

using namespace std;

#include "commons.h"


class Msg {
public:
  double timestamp;
  char sender[20];
  char receiver[20];
  char msg[100];
};

typedef queue<Msg> ChMsg;


