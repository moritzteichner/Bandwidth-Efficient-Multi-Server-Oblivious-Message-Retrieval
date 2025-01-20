#include "dpf.h"
#include "newTypes.hpp"

#include <vector>

std::vector<MessageWithClue> server0;
std::vector<MessageWithClue> server1;

void receiveMessage(int server, MessageWithClue msg){
    if (server == 0) {
        server0.push_back(msg);
    } else {
        server1.push_back(msg);
    }
    return;
}



