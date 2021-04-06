#ifndef MONOKUMAHOOK_MONOKUMA_H
#define MONOKUMAHOOK_MONOKUMA_H

#include <vector>
#include <mutex>

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
            printf("Dropped a print command! Buffer:%i pCmdVt:%i\n",lockBuffer.size() ,pCommandVector.size());
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
#endif //MONOKUMAHOOK_MONOKUMA_H
