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

double runOneSim(double T, double H, int N, double L, double V, double A, double D, double R, RandomGenerator &rng) {
    std::vector<UAV> uavs(N);
    for (int i = 0; i < N; ++i) uavs[i].pos = {rng.uniform(-L, L), rng.uniform(-L, L), rng.uniform(-L, L)};

    long long totalCollisions = 0;
    double nextCheckTime = 0.0;

    for (int t_idx = 0; (double)t_idx * T <= H + 1e-9; ++t_idx) {
        double currentTime = (double)t_idx * T;

        // 1. Collision check every R seconds
        if (currentTime >= nextCheckTime - 1e-9) {
            for (int i = 0; i < N; ++i) {
                for (int j = i + 1; j < N; ++j) {
                    if (uavs[i].pos.distanceTo(uavs[j].pos) <= D) totalCollisions++;
                }
            }
            nextCheckTime += R;
        }

        // 2. Move
        for (int i = 0; i < N; ++i) {
            double x[3] = {uavs[i].pos.x, uavs[i].pos.y, uavs[i].pos.z};
            double new_x[3];
            for (int k = 0; k < 3; ++k) {
                double prob = std::exp(-A * (x[k] + L) / (2.0 * L));
                double vk = rng.bernoulli(std::clamp(prob, 0.0, 1.0)) ? V : -V;
                new_x[k] = x[k] + vk * T;
            }
            uavs[i].pos = {new_x[0], new_x[1], new_x[2]};
        }
    }
    return (double)totalCollisions / H;
}

}

int main() {
    SELib::RandomGenerator rng(0);
    SELib::ParameterParser parser;
    if (!parser.parseFile("parameters.txt")) return 1;

    double T = parser.getDouble("T"), H = parser.getDouble("H"), L = parser.getDouble("L"), V = parser.getDouble("V"), A = parser.getDouble("A"), D = parser.getDouble("D"), R = parser.getDouble("R");
    int M = parser.getInt("M"), N = parser.getInt("N");

    double totalRate = 0;
    for (int i = 0; i < M; ++i) totalRate += SELib::runOneSim(T, H, N, L, V, A, D, R, rng);

    std::ofstream outFile("results.txt");
    outFile << "2026-01-14-Axel-Rubini-2158099" << std::endl;
    outFile << "C " << totalRate / M << std::endl;
    return 0;
}
