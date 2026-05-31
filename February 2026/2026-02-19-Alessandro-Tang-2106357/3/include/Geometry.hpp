#pragma once

#include <cmath>
#include <utility>
#include <vector>

namespace SELib {
// ============================================================================
// 14. GEOMETRIC UTILITIES
// ============================================================================

// Point2D: Represents a point or vector in 2D space.
// Purpose:
//  - Provides basic operations like addition, subtraction, and distance
//  calculation.
// Usage:
//  Point2D p1(1.0, 2.0), p2(2.0, 3.0);
//  double dist = p1.distanceTo(p2);
//  Point2D sum = p1 + p2;
struct Point2D {
  double x, y;

  Point2D(double x_ = 0, double y_ = 0) : x(x_), y(y_) {}

  double distanceTo(const Point2D &other) const {
    double dx = x - other.x;
    double dy = y - other.y;
    return std::sqrt(dx * dx + dy * dy);
  }

  Point2D operator+(const Point2D &other) const {
    return Point2D(x + other.x, y + other.y);
  }

  Point2D operator-(const Point2D &other) const {
    return Point2D(x - other.x, y - other.y);
  }
};

// Point3D: Represents a point or vector in 3D space.
// Purpose:
//  - Provides basic operations like addition, subtraction, and distance
//  calculation.
// Usage:
//  Point3D p1(1.0, 2.0, 3.0), p2(2.0, 3.0, 4.0);
//  double dist = p1.distanceTo(p2);
//  Point3D sum = p1 + p2;
struct Point3D {
  double x, y, z;

  Point3D(double x_ = 0, double y_ = 0, double z_ = 0) : x(x_), y(y_), z(z_) {}

  double distanceTo(const Point3D &other) const {
    double dx = x - other.x;
    double dy = y - other.y;
    double dz = z - other.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
  }

  Point3D operator+(const Point3D &other) const {
    return Point3D(x + other.x, y + other.y, z + other.z);
  }

  Point3D operator-(const Point3D &other) const {
    return Point3D(x - other.x, y - other.y, z - other.z);
  }
};

// ============================================================================
// 19. COLLISION DETECTOR
// ============================================================================

// CollisionDetector: A utility class for detecting collisions between points
// in space. Purpose:
//  - Counts the number of collisions within a given distance threshold.
//  - Retrieves pairs of colliding points.
// Usage:
//  std::vector<Point2D> points = {Point2D(0, 0), Point2D(1, 1)};
//  int numCollisions = CollisionDetector::countCollisions(points, 1.5);
//  auto collidingPairs = CollisionDetector::getCollidingPairs(points, 1.5);
class CollisionDetector {
public:
  template <typename PointType>
  static int countCollisions(const std::vector<PointType> &positions,
                             double threshold) {
    int collisions = 0;
    for (size_t i = 0; i < positions.size(); i++) {
      for (size_t j = i + 1; j < positions.size(); j++) {
        if (positions[i].distanceTo(positions[j]) <= threshold) {
          collisions++;
        }
      }
    }
    return collisions;
  }

  template <typename PointType>
  static std::vector<std::pair<int, int>>
  getCollidingPairs(const std::vector<PointType> &positions, double threshold) {
    std::vector<std::pair<int, int>> pairs;
    for (size_t i = 0; i < positions.size(); i++) {
      for (size_t j = i + 1; j < positions.size(); j++) {
        if (positions[i].distanceTo(positions[j]) <= threshold) {
          pairs.push_back({i, j});
        }
      }
    }
    return pairs;
  }
};
} // namespace SELib
