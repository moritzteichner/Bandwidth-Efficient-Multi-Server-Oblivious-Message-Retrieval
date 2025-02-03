#include <dpf.h>

class MessageWithClue {
private:
    mpz_class message;
    std::vector<uint8_t> clue;

public:
    // Constructor that takes both message and clue as parameters
    MessageWithClue(mpz_class msg, const std::vector<uint8_t>& cl) : message(msg), clue(cl) {}

    mpz_class getMessage() const {
        return message;
    }

    const std::vector<uint8_t>& getClue() const {
        return clue;
    }
};