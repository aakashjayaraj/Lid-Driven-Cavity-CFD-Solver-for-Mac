//
//  CpuSolverBackend.cpp
//  
//
//  Created by Aakash Jayaraj on 30/03/26.
//

#include "CpuSolverBackend.h"

CpuSolverBackend::CpuSolverBackend(const Grid& grid, double Re, double dt)
    : solver_(grid, Re, dt) {}

void CpuSolverBackend::run(int nSteps) {
    solver_.run(nSteps, nullptr);  // progress handled outside for now
}
