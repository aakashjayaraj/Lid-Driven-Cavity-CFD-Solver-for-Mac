//
//  ComputeConfig.h
//  
//
//  Created by Aakash Jayaraj on 30/03/26.
//

#pragma once

enum class ComputeBackend {
    CpuSingle,
    CpuMulti,
    GpuMetal
};

struct ComputeOptions {
    ComputeBackend backend = ComputeBackend::CpuSingle;
    int cpuThreads = 1;  // used only if CpuMulti
};
