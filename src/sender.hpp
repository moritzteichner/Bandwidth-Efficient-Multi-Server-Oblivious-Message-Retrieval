#include <dpf.h>
#include "newTypes.hpp"

#include "server.hpp"

size_t LOG_NUM_USERS

void sendMessage(size_t receiver, int message){
    auto keys = DPF::Gen(receiver, LOG_NUM_USERS);

    MessageWithClue msg_S0(message, keys.first);
    MessageWithClue msg_S1(message, keys.second);

    // Here comes the QRPC Part to send these two shares to the two servers.
    receiveMessage(0, msg_S0);
    receiveMessage(1, msg_S1);
    return;
}