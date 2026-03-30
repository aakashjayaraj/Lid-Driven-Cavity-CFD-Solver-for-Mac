//
//  GpuMetalRunner.h
//  
//
//  Created by Aakash Jayaraj on 30/03/26.
//

// src/GpuMetalRunner.h
#pragma once

#include <vector>
#include "Field.h"
#include "Grid.h"

class GpuMetalRunner {
public:
    GpuMetalRunner();
    ~GpuMetalRunner();
    
    // One Jacobi iteration: pOld, rhs -> pNew
    void jacobiPressureStep(const Grid& grid,
                            const Field& pOld,
                            const Field& rhs,
                            Field& pNew);

private:
    void* device_;
    void* library_;
    void* jacobiPipeline_;
    void* commandQueue_;
};
