//
//  GpuMetalSolverBackend.cpp
//  
//
//  Created by Aakash Jayaraj on 30/03/26.
//

#include "GpuMetalSolverBackend.h"
#include "GpuMetalRunner.h"
#include <cmath>
#include <stdexcept>

GpuMetalSolverBackend::GpuMetalSolverBackend(const Grid& grid, double Re, double dt)
    : grid_(grid), Re_(Re), dt_(dt),
      u_(grid.nx(), grid.ny()),
      v_(grid.nx(), grid.ny()),
      uStar_(grid.nx(), grid.ny()),
      vStar_(grid.nx(), grid.ny()),
      p_(grid.nx(), grid.ny()),
      rhs_(grid.nx(), grid.ny()),
      pTemp_(grid.nx(), grid.ny()),
      runner_(new GpuMetalRunner()) {

    // init fields to zero
    for (std::size_t j = 0; j < grid_.ny(); ++j) {
        for (std::size_t i = 0; i < grid_.nx(); ++i) {
            u_(i, j) = 0.0;
            v_(i, j) = 0.0;
            p_(i, j) = 0.0;
            rhs_(i, j) = 0.0;
            uStar_(i, j) = 0.0;
            vStar_(i, j) = 0.0;
            pTemp_(i, j) = 0.0;
        }
    }

    applyVelocityBCsCpu();
}

GpuMetalSolverBackend::~GpuMetalSolverBackend() {
    delete runner_;
}

void GpuMetalSolverBackend::applyVelocityBCsCpu() {
    std::size_t nx = grid_.nx();
    std::size_t ny = grid_.ny();

    for (std::size_t j = 0; j < ny; ++j) {
        u_(0, j)      = 0.0; v_(0, j)      = 0.0;
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
}

void GpuMetalSolverBackend::computeIntermediateVelocityCpu() {
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
    applyVelocityBCsCpu();
}

void GpuMetalSolverBackend::computeRhsCpu() {
    std::size_t nx = grid_.nx();
    std::size_t ny = grid_.ny();
    double dx = grid_.dx();
    double dy = grid_.dy();

    for (std::size_t j = 1; j < ny - 1; ++j) {
        for (std::size_t i = 1; i < nx - 1; ++i) {
            double duxdx = (u_(i + 1, j) - u_(i - 1, j)) / (2.0 * dx);
            double dvydy = (v_(i, j + 1) - v_(i, j - 1)) / (2.0 * dy);
            rhs_(i, j) = (duxdx + dvydy) / dt_;
        }
    }
}

void GpuMetalSolverBackend::applyPressureBCsCpu() {
    std::size_t nx = grid_.nx();
    std::size_t ny = grid_.ny();

    for (std::size_t i = 0; i < nx; ++i) {
        p_(i, 0)      = p_(i, 1);
        p_(i, ny - 1) = p_(i, ny - 2);
    }
    for (std::size_t j = 0; j < ny; ++j) {
        p_(0, j)      = p_(1, j);
        p_(nx - 1, j) = p_(nx - 2, j);
    }
}

void GpuMetalSolverBackend::correctVelocityCpu() {
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

    applyVelocityBCsCpu();
}

void GpuMetalSolverBackend::run(int nSteps) {
    for (int step = 0; step < nSteps; ++step) {
        computeIntermediateVelocityCpu();
        computeRhsCpu();

        int jacobiIters = 40;
        for (int it = 0; it < jacobiIters; ++it) {
            runner_->jacobiPressureStep(grid_, p_, rhs_, pTemp_);
            p_ = pTemp_;
            applyPressureBCsCpu();
        }

        correctVelocityCpu();
    }
}
