//
//  ISolverBackend.h
//  
//
//  Created by Aakash Jayaraj on 30/03/26.
//

#pragma once

#include "Field.h"

class ISolverBackend {
public:
    virtual ~ISolverBackend() = default;

    virtual void run(int nSteps) = 0;

    virtual const Field& u() const = 0;
    virtual const Field& v() const = 0;
    virtual const Field& p() const = 0;
};
