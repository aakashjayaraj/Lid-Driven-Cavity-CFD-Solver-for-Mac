//
//  Solver.h
//  
//
//  Created by Aakash J on 28/03/26.
//

#pragma once
#include "Grid.h"
#include "Field.h"
#include <functional>

class Solver {
public:
    // progressCallback(step, totalSteps)
    using ProgressCallback = std::function<void(int, int)>;

    Solver(const Grid& grid, double Re, double dt);

    void run(int nSteps, ProgressCallback progressCb = nullptr);

    const Field& u() const { return u_; }
    const Field& v() const { return v_; }
    const Field& p() const { return p_; }

private:
    const Grid& grid_;
    double Re_;
    double dt_;

    Field u_;
    Field v_;
    Field p_;
    Field uStar_;
    Field vStar_;
    Field rhs_;

    void applyBoundaryConditions();
    void computeIntermediateVelocity();
    void solvePressurePoisson(int iterations);
    void correctVelocity();
};
