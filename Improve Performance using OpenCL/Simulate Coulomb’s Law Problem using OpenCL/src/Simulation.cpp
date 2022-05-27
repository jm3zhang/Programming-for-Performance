#include "Simulation.h"

#include <cassert>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <iomanip>

using std::cerr;
using std::cout;
using std::endl;

//-----------------------------------------------------------------------------
// Helpers
//-----------------------------------------------------------------------------

bool retIsEmpty(const std::vector<Particle*> &ret) {
    for (Particle* p : ret) {
        if (p != nullptr) {
            return false;
        }
    }

    return true;
}

void reset(std::vector<Particle*> &v) {
    for (int i = 0; i < v.size(); i++) {
        delete v[i];
        v[i] = nullptr;
    }
}

//-----------------------------------------------------------------------------
// Simulation
//-----------------------------------------------------------------------------

Simulation::Simulation(float initTimeStep, float errorTolerance)
    : initTimeStep(initTimeStep)
    , errorTolerance(errorTolerance) {
    // nop
}

Simulation::~Simulation() {
    reset(y0);
    reset(y1);
    reset(z1);
}

//-----------------------------------------------------------------------------
// Input/output
//-----------------------------------------------------------------------------

void Simulation::readInputFile(std::string inputFile) {
    std::ifstream file(inputFile);
    if (!file.is_open()) {
        cerr << "Unable to open file: " << inputFile << endl;
        exit(1);
    }

    std::string line;
    while (getline(file, line)) {
        std::stringstream ss(line);
        std::string token;

        getline(ss, token, ',');
        Particle::ParticleType type = (token == "p" ? Particle::PROTON : Particle::ELECTRON);

        getline(ss, token, ',');
        float x = std::stod(token);

        getline(ss, token, ',');
        float y = std::stod(token);

        getline(ss, token, ',');
        float z = std::stod(token);

        y0.push_back(new Particle(
            type,
            Vec3(x, y, z)
        ));
    }
}

void Simulation::print() {
    int digitsAfterDecimal = 5;
    int width = digitsAfterDecimal + std::string("-0.e+00").length();

    for (const Particle* p : z1) {
        char type;
        switch (p->type) {
            case Particle::PROTON:
                type = 'p';
                break;
            case Particle::ELECTRON:
                type = 'e';
                break;
            default:
                type = 'u';
        }

        cout << type << ","
             << std::scientific
             << std::setprecision(digitsAfterDecimal)
             << std::setw(width)
             << p->position.x << ","
             << std::setw(width)
             << p->position.y << ","
             << std::setw(width)
             << p->position.z << endl;
    }
}

//-----------------------------------------------------------------------------
// OpenCL Simulation
//-----------------------------------------------------------------------------

#ifdef USE_OPENCL

#define __CL_ENABLE_EXCEPTIONS
#include <CL/cl.hpp>

