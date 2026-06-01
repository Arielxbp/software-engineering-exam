#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include "include/IO.hpp"
#include "include/Random.hpp"
#include "include/Geometry.hpp"

namespace SELib {

struct UAV {
    Point3D pos;
};

// d(z, t) = sum_{j=1}^{N} sum_{k=1}^{3} ((z_k - x_{k,j}(t)) / (2L))^2
// Measures how spread-out candidate position z is from all current UAV positions.
double computeD(const Point3D &z, const std::vector<UAV> &uavs, double L) {
    double sum = 0.0;
    for (const auto &uav : uavs) {
        double dx = (z.x - uav.pos.x) / (2 * L);
        double dy = (z.y - uav.pos.y) / (2 * L);
        double dz = (z.z - uav.pos.z) / (2 * L);
        sum += dx * dx + dy * dy + dz * dz;
    }
    return sum;
}

// Returns candidate next position of UAV i given angles theta, phi.
Point3D nextPos(const Point3D &p, double theta, double phi, double V, double T) {
    return {
        p.x + V * std::sin(theta) * std::cos(phi) * T,
        p.y + V * std::sin(theta) * std::sin(phi) * T,
        p.z + V * std::cos(theta) * T
    };
}

double run_one_simulation(double T, double H, double L, double V, double D,
                          int N, int Q, RandomGenerator &rng) {

    // Init UAVs
    std::vector<UAV> uavs(N);
    for (int i = 0; i < N; ++i) {
        uavs[i].pos = {rng.uniform(-L, L), rng.uniform(-L, L), rng.uniform(-L, L)};
    }

    // Check initial collisions
    long long totalCollisions = 0;
    for (int i = 0; i < N; ++i) {
        for (int j = i + 1; j < N; ++j) {
            if (uavs[i].pos.distanceTo(uavs[j].pos) < D) {
                totalCollisions++;
            }
        }
    }

    // Number of steps
    int numSteps = (int)(std::floor(H / T + 1e-9));

    // Simulate movement
    for (int t = 0; t < numSteps; ++t) {

        // Move each UAV
        for (int i = 0; i < N; ++i) {

            // Grid search over (Q+1)^2 discrete (theta, phi) combinations.
            // theta_q = q/Q * pi,  phi_q = q/Q * 2*pi,  q = 0, 1, ..., Q
            double best = -1;
            
            std::vector<std::pair<double, double>> bestAngles;

            for (int qt = 0; qt <= Q; ++qt) {

                double theta = (double)qt / Q * M_PI;

                for (int qp = 0; qp <= Q; ++qp) {

                    double phi = (double)qp / Q * 2.0 * M_PI;

                    Point3D candidate = nextPos(uavs[i].pos, theta, phi, V, T);
                    double dval = computeD(candidate, uavs, L);

                    if (dval > best) {
                        best = dval;
                        bestAngles.clear();
                        bestAngles.push_back({theta, phi});
                    } else if (dval == best) {
                        // Ties: collect all maximisers, pick uniformly later.
                        bestAngles.push_back({theta, phi});
                    }
                }
            }

            // Choose uniformly at random among all maximisers.
            const int idx = rng.uniformInt(0, static_cast<int>(bestAngles.size()) - 1);
            const double theta = bestAngles[idx].first;
            const double phi = bestAngles[idx].second;

            // Move UAV i.
            uavs[i].pos = nextPos(uavs[i].pos, theta, phi, V, T);

            // Count collisions of UAV i with every j > i (sequential update).
            for (int j = i + 1; j < N; ++j)
                if (uavs[i].pos.distanceTo(uavs[j].pos) < D)
                    totalCollisions++;
        }
    }

    return static_cast<double>(totalCollisions) / H;
}

} // namespace SELib

int main() {

    SELib::RandomGenerator rng(std::random_device{}());

    SELib::ParameterParser parser;
    if (!parser.parseFile("parameters.txt")) { 
        return 1;
    }

    const double T = parser.getDouble("T");
    const double H = parser.getDouble("H");
    const double L = parser.getDouble("L");
    const double V = parser.getDouble("V");
    const double D = parser.getDouble("D");
    const int    N = parser.getInt("N");
    const int    M = parser.getInt("M");
    const int    Q = parser.getInt("Q");

    double totalCollisionRate = 0.0;
    for (int i = 0; i < M; ++i)
        totalCollisionRate += SELib::run_one_simulation(T, H, L, V, D, N, Q, rng);

    double averageCollisionRate = totalCollisionRate / M;

    std::ofstream outFile("results.txt");
    outFile << "2026-05-31-Alessandro-Tang-2106357\n";
    outFile << "C " << averageCollisionRate << "\n";

    return 0;
}