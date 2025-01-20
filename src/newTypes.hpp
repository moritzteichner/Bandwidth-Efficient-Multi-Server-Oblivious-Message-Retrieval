#include <dpf.h>

class MessageWithClue {
private:
    int message;
    std::vector<uint8_t> clue;

public:
    // Constructor that takes both message and clue as parameters
    MessageWithClue(int msg, const std::vector<uint8_t>& cl) : message(msg), clue(cl) {}

    int getMessage() const {
        return message;
    }

    const std::vector<uint8_t>& getClue() const {
        return clue;
    }
};