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

double runOneSimAvoidance(double T, double H, int N, double L, double V, double D, double R, RandomGenerator &rng) {
    std::vector<UAV> uavs(N);
    for (int i = 0; i < N; ++i) uavs[i].pos = {rng.uniform(-L, L), rng.uniform(-L, L), rng.uniform(-L, L)};

    long long totalCollisions = 0;
    double nextCheckTime = 0.0;

    for (int t_idx = 0; (double)t_idx * T <= H + 1e-9; ++t_idx) {
        double currentTime = (double)t_idx * T;

        if (currentTime >= nextCheckTime - 1e-9) {
            for (int i = 0; i < N; ++i) {
                for (int j = i + 1; j < N; ++j) {
                    if (uavs[i].pos.distanceTo(uavs[j].pos) <= D) totalCollisions++;
                }
            }
            nextCheckTime += R;
        }

        std::vector<Point3D> nextPos(N);
        for (int i = 0; i < N; ++i) {
            double minVal = 1e18;
            std::vector<Point3D> bestMoves;
            for (int bit = 0; bit < 8; ++bit) {
                double v1 = (bit & 1) ? V : -V;
                double v2 = (bit & 2) ? V : -V;
                double v3 = (bit & 4) ? V : -V;
                Point3D pot = {uavs[i].pos.x + v1*T, uavs[i].pos.y + v2*T, uavs[i].pos.z + v3*T};
                
                double val = 0;
                for(int j=0; j<N; ++j) {
                    if(i==j) continue;
                    val += std::pow((pot.x - uavs[j].pos.x)/(2.0*L), 2) + 
                           std::pow((pot.y - uavs[j].pos.y)/(2.0*L), 2) + 
                           std::pow((pot.z - uavs[j].pos.z)/(2.0*L), 2);
                }
                if (val < minVal - 1e-12) { minVal = val; bestMoves = {pot}; }
                else if (std::abs(val - minVal) < 1e-12) { bestMoves.push_back(pot); }
            }
            nextPos[i] = bestMoves[rng.uniformInt(0, (int)bestMoves.size() - 1)];
        }
        for (int i = 0; i < N; ++i) uavs[i].pos = nextPos[i];
    }
    return (double)totalCollisions / H;
}

}

int main() {
    SELib::RandomGenerator rng(0);
    SELib::ParameterParser parser;
    if (!parser.parseFile("parameters.txt")) return 1;

    double T = parser.getDouble("T"), H = parser.getDouble("H"), L = parser.getDouble("L"), V = parser.getDouble("V"), D = parser.getDouble("D"), R = parser.getDouble("R");
    int M = parser.getInt("M"), N = parser.getInt("N");

    double totalRate = 0;
    for (int i = 0; i < M; ++i) totalRate += SELib::runOneSimAvoidance(T, H, N, L, V, D, R, rng);

    std::ofstream outFile("results.txt");
    outFile << "2026-01-14-Axel-Rubini-2158099" << std::endl;
    outFile << "C " << totalRate / M << std::endl;
    return 0;
}
