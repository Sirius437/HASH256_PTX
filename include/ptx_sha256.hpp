#pragma once

#include <cuda.h>
#include <cuda_runtime.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>

class PTX_SHA256 {
public:
    PTX_SHA256() : module_(nullptr), kernel_(nullptr), initialized_(false) {}
    
    ~PTX_SHA256() {
        cleanup();
    }
    
    // Initialize and compile PTX kernel
    bool initialize(const std::string& ptx_file_path) {
        try {
            // Initialize CUDA driver API
            CUresult result = cuInit(0);
            if (result != CUDA_SUCCESS) {
                std::cerr << "Failed to initialize CUDA driver API" << std::endl;
                return false;
            }
            
            // Get device
            CUdevice device;
            result = cuDeviceGet(&device, 0);
            if (result != CUDA_SUCCESS) {
                std::cerr << "Failed to get CUDA device" << std::endl;
                return false;
            }
            
            // Create context
            result = cuCtxCreate(&context_, 0, device);
            if (result != CUDA_SUCCESS) {
                std::cerr << "Failed to create CUDA context" << std::endl;
                return false;
            }
            
            // Read PTX source
            std::string ptx_source = read_ptx(ptx_file_path);
            if (ptx_source.empty()) {
                return false;
            }
            
            // JIT compile PTX
            if (!compile_ptx(ptx_source)) {
                return false;
            }
            
            initialized_ = true;
            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "Exception during initialization: " << e.what() << std::endl;
            return false;
        }
    }
    
    // Hash multiple public keys
    bool hash_batch(const uint8_t* h_input, uint8_t* h_output, uint32_t num_keys) {
        if (!initialized_) {
            std::cerr << "PTX_SHA256 not initialized" << std::endl;
            return false;
        }
        
        // Allocate device memory
        CUdeviceptr d_input, d_output;
        size_t input_size = num_keys * 33;   // 33 bytes per compressed pubkey
        size_t output_size = num_keys * 32;  // 32 bytes per SHA256 hash
        
        CUresult result = cuMemAlloc(&d_input, input_size);
        if (result != CUDA_SUCCESS) {
            std::cerr << "Failed to allocate input memory" << std::endl;
            return false;
        }
        
        result = cuMemAlloc(&d_output, output_size);
        if (result != CUDA_SUCCESS) {
            cuMemFree(d_input);
            std::cerr << "Failed to allocate output memory" << std::endl;
            return false;
        }
        
        // Copy input to device
        result = cuMemcpyHtoD(d_input, h_input, input_size);
        if (result != CUDA_SUCCESS) {
            cuMemFree(d_input);
            cuMemFree(d_output);
            std::cerr << "Failed to copy input to device" << std::endl;
            return false;
        }
        
        // Set kernel parameters
        void* args[] = {
            &d_input,
            &d_output,
            &num_keys
        };
        
        // Launch kernel - using smaller block size for better occupancy
        int threads_per_block = 128;  // Reduced from 256 for better occupancy with 40 registers
        int blocks = (num_keys + threads_per_block - 1) / threads_per_block;
        
        result = cuLaunchKernel(
            kernel_,
            blocks, 1, 1,                    // grid dimensions
            threads_per_block, 1, 1,         // block dimensions
            0,                                // shared memory
            nullptr,                          // stream
            args,                             // kernel arguments
            nullptr                           // extra
        );
        
        if (result != CUDA_SUCCESS) {
            cuMemFree(d_input);
            cuMemFree(d_output);
            std::cerr << "Failed to launch kernel" << std::endl;
            return false;
        }
        
        // Wait for completion
        result = cuCtxSynchronize();
        if (result != CUDA_SUCCESS) {
            cuMemFree(d_input);
            cuMemFree(d_output);
            std::cerr << "Kernel execution failed" << std::endl;
            return false;
        }
        
        // Copy output back to host
        result = cuMemcpyDtoH(h_output, d_output, output_size);
        if (result != CUDA_SUCCESS) {
            cuMemFree(d_input);
            cuMemFree(d_output);
            std::cerr << "Failed to copy output from device" << std::endl;
            return false;
        }
        
        // Free device memory
        cuMemFree(d_input);
        cuMemFree(d_output);
        
        return true;
    }
    
private:
    CUmodule module_;
    CUfunction kernel_;
    CUcontext context_;
    bool initialized_;
    
