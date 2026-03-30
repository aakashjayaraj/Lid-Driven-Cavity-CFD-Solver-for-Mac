//
//  GpuMetalSolverBackend.h
//  
//
//  Created by Aakash Jayaraj on 30/03/26.
//

#pragma once

#include "ISolverBackend.h"
#include "Grid.h"
#include "Field.h"

class GpuMetalRunner;

class GpuMetalSolverBackend : public ISolverBackend {
public:
    GpuMetalSolverBackend(const Grid& grid, double Re, double dt);
    ~GpuMetalSolverBackend();

    void run(int nSteps) override;

    const Field& u() const override { return u_; }
    const Field& v() const override { return v_; }
    const Field& p() const override { return p_; }

private:
    const Grid& grid_;
    double Re_;
    double dt_;

    Field u_;
    Field v_;
    Field uStar_;
    Field vStar_;
    Field p_;
    Field rhs_;
    Field pTemp_;

    GpuMetalRunner* runner_;

    void applyVelocityBCsCpu();
    void computeIntermediateVelocityCpu();
    void computeRhsCpu();
    void applyPressureBCsCpu();
    void correctVelocityCpu();
};