void Simulation::run() {
    try { 
        // Get available platforms
        std::vector<cl::Platform> platforms;
        cl::Platform::get(&platforms);

        // Select the default platform and create a context using this platform and the GPU
        cl_context_properties cps[3] = { 
            CL_CONTEXT_PLATFORM, 
            (cl_context_properties)(platforms[0])(), 
            0 
        };
        cl::Context context(CL_DEVICE_TYPE_GPU, cps);
 
        // Get a list of devices on this platform
        std::vector<cl::Device> devices = context.getInfo<CL_CONTEXT_DEVICES>();
 
        // Create a command queue and use the first device
        cl::CommandQueue queue = cl::CommandQueue(context, devices[0]);
 
        // Read source file
        std::ifstream sourceFile("src/protons.cl");
            if(!sourceFile.is_open()){
                std::cerr << "Cannot find kernel file" << std::endl;
                throw;
            }
        std::string sourceCode(std::istreambuf_iterator<char>(sourceFile), (std::istreambuf_iterator<char>()));
        cl::Program::Sources source(1, std::make_pair(sourceCode.c_str(), sourceCode.length()+1));
 
        // Make program of the source code in the context
        cl::Program program = cl::Program(context, source);
 
        // Build program for these specific devices
        try {
            program.build(devices);
        } catch(cl::Error error) {
            std::cerr << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(devices[0]) << std::endl;
            throw;
        }
 
        // Make kernels
        cl::Kernel kernel_computeForces_k0(program, "computeForces");
        cl::Kernel kernel_computeApproxPositions(program, "computePositions");
        cl::Kernel kernel_computeForces_k1(program, "computeForces");
        cl::Kernel kernel_computeBetterPositions(program, "computePositions");
        cl::Kernel kernel_isErrorAcceptable(program, "isErrorAcceptable");
 
        // Create buffers
        std::vector<char> type;
        for (size_t i = 0; i < y0.size(); i++){
            switch (y0[i]->type) {
                case 1: 
                    type.push_back('p');
                    break;
                case 2: 
                    type.push_back('e');
                    break;
                default: 
                    type.push_back('u');
                    break;
            }
        }

        cl::Buffer buffer_type = cl::Buffer(
            context,
            CL_MEM_READ_WRITE,
            type.size() * sizeof(char)
        );

        std::vector<cl_float3> position_k0;
        std::vector<cl_float3> position_k1;
        for (size_t i = 0; i < y0.size(); i++){
            Vec3 position = y0[i]->position;
            position_k0.push_back((cl_float3){position.x, position.y, position.z});
            position_k1.push_back((cl_float3){position.x, position.y, position.z});
        }

        cl::Buffer buffer_position_k0 = cl::Buffer(
            context,
            CL_MEM_READ_WRITE,
            position_k0.size() * sizeof(cl_float3)
        );

        cl::Buffer buffer_position_k1 = cl::Buffer(
            context,
            CL_MEM_READ_WRITE,
            position_k1.size() * sizeof(cl_float3)
        );

        std::vector<cl_float3> force_k0;
        std::vector<cl_float3> force_k1;
        for (size_t i = 0; i < y0.size(); i++){
            force_k0.push_back((cl_float3){0, 0, 0});
            force_k1.push_back((cl_float3){0, 0, 0});
        }

        cl::Buffer buffer_force_k0 = cl::Buffer(
            context,
            CL_MEM_READ_WRITE,
            force_k0.size() * sizeof(cl_float3)
        );

        cl::Buffer buffer_force_k1 = cl::Buffer(
            context,
            CL_MEM_READ_WRITE,
            force_k1.size() * sizeof(cl_float3)
        );

        cl_float *h = new cl_float();
        *h = initTimeStep;
        cl::Buffer buffer_h = cl::Buffer(
            context,
            CL_MEM_READ_WRITE,
            sizeof(cl_float)
        );

        cl_int *errorAcceptable = new cl_int();
        *errorAcceptable = 0;
        cl::Buffer buffer_errorAcceptable = cl::Buffer(
            context,
            CL_MEM_READ_WRITE,
            sizeof(cl_int)
        );

        // Write buffers
        queue.enqueueWriteBuffer(
            buffer_type,
            CL_TRUE,
            0,
            type.size() * sizeof(char),
            type.data()
        );

        queue.enqueueWriteBuffer(
            buffer_position_k0,
            CL_TRUE,
            0,
            position_k0.size() * sizeof(cl_float3),
            position_k0.data()
        );

        queue.enqueueWriteBuffer(
            buffer_force_k0,
            CL_TRUE,
            0,
            force_k0.size() * sizeof(cl_float3),
            force_k0.data()
        );

        // Set arguments to kernel
        kernel_computeForces_k0.setArg(0, buffer_type);
        kernel_computeForces_k0.setArg(1, buffer_position_k0);
        kernel_computeForces_k0.setArg(2, buffer_force_k0);

        kernel_computeApproxPositions.setArg(0, buffer_type);
        kernel_computeApproxPositions.setArg(1, buffer_force_k0);
        kernel_computeApproxPositions.setArg(2, buffer_force_k0);
        kernel_computeApproxPositions.setArg(3, buffer_h);
        kernel_computeApproxPositions.setArg(4, buffer_position_k0);

        kernel_computeForces_k1.setArg(0, buffer_type);
        kernel_computeForces_k1.setArg(1, buffer_position_k0);
        kernel_computeForces_k1.setArg(2, buffer_force_k1);
 
        kernel_computeBetterPositions.setArg(0, buffer_type);
        kernel_computeBetterPositions.setArg(1, buffer_force_k0);
        kernel_computeBetterPositions.setArg(2, buffer_force_k1);
        kernel_computeBetterPositions.setArg(3, buffer_h);
        kernel_computeBetterPositions.setArg(4, buffer_position_k1);

        kernel_isErrorAcceptable.setArg(0, buffer_position_k1);
        kernel_isErrorAcceptable.setArg(1, buffer_position_k0);
        kernel_isErrorAcceptable.setArg(2, errorTolerance);
        kernel_isErrorAcceptable.setArg(3, buffer_errorAcceptable);

        cl::NDRange globalSize(y0.size());
        queue.enqueueNDRangeKernel(kernel_computeForces_k0, cl::NullRange, globalSize); 

        queue.enqueueWriteBuffer(
            buffer_h,
            CL_TRUE,
            0,
            sizeof(cl_float),
            h
        );

        while(*errorAcceptable != 1){
            // k1.clear();
            queue.enqueueWriteBuffer(
                buffer_force_k1,
                CL_TRUE,
                0,
                force_k1.size() * sizeof(cl_float3),
                force_k1.data()
            );

            // reset(y1);
            queue.enqueueWriteBuffer(
                buffer_position_k0,
                CL_TRUE,
                0,
                position_k0.size() * sizeof(cl_float3),
                position_k0.data()
            );

            // reset(z1);
            queue.enqueueWriteBuffer(
                buffer_position_k1,
                CL_TRUE,
                0,
                position_k1.size() * sizeof(cl_float3),
                position_k1.data()
            );

            // computeApproxPositions(h); // Compute y1
            queue.enqueueNDRangeKernel(kernel_computeApproxPositions, cl::NullRange, globalSize); 

            // computeForces(k1, y1); // Compute k1
            queue.enqueueNDRangeKernel(kernel_computeForces_k1, cl::NullRange, globalSize); 
            
            // computeBetterPositions(h); // Compute z1
            queue.enqueueNDRangeKernel(kernel_computeBetterPositions, cl::NullRange, globalSize); 

            // if (isErrorAcceptable(z1, y1)) {
            //     // Error is acceptable so we can stop simulation
            //     break;
            // }
            *errorAcceptable = 1;
            queue.enqueueWriteBuffer(
                buffer_errorAcceptable,
                CL_TRUE,
                0,
                sizeof(cl_int),
                errorAcceptable
            );
            queue.enqueueNDRangeKernel(kernel_isErrorAcceptable, cl::NullRange, globalSize); 
            queue.enqueueReadBuffer(
                buffer_errorAcceptable,
                CL_TRUE,
                0,
                sizeof(cl_int),
                errorAcceptable
            );

            *h /= 2.0;
            queue.enqueueWriteBuffer(
                buffer_h,
                CL_TRUE,
                0,
                sizeof(cl_float),
                h
            );
        }
 
        // Read buffer(s)
        std::vector<cl_float3> *output_position = new std::vector<cl_float3>(y0.size());
        queue.enqueueReadBuffer(
            buffer_position_k1,
            CL_TRUE,
            0,
            position_k1.size() * sizeof(cl_float3),
            (*output_position).data()
        );

        for (size_t i = 0; i < (*output_position).size(); i++) {
            int digitsAfterDecimal = 5;
            int width = digitsAfterDecimal + std::string("-0.e+00").length();
            cout << type[i] << ","
                << std::scientific
                << std::setprecision(digitsAfterDecimal)
                << std::setw(width)
                << (*output_position)[i].s[0] << ","
                << std::setw(width)
                << (*output_position)[i].s[1] << ","
                << std::setw(width)
                << (*output_position)[i].s[2] << endl;
        }
    
    } catch(cl::Error error) {
        std::cout << error.what() << "(" << error.err() << ")" << std::endl;
    }
}