    std::string read_ptx(const std::string& ptx_file_path) {
        std::ifstream file(ptx_file_path);
        if (!file.is_open()) {
            std::cerr << "Failed to open PTX file: " << ptx_file_path << std::endl;
            return "";
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
    
    bool compile_ptx(const std::string& ptx_source) {
        const unsigned int num_options = 7;
        CUjit_option options[num_options];
        void* option_values[num_options];
        float walltime;
        const unsigned int log_size = 8192;
        char error_log[log_size], info_log[log_size];
        
        // Setup linker options
        options[0] = CU_JIT_WALL_TIME;
        option_values[0] = (void*)&walltime;
        options[1] = CU_JIT_INFO_LOG_BUFFER;
        option_values[1] = (void*)info_log;
        options[2] = CU_JIT_INFO_LOG_BUFFER_SIZE_BYTES;
        option_values[2] = (void*)(long)log_size;
        options[3] = CU_JIT_ERROR_LOG_BUFFER;
        option_values[3] = (void*)error_log;
        options[4] = CU_JIT_ERROR_LOG_BUFFER_SIZE_BYTES;
        option_values[4] = (void*)(long)log_size;
        options[5] = CU_JIT_LOG_VERBOSE;
        option_values[5] = (void*)1;
        options[6] = CU_JIT_OPTIMIZATION_LEVEL;
        option_values[6] = (void*)4;  // Maximum optimization
        
        // Create linker
        CUlinkState linker_state;
        CUresult result = cuLinkCreate(num_options, options, option_values, &linker_state);
        if (result != CUDA_SUCCESS) {
            std::cerr << "Failed to create linker" << std::endl;
            return false;
        }
        
        // Add PTX source
        result = cuLinkAddData(
            linker_state,
            CU_JIT_INPUT_PTX,
            (void*)ptx_source.c_str(),
            ptx_source.size(),
            "sha256_kernel.ptx",
            0, nullptr, nullptr
        );
        
        if (result != CUDA_SUCCESS) {
            std::cerr << "Failed to add PTX source to linker" << std::endl;
            std::cerr << "Error log: " << error_log << std::endl;
            cuLinkDestroy(linker_state);
            return false;
        }
        
        // Complete linking
        void* cubin_out;
        size_t cubin_size;
        result = cuLinkComplete(linker_state, &cubin_out, &cubin_size);
        if (result != CUDA_SUCCESS) {
            std::cerr << "Failed to complete linking" << std::endl;
            std::cerr << "Error log: " << error_log << std::endl;
            cuLinkDestroy(linker_state);
            return false;
        }
        
        std::cout << "CUDA Link Completed in " << walltime << " ms." << std::endl;
        std::cout << "Linker Output:\n" << info_log << std::endl;
        
        // Load module
        result = cuModuleLoadData(&module_, cubin_out);
        if (result != CUDA_SUCCESS) {
            std::cerr << "Failed to load module" << std::endl;
            cuLinkDestroy(linker_state);
            return false;
        }
        
        // Get kernel function
        result = cuModuleGetFunction(&kernel_, module_, "sha256_kernel");
        if (result != CUDA_SUCCESS) {
            std::cerr << "Failed to get kernel function" << std::endl;
            cuModuleUnload(module_);
            cuLinkDestroy(linker_state);
            return false;
        }
        
        // Cleanup linker
        cuLinkDestroy(linker_state);
        
        return true;
    }
    
    void cleanup() {
        if (kernel_) {
            kernel_ = nullptr;
        }
        if (module_) {
            cuModuleUnload(module_);
            module_ = nullptr;
        }
        if (context_) {
            cuCtxDestroy(context_);
            context_ = nullptr;
        }
        initialized_ = false;
    }
};
