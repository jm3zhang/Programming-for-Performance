#include "PackageDownloader.h"
#include "utils.h"
#include <fstream>
#include <cassert>

PackageDownloader::PackageDownloader(EventQueue* eq, int packageStartIdx, int packageEndIdx)
    : ChecksumTracker()
    , eq(eq)
    , numPackages(packageEndIdx - packageStartIdx + 1)
    , packageStartIdx(packageStartIdx)
    , packageEndIdx(packageEndIdx)
{
    #ifdef DEBUG
    std::unique_lock<std::mutex> stdoutLock(eq->stdoutMutex);
    printf("PackageDownloader n:%d s:%d e:%d\n", numPackages, packageStartIdx, packageEndIdx);
    #endif
}

PackageDownloader::~PackageDownloader() {
    // nop
}

void PackageDownloader::run() {

    Container<std::string> packages = readFileLines("data/packages.txt", packageStartIdx, packageEndIdx);
    for (int i = 0; i < packages.size(); i++) {
        std::string packageName = packages[i];

        eq->enqueueEvent(Event(Event::DOWNLOAD_COMPLETE, new Package(packageName)));
        uint8_t* sha256_packageName = sha256(packageName);
        updateGlobalChecksum(sha256_packageName);
        delete[] sha256_packageName;
    }
}
