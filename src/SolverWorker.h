//
//  SolverWorker.h
//  
//
//  Created by Aakash Jayaraj on 28/03/26.
//

#pragma once

#include <QObject>
#include "Grid.h"
#include "Solver.h"

class SolverWorker : public QObject {
    Q_OBJECT
public:
    SolverWorker(int nx, int ny, double Re, double dt, int steps, QObject* parent = nullptr);

signals:
    void progress(int step, int total);
    void finished(Field u, Field v, Field p);
    void done();

public slots:
    void start();

private:
    int nx_;
    int ny_;
    double Re_;
    double dt_;
    int steps_;
};
