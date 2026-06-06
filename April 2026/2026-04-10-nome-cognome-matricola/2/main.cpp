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

std::pair<int, int> predict_best_u_w(double theta, double phi, std::vector<UAV> &uavs, int current_index, double V, double T, int N, double a, double b , double r, RandomGenerator &rng) {
    
    std::vector<int> neighbor_indices;
    for (int j = 0; j < N; ++j) {
        if (j != current_index && uavs[current_index].pos.distanceTo(uavs[j].pos) <= r) {
            neighbor_indices.push_back(j);
        }
    }

    double best_F = -1e18;
    std::vector<std::pair<int, int>> best_u_w_pairs;

    for (int u = -1; u <= 1; ++u) {
        for (int w = -1; w <= 1; ++w) {

            double theta_tplus1 = theta + T * a * u;
            double phi_tplus1 = phi + T * b * w;
            double vx_t = V * std::sin(theta) * std::cos(phi);
            double vy_t = V * std::sin(theta) * std::sin(phi);
            double vz_t = V * std::cos(theta);
            double vx_tplus1 = V * std::sin(theta_tplus1) * std::cos(phi_tplus1);
            double vy_tplus1 = V * std::sin(theta_tplus1) * std::sin(phi_tplus1);
            double vz_tplus1 = V * std::cos(theta_tplus1);
            
            double x_tplus1 = uavs[current_index].pos.x + vx_t * T;
            double y_tplus1 = uavs[current_index].pos.y + vy_t * T;
            double z_tplus1 = uavs[current_index].pos.z + vz_t * T;

            double x_tplus2 = x_tplus1 + vx_tplus1 * T;
            double y_tplus2 = y_tplus1 + vy_tplus1 * T;
            double z_tplus2 = z_tplus1 + vz_tplus1 * T;

            Point3D predicted_pos(x_tplus2, y_tplus2, z_tplus2);

            double F = 1e18;

            for (int j : neighbor_indices) {
                F = std::min(F, predicted_pos.distanceTo(uavs[j].pos));
            }
            if (F > best_F) {
                best_F = F;
                best_u_w_pairs = {{u, w}};
            } else if (F == best_F) {
                best_u_w_pairs.emplace_back(u, w);
            }
        }
    }
    return best_u_w_pairs[rng.uniformInt(0, (int)best_u_w_pairs.size() - 1)];
}

double run_one_simulation(double T, double H, double L, double V, double D, int N, double a, double b, double r, RandomGenerator &rng) {

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
    int numSteps = (int)std::floor(H / T + 1e-9);

    std::vector<double> thetas(N);
    std::vector<double> phis(N);
    for (int i = 0; i < N; ++i) {
        thetas[i] = rng.uniform(0, M_PI);
        phis[i] = rng.uniform(0, 2 * M_PI);
    }

    // Simulate movement
    for (int t = 0; t < numSteps; ++t) {

        std::vector<std::pair<int, int>> uavs_u_w(N);
        
        // Il drone i sceglie u_i(t) e w_i(t) tali da massimizzare F(t+2, i). Cioè il drone
        // massimizza la distanza tra i suoi futuri vicini assumendo che non si muovano
        // Questo vale per tutti i droni in modo sincrono, quindi si calcolano prima tutti
        // i migliori u_i(t) e w_i(t) e poi si aggiornano le posizioni e le direzioni di tutti i droni

        // Predict u and w values for each UAV
        for (int i = 0; i < N; ++i) {
            uavs_u_w[i] = predict_best_u_w(thetas[i], phis[i], uavs, i, V, T, N, a, b, r, rng);
        }

        // Update position for each UAV
        for (int i = 0; i < N; ++i) {
            uavs[i].pos.x += V * T * std::sin(thetas[i]) * std::cos(phis[i]);
            uavs[i].pos.y += V * T * std::sin(thetas[i]) * std::sin(phis[i]);
            uavs[i].pos.z += V * T * std::cos(thetas[i]);

            thetas[i] = thetas[i] + T * a * uavs_u_w[i].first;
            phis[i] = phis[i] + T * b * uavs_u_w[i].second;

        }

        // Check collisions with every other UAV j > i and update totalCollisions
        for (int i = 0; i < N; ++i) {
            for (int j = i + 1; j < N; ++j) {
                if (uavs[i].pos.distanceTo(uavs[j].pos) < D) {
                    totalCollisions++;
                }
            }
        }
    }
    printf("Total collisions in this simulation: %lld\n", totalCollisions);
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
    double r = parser.getDouble("r"); // Minimum distance for best u_w prediction

    // Init total rate of collisions
    double totalCollisionRate = 0.0;

    // Run M simulations
    for (int i = 0; i < M; ++i) {
        totalCollisionRate += SELib::run_one_simulation(T, H, L, V, D, N, a, b, r, rng);
    }

    // Divide by M to get average collision rate
    double averageCollisionRate = totalCollisionRate / M;

    // Output result
    std::ofstream outFile("results.txt");
    outFile << "2026-05-31-Alessandro-Tang-2106357" << std::endl;
    outFile << "C " << averageCollisionRate << std::endl;
    return 0;
}
