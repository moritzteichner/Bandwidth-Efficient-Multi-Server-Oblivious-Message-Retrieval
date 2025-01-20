
//Method to generate a random list of users. Positions of the retrievig user in the simluation are random.
std::vector<int> generateClients(int k, int NUM_Messages, int LOG_NUM_USERS, int RETREIVING_USER) {

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(1, std::pow(2, LOG_NUM_USERS)-1);

    std::vector<int> random_numbers;

    for (int i = 0; i < k; ++i) {
        random_numbers.push_back(RETREIVING_USER);
    }

    //This ensures that no more than k messages are for this simluated recipient.
    for (int i = 0; i < NUM_Messages - k; ++i) {
        int random_value;
        do {
            random_value = dist(gen);
        } while (random_value == RETREIVING_USER);
        random_numbers.push_back(random_value);
    }

    std::shuffle(random_numbers.begin(), random_numbers.end(), gen);
    return random_numbers;
}