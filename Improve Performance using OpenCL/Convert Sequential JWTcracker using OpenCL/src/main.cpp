#define __CL_ENABLE_EXCEPTIONS

#include <CL/cl.hpp>

#include <string>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <math.h>  

#include <base64/base64.h>

using std::cout;
using std::cerr;
using std::endl;

//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------

int gMaxSecretLen = 4;

std::string gAlphabet = "abcdefghijklmnopqrstuvwxyz"
                        "0123456789";

//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------

void usage(const char *cmd) {
    cout << cmd << " <token> [maxLen] [alphabet]" << endl;
    cout << endl;

    cout << "Defaults:" << endl;
    cout << "maxLen = " << gMaxSecretLen << endl;
    cout << "alphabet = " << gAlphabet << endl;
}

int main(int argc, char const *argv[]) {

    if (argc < 2) {
        usage(argv[0]);
        return 1;
    }

    std::stringstream jwt;
    jwt << argv[1];

    if (argc > 2) {
        gMaxSecretLen = atoi(argv[2]);
    }
    if (argc > 3) {
        gAlphabet = argv[3];
    }

    std::string header64;
    getline(jwt, header64, '.');

    std::string payload64;
    getline(jwt, payload64, '.');

    std::string origSig64;
    getline(jwt, origSig64, '.');

    // Our goal is to find the secret to HMAC this string into our origSig
    std::string message = header64 + '.' + payload64;
    std::string origSig = base64_decode(origSig64);

    // Use OpenCL to brute force JWT
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
        std::ifstream sourceFile("src/jwtcracker.cl");
            if(!sourceFile.is_open()){
                std::cerr << "Cannot find kernel file" << std::endl;
                throw;
            }
        std::string sourceCode(std::istreambuf_iterator<char>(sourceFile), (std::istreambuf_iterator<char>()));
        cl::Program::Sources source(1, std::make_pair(sourceCode.c_str(), sourceCode.length() + 1));
 
        // Make program of the source code in the context
        cl::Program program = cl::Program(context, source);
 
        // Build program for these specific devices
        std::string v = "-D sigLen=" + std::to_string( origSig.size() ) + " -D gMaxSecretLen=" + std::to_string( gMaxSecretLen ) + " -D alphabetLenght=" + std::to_string( gAlphabet.size() ) + " -D messageLenght=" + std::to_string( message.size() );
        char variable[v.size()];
        strcpy(variable, v.c_str());
        try {
            program.build(devices, variable);
        } catch(cl::Error error) {
            std::cerr << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(devices[0]) << std::endl;
            throw;
        }
 
        // Make kernels
        cl::Kernel kernel(program, "bruteForceJWT");
 
        // Create buffers
        // to char array
        char *charAlphabet = new char[gAlphabet.size()+1];
        charAlphabet[gAlphabet.size()] = 0;
        memcpy(charAlphabet, gAlphabet.c_str(), gAlphabet.size());

        char *charMessage = new char[message.size()+1];
        charMessage[message.size()] = 0;
        memcpy(charMessage, message.c_str(), message.size());

        char *charOrigSig = new char[origSig.size()+1];
        charOrigSig[origSig.size()] = 0;
        memcpy(charOrigSig, origSig.c_str(), origSig.size());

        cl::Buffer buffer_gAlphabet = cl::Buffer(
            context,
            CL_MEM_READ_ONLY,
            strlen(charAlphabet) * sizeof(char)
        );
        cl::Buffer buffer_message = cl::Buffer(
            context,
            CL_MEM_READ_ONLY,
            strlen(charMessage) * sizeof(char)
        );
        cl::Buffer buffer_origSig = cl::Buffer(
            context,
            CL_MEM_READ_ONLY,
            strlen(charOrigSig) * sizeof(char)
        );
        cl::Buffer buffer_secret = cl::Buffer(
            context,
            CL_MEM_WRITE_ONLY,
            gMaxSecretLen + 1 * sizeof(char)
        );

        // Write buffers
        queue.enqueueWriteBuffer(
            buffer_gAlphabet,
            CL_TRUE,
            0,
            strlen(charAlphabet) * sizeof(char),
            charAlphabet
        );

        queue.enqueueWriteBuffer(
            buffer_message,
            CL_TRUE,
            0,
            strlen(charMessage) * sizeof(char),
            charMessage
        );

        queue.enqueueWriteBuffer(
            buffer_origSig,
            CL_TRUE,
            0,
            strlen(charOrigSig) * sizeof(char),
            charOrigSig
        );

        // Set arguments to kernel
        kernel.setArg(0, buffer_gAlphabet);
        kernel.setArg(1, buffer_message);
        kernel.setArg(2, buffer_origSig);
        kernel.setArg(3, buffer_secret);

        // Run the kernel on specific ND range
        cl::NDRange globalSize(pow(gAlphabet.size(), gMaxSecretLen));
        queue.enqueueNDRangeKernel(kernel, cl::NullRange, globalSize); 
 
        // Read buffer(s)
        char* secret = new char[gMaxSecretLen + 1]; 
        queue.enqueueReadBuffer(
            buffer_secret,
            CL_TRUE,
            0,
            gMaxSecretLen + 1 * sizeof(char), 
            secret
        );

        std::cout << std::string( secret ) << endl;

    } catch(cl::Error error) {
        std::cout << error.what() << "(" << error.err() << ")" << std::endl;
    }

    return 0;
}
