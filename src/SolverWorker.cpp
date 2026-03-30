//
//  SolverWorker.cpp
//  
//
//  Created by Aakash Jayaraj on 28/03/26.
//

#include "SolverWorker.h"

SolverWorker::SolverWorker(int nx, int ny, double Re, double dt, int steps, QObject* parent)
    : QObject(parent),
      nx_(nx), ny_(ny), Re_(Re), dt_(dt), steps_(steps) {}

void SolverWorker::start() {
    Grid grid(static_cast<std::size_t>(nx_),
              static_cast<std::size_t>(ny_),
              1.0, 1.0);

    Solver solver(grid, Re_, dt_);

    solver.run(steps_, [this](int step, int total) {
        emit progress(step, total);
    });

    Field u = solver.u();
    Field v = solver.v();
    Field p = solver.p();

    emit finished(u, v, p);
    emit done();
}
