#define __CL_ENABLE_EXCEPTIONS

#include <CL/cl.hpp>

#include <string>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <math.h>  

using std::cout;
using std::cerr;
using std::endl;

//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------

const int RANDOM_SEED = 1138;


//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------

void print_results( float a_pct, float b_pct, float t_pct) {
    cout << "Candidate A wins " << a_pct << "% of the time." << endl;
    cout << "Candidate B wins " << b_pct << "% of the time." << endl;
    cout << "Ties " << t_pct << "% of the time." << endl;
}


int main(int argc, char const *argv[]) {
    cout << std::setprecision(4);

    // Use OpenCL to run simulation
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
        std::ifstream sourceFile("q6/election.cl");
            if(!sourceFile.is_open()){
                std::cerr << "Cannot find kernel file" << std::endl;
                throw;
            }
        std::string sourceCode(std::istreambuf_iterator<char>(sourceFile), (std::istreambuf_iterator<char>()));
        cl::Program::Sources source(1, std::make_pair(sourceCode.c_str(), sourceCode.length() + 1));
 
        // Make program of the source code in the context
        cl::Program program = cl::Program(context, source);
 
        // Build program for these specific devices
        std::string v = "-D RANDOM_SEED=" + std::to_string( RANDOM_SEED );
        char variable[v.size()];
        strcpy(variable, v.c_str());
        try {
            program.build(devices, variable);
        } catch(cl::Error error) {
            std::cerr << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(devices[0]) << std::endl;
            throw;
        }
 
        // Make kernel(s)
        cl::Kernel kernel_elect(program, "elect");
 
        // Create buffers
        // read election.
        std::ifstream file("q6/voters.csv");
        if (!file.is_open()) {
            cerr << "Unable to open file: q6/voters.csv" << endl;
            exit(1);
        }

        std::string line;
        std::vector<cl_float3> voters;
        while (getline(file, line)) {
            std::stringstream ss(line);
            std::string token;

            getline(ss, token, ',');
            float x = std::stod(token);

            getline(ss, token, ',');
            float y = std::stod(token);

            getline(ss, token, ',');
            float z = std::stod(token);

            voters.push_back((cl_float3){x, y, z});
        }

        cl::Buffer buffer_voters = cl::Buffer(
            context,
            CL_MEM_READ_ONLY,
            voters.size() * sizeof(cl_float3)
        );

        cl::Buffer buffer_outcome = cl::Buffer(
            context,
            CL_MEM_WRITE_ONLY,
            voters.size() * sizeof(cl_uint2)
        );

 
        // Write buffers
        queue.enqueueWriteBuffer(
            buffer_voters,
            CL_TRUE,
            0,
            voters.size() * sizeof(cl_float3),
            voters.data()
        );

        // Set arguments to kernel
        kernel_elect.setArg(0, buffer_voters);
        kernel_elect.setArg(1, buffer_outcome);
        

        // Run the kernel on specific ND range
        cl::NDRange globalSize(100000);
        queue.enqueueNDRangeKernel(kernel_elect, cl::NullRange, globalSize); 
        
        // Read buffer(s)
        std::vector<cl_uint2> *outcome = new std::vector<cl_uint2>(voters.size());
        queue.enqueueReadBuffer(
            buffer_outcome,
            CL_TRUE,
            0,
            voters.size() * sizeof(cl_uint2),
            (*outcome).data()
        );

        int a_wins_count = 0;
        int b_wins_count = 0;
        int ties_count = 0;

        for (int i = 0; i < (*outcome).size(); i++) {
            if ((*outcome)[i].x > (*outcome)[i].y){
                a_wins_count ++;
            }
            else if ((*outcome)[i].x < (*outcome)[i].y){
                b_wins_count ++;
            } else {
                ties_count ++;
            }
        }

        // Placeholder floats; replace them if you wish.
        float a_wins = (float) (a_wins_count*100)/(a_wins_count + b_wins_count + ties_count);
        float b_wins = (float) (b_wins_count*100)/(a_wins_count + b_wins_count + ties_count);
        float ties = (float) (ties_count*100)/(a_wins_count + b_wins_count + ties_count);
        // Print results using print_results function
        print_results(a_wins, b_wins, ties);
        
        
    } catch(cl::Error error) {
        std::cout << error.what() << "(" << error.err() << ")" << std::endl;
    }

    return 0;
}
