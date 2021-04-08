#ifndef MONOKUMAHOOK_MONOKUMA_H
#define MONOKUMAHOOK_MONOKUMA_H

#include <vector>
#include <mutex>
#include <windows.h>

struct ScreenPrintCommand {
    int xPos;
    int yPos;
    std::string text;
};
class ScreenPrintCommandBuffer {

private:
    std::vector<ScreenPrintCommand> pCommandVector;
    std::vector<ScreenPrintCommand> lockBuffer;
    std::mutex lock;

public:
    void push(ScreenPrintCommand screenPrintCommand){
        if (lock.try_lock()) {
            pCommandVector.push_back(screenPrintCommand);
            /* pCommandVector.insert(
                    pCommandVector.end(),
                    lockBuffer.begin(),
                    lockBuffer.end()
                    );
            */
            lockBuffer.clear();
            lock.unlock();
        }
        else {
            // drop command
            lockBuffer.push_back(screenPrintCommand);
            printf("[Monokuma] Dropped a print command! Buffer:%i pCmdVt:%i\n",lockBuffer.size() ,pCommandVector.size());
        }
    }

    std::vector<ScreenPrintCommand> pull() {
        lock.lock();
        std::vector<ScreenPrintCommand> retval = pCommandVector;
        pCommandVector.clear();
        lock.unlock();
        return retval;
    }
};
enum CAMERASTATE {
    LOCKED,
    UNLOCKED
};

int BaseAddress = (int) GetModuleHandle(nullptr);
int ExecutableBase = 0x30000;

// Thanks [https://stackoverflow.com/a/48737037]
void Patch(int* dst, int* src, int size)
{
    DWORD oldprotect;
    VirtualProtect(dst, size, PAGE_EXECUTE_READWRITE, &oldprotect);
    memcpy(dst, src, size);
    VirtualProtect(dst, size, oldprotect, &oldprotect);
}

void SetCameraState(CAMERASTATE state) {
    // Camera Lock Bytes
    char* cameraByte1 = (char*)((BaseAddress + 0x9AD02) - ExecutableBase); // should be 0x00 in debug mode - code
    char* cameraByte2 = (char*)((BaseAddress + 0x9AD0C) - ExecutableBase); // should be 0x01 in debug mode - code
    int* cameraByteA  = (int*)((BaseAddress + 0x36CC60) - ExecutableBase); // Should toggle after the camera bytes.
    int* cameraByteB  = (int*)((BaseAddress + 0x36CC68) - ExecutableBase); // Should toggle after the camera bytes.

    int one = 1;
    int zero = 0;

    switch (state) {
        case UNLOCKED:
            Patch((int *) cameraByte1, &one, 1);
            Patch((int *) cameraByte2, &zero, 1);
            break;
        case LOCKED:
            Patch((int *) cameraByte1, &zero, 1);
            Patch((int *) cameraByte2, &one, 1);
            break;
    };

    *cameraByteA = *cameraByteA ^ 0x00000001;
    *cameraByteB = *cameraByteB ^ 0x00000001;
}

#endif //MONOKUMAHOOK_MONOKUMA_H
