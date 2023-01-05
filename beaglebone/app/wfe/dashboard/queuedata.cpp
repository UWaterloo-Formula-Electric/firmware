#include <vector>
#include <string>
#include <utility>

class QueueData {
public:
    double speed;
    double temperature;
    double voltage;
    double battery;
    std::vector<std::pair<int, std::string>> dtcMessagePayload;

    QueueData() {

    }

    ~QueueData() {

    }

};
