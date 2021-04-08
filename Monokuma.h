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
enum DEBUGMENU {
    NONE = 1,
    DEBUG = 2,
    CAMERA = 4,
    FORCED_OPEN = 8
};
DEBUGMENU CurrentDebugMenu = NONE;

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

#include <windows.h>
struct PatchAddress { int Address; unsigned char OldByte; unsigned char NewByte; };
struct Patch { const char* ModuleName; std::vector<PatchAddress> Patches;};
void PatchUChar(unsigned char* dst, unsigned char* src, int size) {
    DWORD oldprotect;
    VirtualProtect(dst, size, PAGE_EXECUTE_READWRITE, &oldprotect);
    memcpy(dst, src, size);
    VirtualProtect(dst, size, oldprotect, &oldprotect); };
Patch dr1_us_exe = { "dr1_us.exe", {
        PatchAddress{ 126805, 87, 88 },
        PatchAddress{ 126885, 7, 8 },
        PatchAddress{ 126948, 200, 201 },
        PatchAddress{ 126965, 183, 184 },
        PatchAddress{ 126982, 166, 167 },
        PatchAddress{ 126999, 149, 150 },
        PatchAddress{ 127019, 129, 130 },
        PatchAddress{ 127036, 112, 113 },
        PatchAddress{ 127529, 131, 132 },
        PatchAddress{ 127575, 85, 86 },
        PatchAddress{ 127621, 39, 40 },
        PatchAddress{ 127667, 249, 250 },
        PatchAddress{ 127684, 232, 233 },
        PatchAddress{ 127701, 215, 216 },
        PatchAddress{ 127718, 198, 199 },
        PatchAddress{ 127738, 178, 179 },
        PatchAddress{ 127966, 206, 207 },
        PatchAddress{ 127983, 189, 190 },
        PatchAddress{ 164298, 226, 227 },
        PatchAddress{ 164483, 41, 42 },
        PatchAddress{ 164562, 218, 219 },
        PatchAddress{ 164678, 102, 103 },
        PatchAddress{ 164778, 2, 3 },
        PatchAddress{ 165474, 74, 75 },
        PatchAddress{ 165491, 57, 58 },
        PatchAddress{ 165508, 40, 41 },
        PatchAddress{ 165525, 23, 24 },
        PatchAddress{ 166207, 109, 110 },
        PatchAddress{ 166224, 92, 93 },
        PatchAddress{ 166241, 75, 76 },
        PatchAddress{ 166258, 58, 59 },
        PatchAddress{ 166411, 161, 162 },
        PatchAddress{ 166545, 27, 28 },
        PatchAddress{ 166626, 202, 203 },
        PatchAddress{ 166671, 157, 158 },
        PatchAddress{ 166709, 119, 120 },
        PatchAddress{ 166726, 102, 103 },
        PatchAddress{ 168406, 214, 215 },
        PatchAddress{ 168757, 119, 120 },
        PatchAddress{ 168805, 71, 72 },
        PatchAddress{ 168894, 238, 239 },
        PatchAddress{ 168911, 221, 222 },
        PatchAddress{ 168970, 162, 163 },
        PatchAddress{ 168990, 142, 143 },
        PatchAddress{ 174984, 36, 37 },
        PatchAddress{ 175062, 214, 215 },
        PatchAddress{ 175112, 164, 165 },
        PatchAddress{ 175151, 125, 126 },
        PatchAddress{ 175181, 95, 96 },
        PatchAddress{ 175211, 65, 66 },
        PatchAddress{ 175255, 21, 22 },
        PatchAddress{ 175302, 230, 231 },
        PatchAddress{ 175319, 213, 214 },
        PatchAddress{ 175336, 196, 197 },
        PatchAddress{ 175353, 179, 180 },
        PatchAddress{ 175410, 122, 123 },
        PatchAddress{ 175459, 73, 74 },
        PatchAddress{ 175476, 56, 57 },
        PatchAddress{ 175493, 39, 40 },
        PatchAddress{ 175510, 22, 23 },
        PatchAddress{ 175547, 241, 242 },
        PatchAddress{ 175584, 204, 205 },
        PatchAddress{ 175601, 187, 188 },
        PatchAddress{ 175618, 170, 171 },
        PatchAddress{ 175638, 150, 151 },
        PatchAddress{ 177927, 165, 166 },
        PatchAddress{ 178314, 34, 35 },
        PatchAddress{ 178362, 242, 243 },
        PatchAddress{ 178506, 98, 99 },
        PatchAddress{ 178526, 78, 79 },
        PatchAddress{ 178847, 13, 14 },
        PatchAddress{ 179013, 103, 104 },
        PatchAddress{ 179096, 20, 21 },
        PatchAddress{ 179179, 193, 194 },
        PatchAddress{ 179335, 37, 38 },
        PatchAddress{ 179507, 121, 122 },
        PatchAddress{ 179752, 132, 133 },
        PatchAddress{ 179769, 115, 116 },
        PatchAddress{ 180067, 73, 74 },
        PatchAddress{ 180084, 56, 57 },
        PatchAddress{ 180326, 70, 71 },
        PatchAddress{ 180404, 248, 249 },
        PatchAddress{ 180467, 185, 186 },
        PatchAddress{ 180503, 149, 150 },
        PatchAddress{ 180839, 69, 70 },
        PatchAddress{ 180930, 234, 235 },
        PatchAddress{ 180975, 189, 190 },
        PatchAddress{ 181004, 160, 161 },
        PatchAddress{ 181021, 143, 144 },
        PatchAddress{ 419685, 71, 72 },
        PatchAddress{ 419711, 45, 46 },
        PatchAddress{ 419757, 255, 0 },
        PatchAddress{ 419758, 205, 206 },
        PatchAddress{ 419803, 209, 210 },
        PatchAddress{ 419849, 163, 164 },
        PatchAddress{ 419895, 117, 118 },
        PatchAddress{ 423562, 34, 35 },
        PatchAddress{ 423588, 8, 9 },
        PatchAddress{ 423634, 218, 219 },
        PatchAddress{ 423680, 172, 173 },
        PatchAddress{ 423726, 126, 127 },
        PatchAddress{ 423772, 80, 81 },
        PatchAddress{ 423826, 26, 27 },
        PatchAddress{ 423880, 228, 229 },
        PatchAddress{ 423934, 174, 175 },
        PatchAddress{ 423988, 120, 121 },
        PatchAddress{ 425773, 127, 128 },
        PatchAddress{ 425799, 101, 102 },
        PatchAddress{ 425845, 55, 56 },
        PatchAddress{ 425891, 9, 10 },
        PatchAddress{ 425937, 219, 220 },
        PatchAddress{ 425983, 173, 174 },
        PatchAddress{ 426029, 127, 128 },
        PatchAddress{ 426075, 81, 82 },
        PatchAddress{ 426121, 35, 36 },
        PatchAddress{ 426167, 245, 246 },
        PatchAddress{ 426213, 199, 200 },
        PatchAddress{ 426259, 153, 154 },
        PatchAddress{ 426305, 107, 108 },
        PatchAddress{ 426351, 61, 62 },
        PatchAddress{ 426400, 12, 13 },
        PatchAddress{ 426449, 219, 220 },
        PatchAddress{ 426498, 170, 171 },
        PatchAddress{ 426547, 121, 122 },
        PatchAddress{ 427830, 118, 119 },
        PatchAddress{ 427856, 92, 93 },
        PatchAddress{ 427902, 46, 47 },
        PatchAddress{ 427948, 0, 1 },
        PatchAddress{ 427994, 210, 211 },
        PatchAddress{ 428040, 164, 165 },
        PatchAddress{ 428086, 118, 119 },
        PatchAddress{ 428132, 72, 73 },
        PatchAddress{ 428178, 26, 27 },
        PatchAddress{ 428224, 236, 237 },
        PatchAddress{ 428270, 190, 191 },
        PatchAddress{ 428316, 144, 145 },
        PatchAddress{ 428362, 98, 99 },
        PatchAddress{ 439294, 174, 175 },
        PatchAddress{ 439320, 148, 149 },
        PatchAddress{ 439366, 102, 103 },
        PatchAddress{ 439412, 56, 57 },
        PatchAddress{ 439458, 10, 11 },
} };
void patch_dr1_us_exe() {
    for (PatchAddress addr : dr1_us_exe.Patches) {
        PatchUChar((unsigned char*)(BaseAddress + addr.Address), &addr.NewByte, 1); }
};
void unpatch_dr1_us_exe() {
    for (PatchAddress addr : dr1_us_exe.Patches) {
        PatchUChar((unsigned char*)(BaseAddress + addr.Address), &addr.OldByte, 1); }
};

#endif //MONOKUMAHOOK_MONOKUMA_H
