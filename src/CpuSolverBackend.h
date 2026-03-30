//
//  CpuSolverBackend.h
//  
//
//  Created by Aakash Jayaraj on 30/03/26.
//

#pragma once

#include "ISolverBackend.h"
#include "Solver.h"
#include "Grid.h"

class CpuSolverBackend : public ISolverBackend {
public:
    CpuSolverBackend(const Grid& grid, double Re, double dt);

    void run(int nSteps) override;

    const Field& u() const override { return solver_.u(); }
    const Field& v() const override { return solver_.v(); }
    const Field& p() const override { return solver_.p(); }

private:
    Solver solver_;
};
