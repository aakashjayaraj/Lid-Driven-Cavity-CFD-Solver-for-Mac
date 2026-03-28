//
//  Solver.cpp
//  
//
//  Created by Aakash J on 28/03/26.
//

#include "Solver.h"
#include <cmath>

Solver::Solver(const Grid& grid, double Re, double dt)
    : grid_(grid), Re_(Re), dt_(dt),
      u_(grid.nx(), grid.ny()),
      v_(grid.nx(), grid.ny()),
      p_(grid.nx(), grid.ny()),
      uStar_(grid.nx(), grid.ny()),
      vStar_(grid.nx(), grid.ny()),
      rhs_(grid.nx(), grid.ny()) {
    applyBoundaryConditions();
}

void Solver::applyBoundaryConditions() {
    std::size_t nx = grid_.nx();
    std::size_t ny = grid_.ny();

    for (std::size_t j = 0; j < ny; ++j) {
        u_(0, j) = 0.0; v_(0, j) = 0.0;
        u_(nx - 1, j) = 0.0; v_(nx - 1, j) = 0.0;
    }
    for (std::size_t i = 0; i < nx; ++i) {
        u_(i, 0) = 0.0;
        v_(i, 0) = 0.0;
    }
    for (std::size_t i = 0; i < nx; ++i) {
        u_(i, ny - 1) = 1.0;
        v_(i, ny - 1) = 0.0;
    }

    for (std::size_t i = 0; i < nx; ++i) {
        p_(i, 0)      = p_(i, 1);
        p_(i, ny - 1) = p_(i, ny - 2);
    }
    for (std::size_t j = 0; j < ny; ++j) {
        p_(0, j)      = p_(1, j);
        p_(nx - 1, j) = p_(nx - 2, j);
    }
}

void Solver::computeIntermediateVelocity() {
    std::size_t nx = grid_.nx();
    std::size_t ny = grid_.ny();
    double dx = grid_.dx();
    double dy = grid_.dy();
    double dx2 = dx * dx;
    double dy2 = dy * dy;

    for (std::size_t j = 1; j < ny - 1; ++j) {
        for (std::size_t i = 1; i < nx - 1; ++i) {
            double du2dx = (std::pow(u_(i + 1, j), 2) - std::pow(u_(i - 1, j), 2)) / (2.0 * dx);
            double duvdy = ((u_(i, j + 1) * v_(i, j + 1)) -
                            (u_(i, j - 1) * v_(i, j - 1))) / (2.0 * dy);
            double d2udx2 = (u_(i + 1, j) - 2.0 * u_(i, j) + u_(i - 1, j)) / dx2;
            double d2udy2 = (u_(i, j + 1) - 2.0 * u_(i, j) + u_(i, j - 1)) / dy2;
            double dpdx = (p_(i + 1, j) - p_(i - 1, j)) / (2.0 * dx);

            uStar_(i, j) = u_(i, j)
                + dt_ * ( -du2dx - duvdy - dpdx + (1.0 / Re_) * (d2udx2 + d2udy2) );
        }
    }

    for (std::size_t j = 1; j < ny - 1; ++j) {
        for (std::size_t i = 1; i < nx - 1; ++i) {
            double dv2dy = (std::pow(v_(i, j + 1), 2) - std::pow(v_(i, j - 1), 2)) / (2.0 * dy);
            double duvdx = ((u_(i + 1, j) * v_(i + 1, j)) -
                            (u_(i - 1, j) * v_(i - 1, j))) / (2.0 * dx);
            double d2vdx2 = (v_(i + 1, j) - 2.0 * v_(i, j) + v_(i - 1, j)) / dx2;
            double d2vdy2 = (v_(i, j + 1) - 2.0 * v_(i, j) + v_(i, j - 1)) / dy2;
            double dpdy = (p_(i, j + 1) - p_(i, j - 1)) / (2.0 * dy);

            vStar_(i, j) = v_(i, j)
                + dt_ * ( -dv2dy - duvdx - dpdy + (1.0 / Re_) * (d2vdx2 + d2vdy2) );
        }
    }

    u_ = uStar_;
    v_ = vStar_;
    applyBoundaryConditions();
}

void Solver::solvePressurePoisson(int iterations) {
    std::size_t nx = grid_.nx();
    std::size_t ny = grid_.ny();
    double dx = grid_.dx();
    double dy = grid_.dy();
    double dx2 = dx * dx;
    double dy2 = dy * dy;

    for (std::size_t j = 1; j < ny - 1; ++j) {
        for (std::size_t i = 1; i < nx - 1; ++i) {
            double duxdx = (u_(i + 1, j) - u_(i - 1, j)) / (2.0 * dx);
            double dvydy = (v_(i, j + 1) - v_(i, j - 1)) / (2.0 * dy);
            rhs_(i, j) = (duxdx + dvydy) / dt_;
        }
    }

    Field pNew(nx, ny);

    for (int it = 0; it < iterations; ++it) {
        for (std::size_t j = 1; j < ny - 1; ++j) {
            for (std::size_t i = 1; i < nx - 1; ++i) {
                double term = ((p_(i + 1, j) + p_(i - 1, j)) * dy2 +
                               (p_(i, j + 1) + p_(i, j - 1)) * dx2);
                double denom = 2.0 * (dx2 + dy2);
                pNew(i, j) = (term - rhs_(i, j) * dx2 * dy2) / denom;
            }
        }
        for (std::size_t j = 1; j < ny - 1; ++j) {
            for (std::size_t i = 1; i < nx - 1; ++i) {
                p_(i, j) = pNew(i, j);
            }
        }
        applyBoundaryConditions();
    }
}

void Solver::correctVelocity() {
    std::size_t nx = grid_.nx();
    std::size_t ny = grid_.ny();
    double dx = grid_.dx();
    double dy = grid_.dy();

    for (std::size_t j = 1; j < ny - 1; ++j) {
        for (std::size_t i = 1; i < nx - 1; ++i) {
            double dpdx = (p_(i + 1, j) - p_(i - 1, j)) / (2.0 * dx);
            double dpdy = (p_(i, j + 1) - p_(i, j - 1)) / (2.0 * dy);

            u_(i, j) = u_(i, j) - dt_ * dpdx;
            v_(i, j) = v_(i, j) - dt_ * dpdy;
        }
    }
    applyBoundaryConditions();
}

void Solver::run(int nSteps, ProgressCallback progressCb) {
    for (int step = 0; step < nSteps; ++step) {
        computeIntermediateVelocity();
        solvePressurePoisson(40);
        correctVelocity();
        if (progressCb) {
            progressCb(step + 1, nSteps);
        }
    }
}
