#pragma once

using namespace std;

#include "commons.h"

class Parameters {

public:
  
  double H = 10;  // horizon
  double T = 1;  // time step (in seconds)
  int M = 1; // Montecarlo simulations
  double L = 10;
  double V = 1; // valore assoluto velocità
  int N = 1; // numero droni
  double D = 2;  // collision distance
};

  
