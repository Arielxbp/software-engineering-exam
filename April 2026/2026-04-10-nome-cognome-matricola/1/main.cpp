#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <iomanip>
#include "include/IO.hpp"
#include "include/Random.hpp"
#include "include/Geometry.hpp"

namespace SELib {

struct UAV {
    Point3D pos;
};

double run_one_simulation(double T, double H, double L, double V, double D, int N, double a, double b, RandomGenerator &rng) {

    // Init UAVs
    std::vector<UAV> uavs(N);
    for (int i = 0; i < N; ++i) {
        uavs[i].pos = {rng.uniform(-L, L), rng.uniform(-L, L), rng.uniform(-L, L)};
    }

    long long totalCollisions = 0;


    // Number of steps
    int numSteps = (int)std::floor(H / T + 1e-9);

    std::vector<double> thetas(N);
    std::vector<double> phis(N);
    for (int i = 0; i < N; ++i) {
        thetas[i] = rng.uniform(0, M_PI);
        phis[i] = rng.uniform(0, 2 * M_PI);
    }

    // Simulate movement
    for (int t = 0; t < numSteps; ++t) {
        
        // Move each UAV in a random direction
        for (int i = 0; i < N; ++i) {   

            thetas[i] = thetas[i] + T * a * rng.gaussian(0, 1);
            phis[i] = phis[i] + T * b * rng.gaussian(0, 1);

            uavs[i].pos.x += V * T * std::sin(thetas[i]) * std::cos(phis[i]);
            uavs[i].pos.y += V * T * std::sin(thetas[i]) * std::sin(phis[i]);
            uavs[i].pos.z += V * T * std::cos(thetas[i]);

            // Check collisions with every other UAV j > i and update totalCollisions
            for (int j = 0; j < N; ++j) {
                if (j > i && uavs[i].pos.distanceTo(uavs[j].pos) < D) {
                    totalCollisions++;
                }
            }
        }
    }

    return (double)totalCollisions / H;
}

} 

int main() {

    // Generate random seed
    SELib::RandomGenerator rng(std::random_device{}());
    
    // Init parser
    SELib::ParameterParser parser;
    if (!parser.parseFile("parameters.txt")) {
        return 1;
    }

    // Extract parameters
    double T = parser.getDouble("T"); // Time step
    double H = parser.getDouble("H"); // Total simulation time
    double L = parser.getDouble("L"); // Area limit
    double V = parser.getDouble("V"); // Speed
    double D = parser.getDouble("D"); // Collision distance
    int N = parser.getInt("N"); // Number of UAVs
    int M = parser.getInt("M"); // Number of simulations
    double a = parser.getDouble("a"); // Angular acceleration for theta
    double b = parser.getDouble("b"); // Angular acceleration for phi

    // Init total rate of collisions
    double totalCollisionRate = 0.0;

    // Run M simulations
    for (int i = 0; i < M; ++i) {
        totalCollisionRate += SELib::run_one_simulation(T, H, L, V, D, N, a, b, rng);
    }

    // Divide by M to get average collision rate
    double averageCollisionRate = totalCollisionRate / M;

    // Output result
    std::ofstream outFile("results.txt");
    outFile << "2026-05-31-Alessandro-Tang-2106357" << std::endl;
    outFile << "C " << averageCollisionRate << std::endl;
    return 0;
}
