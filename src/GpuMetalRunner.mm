//
//  GpuMetalRunner.mm
//  
//
//  Created by Aakash Jayaraj on 30/03/26.
//

#import <Metal/Metal.h>
#import <Foundation/Foundation.h>

#include <stdexcept>
#include <fstream>
#include <sstream>
#include "GpuMetalRunner.h"

GpuMetalRunner::GpuMetalRunner()
    : device_(nullptr),
      library_(nullptr),
      jacobiPipeline_(nullptr),
      commandQueue_(nullptr) {

    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if (!device) {
        throw std::runtime_error("Metal device not available.");
    }

    // Load Metal source from GpuKernels.metal in working directory
    std::ifstream file("GpuKernels.metal");
    if (!file) {
        throw std::runtime_error("Failed to open GpuKernels.metal");
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string sourceStr = buffer.str();

    NSString* sourceNS = [[NSString alloc] initWithBytes:sourceStr.data()
                                                 length:sourceStr.size()
                                               encoding:NSUTF8StringEncoding];

    NSError* error = nil;
    id<MTLLibrary> library =
        [device newLibraryWithSource:sourceNS options:nil error:&error];
    if (!library) {
        std::string msg = "Failed to compile Metal library: ";
        if (error) {
            msg += [[error localizedDescription] UTF8String];
        }
        throw std::runtime_error(msg);
    }

    id<MTLFunction> jacobiFunc = [library newFunctionWithName:@"jacobiPressure2D"];
    if (!jacobiFunc) {
        throw std::runtime_error("Failed to find Metal kernel 'jacobiPressure2D'.");
    }

    id<MTLComputePipelineState> pipeline =
        [device newComputePipelineStateWithFunction:jacobiFunc error:&error];
    if (!pipeline) {
        std::string msg = "Failed to create compute pipeline state: ";
        if (error) {
            msg += [[error localizedDescription] UTF8String];
        }
        throw std::runtime_error(msg);
    }

    id<MTLCommandQueue> queue = [device newCommandQueue];
    if (!queue) {
        throw std::runtime_error("Failed to create Metal command queue.");
    }

    device_         = (__bridge_retained void*)device;
    library_        = (__bridge_retained void*)library;
    jacobiPipeline_ = (__bridge_retained void*)pipeline;
    commandQueue_   = (__bridge_retained void*)queue;
}

GpuMetalRunner::~GpuMetalRunner() {
    if (device_) {
        id<MTLDevice> device = (__bridge_transfer id<MTLDevice>)device_;
        (void)device;
    }
    if (library_) {
        id<MTLLibrary> library = (__bridge_transfer id<MTLLibrary>)library_;
        (void)library;
    }
    if (jacobiPipeline_) {
        id<MTLComputePipelineState> pipeline =
            (__bridge_transfer id<MTLComputePipelineState>)jacobiPipeline_;
        (void)pipeline;
    }
    if (commandQueue_) {
        id<MTLCommandQueue> queue =
            (__bridge_transfer id<MTLCommandQueue>)commandQueue_;
        (void)queue;
    }
}

void GpuMetalRunner::jacobiPressureStep(const Grid& grid,
                                        const Field& pOld,
                                        const Field& rhs,
                                        Field& pNew) {
    std::size_t nx = grid.nx();
    std::size_t ny = grid.ny();

    if (nx == 0 || ny == 0) {
        throw std::runtime_error("jacobiPressureStep: grid has zero size");
    }
    if (pOld.nx() != nx || pOld.ny() != ny ||
        rhs.nx()  != nx || rhs.ny()  != ny) {
        throw std::runtime_error("jacobiPressureStep: field size mismatch");
    }

    if (pNew.nx() != nx || pNew.ny() != ny) {
        const_cast<Field&>(pNew).resize(nx, ny);
    }

    std::size_t count = nx * ny;
    std::size_t bytes = count * sizeof(float);

    std::vector<float> pOldHost(count);
    std::vector<float> rhsHost(count);

    for (std::size_t j = 0; j < ny; ++j) {
        for (std::size_t i = 0; i < nx; ++i) {
            std::size_t idx = j * nx + i;
            pOldHost[idx] = static_cast<float>(pOld(i, j));
            rhsHost[idx]  = static_cast<float>(rhs(i, j));
        }
    }

    id<MTLDevice> device = (__bridge id<MTLDevice>)device_;
    id<MTLComputePipelineState> pipeline =
        (__bridge id<MTLComputePipelineState>)jacobiPipeline_;
    id<MTLCommandQueue> queue =
        (__bridge id<MTLCommandQueue>)commandQueue_;

    id<MTLBuffer> pOldBuffer = [device newBufferWithBytes:pOldHost.data()
                                                   length:bytes
                                                  options:MTLResourceStorageModeShared];

    id<MTLBuffer> rhsBuffer  = [device newBufferWithBytes:rhsHost.data()
                                                   length:bytes
                                                  options:MTLResourceStorageModeShared];

    id<MTLBuffer> pNewBuffer = [device newBufferWithLength:bytes
                                                   options:MTLResourceStorageModeShared];

    id<MTLCommandBuffer> cmdBuffer = [queue commandBuffer];
    id<MTLComputeCommandEncoder> encoder = [cmdBuffer computeCommandEncoder];

    [encoder setComputePipelineState:pipeline];
    [encoder setBuffer:pOldBuffer offset:0 atIndex:0];
    [encoder setBuffer:rhsBuffer  offset:0 atIndex:1];
    [encoder setBuffer:pNewBuffer offset:0 atIndex:2];

    uint nx_u = static_cast<uint>(nx);
    uint ny_u = static_cast<uint>(ny);
    float dx_f = static_cast<float>(grid.dx());
    float dy_f = static_cast<float>(grid.dy());

    [encoder setBytes:&nx_u length:sizeof(uint)  atIndex:3];
    [encoder setBytes:&ny_u length:sizeof(uint)  atIndex:4];
    [encoder setBytes:&dx_f length:sizeof(float) atIndex:5];
    [encoder setBytes:&dy_f length:sizeof(float) atIndex:6];

    MTLSize gridSize = MTLSizeMake(nx_u, ny_u, 1);
    NSUInteger w = pipeline.threadExecutionWidth;
    NSUInteger h = pipeline.maxTotalThreadsPerThreadgroup / w;
    if (h < 1) h = 1;
    MTLSize threadgroupSize = MTLSizeMake(w, h, 1);

    [encoder dispatchThreads:gridSize threadsPerThreadgroup:threadgroupSize];
    [encoder endEncoding];

    [cmdBuffer commit];
    [cmdBuffer waitUntilCompleted];

    float* pNewPtr = (float*)[pNewBuffer contents];
    for (std::size_t j = 0; j < ny; ++j) {
        for (std::size_t i = 0; i < nx; ++i) {
            const_cast<Field&>(pNew)(i, j) =
                static_cast<double>(pNewPtr[j * nx + i]);
        }
    }
}
