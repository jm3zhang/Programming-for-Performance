#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <cmath>
#include <cassert>
#include <cstring>
#include <string>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <vector>
#include <omp.h>

#include <base64/base64.h>

using std::cout;
using std::cerr;
using std::endl;
using namespace std;

//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------

#define HASH_BITS    256
#define HASH_BYTES   (HASH_BITS / 8)

int gMaxSecretLen = 4;

std::string gAlphabet = "abcdefghijklmnopqrstuvwxyz"
                        "0123456789";

//-----------------------------------------------------------------------------
// Helper
//-----------------------------------------------------------------------------

bool isValidSecret(const std::string &message, const std::string &origSig, const std::string &secret) {
    uint8_t sigBuffer[EVP_MAX_MD_SIZE];
    uint32_t sigBufferLen;

    HMAC(
        (EVP_MD *)EVP_sha256(),
        (const unsigned char*)secret.c_str(), secret.size(),
        (const unsigned char*)message.c_str(), message.size(),
        sigBuffer, &(sigBufferLen)
    );

    assert(origSig.size() == HASH_BYTES);
    assert(sigBufferLen == HASH_BYTES);

    return memcmp(sigBuffer, origSig.c_str(), HASH_BYTES) == 0;
}

int done = 0;

void findPrintCombAndValidate(char alphabet[], 
                                string subString, 
                                int alphabetLength, 
                                string message, 
                                string origSig) { 
    
    if (done == 1){
        return;
    }

    if (isValidSecret(message, origSig, subString)) {
        cout << subString << endl;
        done = 1;
        return;
    }

    if (subString.size() == alphabetLength) {
        return;
    }
    
    #pragma omp parallel for
    for (int i = 0; i < strlen(alphabet); i++) { 
        string newString; 
        newString = subString + alphabet[i]; 
        // cout << newString << endl;
        findPrintCombAndValidate(alphabet, newString, alphabetLength, message, origSig); 
    } 
} 

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

    // *****************************************************
    // ** Your job is to brute force all possible secrets **
    // *****************************************************

    // std::string secret = "Hello there";

    char alphabet[gAlphabet.length() + 1]; 
    strcpy(alphabet, gAlphabet.c_str()); 
    // cout << alphabet << endl;
    findPrintCombAndValidate(alphabet, "", gMaxSecretLen, message, origSig);

    return 0;
}
