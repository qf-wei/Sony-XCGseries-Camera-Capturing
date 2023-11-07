#include <iostream>
#include <string>
#include <memory>
#include <fcntl.h>
#include <io.h>
#include "SonyCam.h"

std::unique_ptr<CSonyCam> pSonyCam;
PBITMAPINFO pBitInfo;
std::unique_ptr<BYTE[]> pRGBData;
UINT64 FileCount = 0;

std::wstring shmName;

// Shared memory size
const size_t SHM_SIZE = 1024 * 768 * 4;

HANDLE hMapFile;
LPVOID pBuf;

void InitializeCamera();
void FinalizeCamera();
void WriteToSharedMemory(BYTE* data, size_t dataSize);
void InitializeSharedMemory(const std::wstring& name);


int main() {
    std::wstring inputName;
    std::wcout << L"Enter the shared memory name: ";
    std::getline(std::wcin, inputName);

    //the memory address should match the one in the python script
    shmName = inputName.empty() ? L"Local\\Cam1Mem" : inputName;
    InitializeSharedMemory(shmName);
    InitializeCamera();
    std::string command;
    while (std::getline(std::cin, command)) {
        if (command == "capture") {
            BYTE* imageData = new BYTE[SHM_SIZE]; 
            // Assuming image data will fit into shared memory
            auto imageSize = pBitInfo->bmiHeader.biSizeImage;
            if (pSonyCam->Capture(imageData)) {
                WriteToSharedMemory(imageData, imageSize);
            }
            else {
                std::cerr << "Failed to capture image." << std::endl;
            }

            delete[] imageData;
        }
        else if (command == "finalize") {
            FinalizeCamera();
            break;
        }
        else {
            std::cerr << "Unknown command." << std::endl;
        }
    }

    UnmapViewOfFile(pBuf);
    CloseHandle(hMapFile);
    return 0;
}


void InitializeCamera() {
    pSonyCam = std::make_unique<CSonyCam>();
    pSonyCam->SetMaxPacketSize();
    pSonyCam->SetFeature("AcquisitionMode", "Continuous");
    pSonyCam->StreamStart();
    pBitInfo = pSonyCam->GetBMPINFO();
    pRGBData = std::make_unique<BYTE[]>(pBitInfo->bmiHeader.biSizeImage);

    std::string serialNum = pSonyCam->GetSerialNumber();
    std::cerr << serialNum << std::endl;
}

void FinalizeCamera() {
    pSonyCam->StreamStop();
}


void InitializeSharedMemory(const std::wstring& name) {
    hMapFile = CreateFileMapping(
        INVALID_HANDLE_VALUE,
        NULL,
        PAGE_READWRITE,
        0,
        SHM_SIZE,
        name.c_str() 
    );

    if (hMapFile == NULL) {
        std::cerr << "Could not create file mapping object: " << GetLastError() << std::endl;
        exit(1);
    }

    pBuf = MapViewOfFile(
        hMapFile,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        SHM_SIZE
    );

    if (pBuf == NULL) {
        CloseHandle(hMapFile);
        std::cerr << "Could not map view of file: " << GetLastError() << std::endl;
        exit(1);
    }
}

void WriteToSharedMemory(BYTE* data, size_t dataSize) {
    memcpy(pBuf, data, dataSize);
    std::cerr << "Done!" << std::endl;

}
