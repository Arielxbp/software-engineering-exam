#pragma once

using namespace std;

#include "commons.h"

class Parameters {

public:
  
  double H = 10;  // horizon
  double T = 1;  // time step (in seconds)
  int M = 1; // Montecarlo simulations
  
  int Q = 2;  // numero items
  int P = 2;  // numero prodotti
  int C = 2;  // numero customers
  int S = 2;  // numero servers
  int F = 2;  // numero suppliers (fornitori)
  
  double A = 2;  // min send time customers
  double B = 3;  // max send time customers
  double V = 2;  // min send time suppleirs
  double W = 3;  // max send time suppliers
  double r = 2;  // read time from network
  double w = 2;  // write time from network
  double l = 2;  // read time from db
  double s = 2;  // write time from db
  double z = 2;  // read time from server
  double v = 2;  // write time from server
  int G = 100;  // opt budget
  double a = 0.2;  // opt weight db time
  double b = 0.8;  // opt weight rate

};

  
