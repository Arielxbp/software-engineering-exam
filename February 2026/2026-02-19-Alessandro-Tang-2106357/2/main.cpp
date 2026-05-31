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

// d(z, t) = sum_{j != excludeIdx} sum_k ((z_k - x_{k,j}(t)) / (2L))^2
// Measures total squared normalised distance from candidate position z
// to all other UAVs' current positions.
double computeD(const Point3D& z, const std::vector<UAV>& uavs, int excludeIdx, double L) {
    double val = 0.0;
    double inv2L = 1.0 / (2.0 * L);
    for (int j = 0; j < (int)uavs.size(); ++j) {
        if (j == excludeIdx) continue;
        double dx = (z.x - uavs[j].pos.x) * inv2L;
        double dy = (z.y - uavs[j].pos.y) * inv2L;
        double dz = (z.z - uavs[j].pos.z) * inv2L;
        val += dx*dx + dy*dy + dz*dz;
    }
    return val;
}


double runOneSim(double T, double H, int N, double L, double V, double D, int Q, RandomGenerator &rng) {
    std::vector<UAV> uavs(N);
    for (int i = 0; i < N; ++i)
        uavs[i].pos = {rng.uniform(-L, L), rng.uniform(-L, L), rng.uniform(-L, L)};

    long long totalCollisions = 0;
    const double PI = M_PI;

    for (int i = 0; i < N; ++i)
        for (int j = i + 1; j < N; ++j)
            if ((j > i) && uavs[i].pos.distanceTo(uavs[j].pos) < D)
                totalCollisions++;

    // Simulate: check collisions at t = 0, T, 2T, ..., floor(H/T)*T
    for (int t_idx = 0; (double)t_idx * T <= H + 1e-9; ++t_idx) {

        // Collision-avoidance policy: choose best (theta, phi) for each UAV
        // using the current positions of all UAVs, then update simultaneously.
        std::vector<Point3D> nextPos(N);
        for (int i = 0; i < N; ++i) {
            double bestD = -1e300;
            std::vector<std::pair<int,int>> bestPairs;

            // Enumerate all (Q+1)^2 discrete (theta, phi) combinations
            for (int qi = 0; qi <= Q; ++qi) {
                double theta = (double)qi / Q * PI;
                for (int qj = 0; qj <= Q; ++qj) {
                    double phi = (double)qj / Q * 2.0 * PI;

                    // Candidate next position: Xi(theta, phi, t+1)
                    Point3D next = {
                        uavs[i].pos.x + V * std::sin(theta) * std::cos(phi) * T,
                        uavs[i].pos.y + V * std::sin(theta) * std::sin(phi) * T,
                        uavs[i].pos.z + V * std::cos(theta) * T
                    };
                    
                    // Find the d value for this candidate position
                    double dVal = computeD(next, uavs, i, L);

                    if (dVal > bestD + 1e-12) {
                        bestD = dVal;
                        bestPairs = {{qi, qj}};
                    } else if (std::abs(dVal - bestD) <= 1e-12) {
                        bestPairs.push_back({qi, qj});
                    }
                }
            }

            // Randomly select one of the best (theta, phi) pairs and update position
            int idx = rng.uniformInt(0, (int)bestPairs.size() - 1);
            int qi = bestPairs[idx].first;
            int qj = bestPairs[idx].second;
            double theta = (double)qi / Q * PI;
            double phi   = (double)qj / Q * 2.0 * PI;

            nextPos[i] = {
                uavs[i].pos.x + V * std::sin(theta) * std::cos(phi) * T,
                uavs[i].pos.y + V * std::sin(theta) * std::sin(phi) * T,
                uavs[i].pos.z + V * std::cos(theta) * T
            };

            // Update the position of UAV i
            uavs[i].pos = nextPos[i];

            // Clamp to the box [-L, L]^3
            // uavs[i].pos.x = std::clamp(uavs[i].pos.x, -L, L);
            // uavs[i].pos.y = std::clamp(uavs[i].pos.y, -L, L);
            // uavs[i].pos.z = std::clamp(uavs[i].pos.z, -L, L);

            // Check for collisions with all other UAVs
            for (int j = 0; j < N; ++j) {
                if ((j > i) && nextPos[i].distanceTo(uavs[j].pos) < D)
                     totalCollisions++;
            }

        }
    }

    return (double)totalCollisions / H;
}

} // namespace SELib

int main() {
    SELib::RandomGenerator rng(std::random_device{}());
    SELib::ParameterParser parser;
    if (!parser.parseFile("parameters.txt")) return 1;

    double T = parser.getDouble("T"); // time step
    double H = parser.getDouble("H"); // simulation horizon
    double L = parser.getDouble("L"); // area half-width
    double V = parser.getDouble("V"); // absolute speed
    double D = parser.getDouble("D"); // collision distance threshold
    int M = parser.getInt("M");    // number of Monte Carlo simulations
    int N = parser.getInt("N");    // number of UAVs
    int Q = parser.getInt("Q");    // discretisation intervals

    double totalRate = 0.0;
    for (int i = 0; i < M; ++i)
        totalRate += SELib::runOneSim(T, H, N, L, V, D, Q, rng);

    std::ofstream outFile("results.txt");
    outFile << "2026-02-19-Alessandro-Tang-2106357" << std::endl;
    outFile << "C " << totalRate / M << std::endl;
    return 0;
}