// ifdef USE_OPENCL
#endif

//-----------------------------------------------------------------------------
// CPU Simulation
//-----------------------------------------------------------------------------

#ifndef USE_OPENCL

void Simulation::computeForces(std::vector<Vec3> &ret, const std::vector<Particle*> &particles) {
    assert(ret.size() == 0);
    ret.resize(particles.size());

    #pragma omp parallel for
    for (int i = 0; i < particles.size(); i++) {
        Vec3 totalForces;

        for (int j = 0; j < particles.size(); j++) {
            totalForces += particles[i]->computeForceOnMe(particles[j]);
        }

        ret[i] = totalForces;
    }

    assert(ret.size() == particles.size());
}

void Simulation::computeApproxPositions(const float h) {
    assert(y0.size() == k0.size());
    assert(retIsEmpty(y1));

    #pragma omp parallel for
    for (int i = 0; i < y0.size(); i++) {
        float mass = y0[i]->getMass();
        Vec3 f = k0[i];

        // h's unit is in seconds
        //
        //         F = ma
        //     F / m = a
        //   h F / m = v
        // h^2 F / m = d

        Vec3 deltaDist = f * std::pow(h, 2) / mass;
        y1[i] = new Particle(y0[i]->type, y0[i]->position + deltaDist);
    }
}

void Simulation::computeBetterPositions(const float h) {
    assert(y0.size() == k0.size());
    assert(y0.size() == k1.size());
    assert(retIsEmpty(z1));

    #pragma omp parallel for
    for (int i = 0; i < y0.size(); i++) {
        float mass = y0[i]->getMass();
        Vec3 f0 = k0[i];
        Vec3 f1 = k1[i];

        Vec3 avgForce = (f0 + f1) / 2.0;
        Vec3 deltaDist = avgForce * std::pow(h, 2) / mass;
        Vec3 y1 = y0[i]->position + deltaDist;

        z1[i] = new Particle(y0[i]->type, y1);
    }
}

bool Simulation::isErrorAcceptable(const std::vector<Particle*> &p0, const std::vector<Particle*> &p1) {
    assert(p0.size() == p1.size());

    bool errorAcceptable = true;

    #pragma omp parallel for
    for (int i = 0; i < p0.size(); i++) {
        if ((p0[i]->position - p1[i]->position).magnitude() > errorTolerance) {
            #pragma omp critical
            {
                errorAcceptable = false;
            }
        }
    }

    return errorAcceptable;
}

void Simulation::run() {
    const int numParticles = y0.size();
    k0.reserve(numParticles);
    k1.reserve(numParticles);
    y1.resize(numParticles);
    z1.resize(numParticles);

    float h = initTimeStep;

    k0.clear();
    computeForces(k0, y0); // Compute k0

    while (true) {
        k1.clear();
        reset(y1);
        reset(z1);

        computeApproxPositions(h); // Compute y1
        computeForces(k1, y1); // Compute k1
        computeBetterPositions(h); // Compute z1

        if (isErrorAcceptable(z1, y1)) {
            // Error is acceptable so we can stop simulation
            break;
        }

        h /= 2.0;
    }
}

// ifndef USE_OPENCL
#endif
