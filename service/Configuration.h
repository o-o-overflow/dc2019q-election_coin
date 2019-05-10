#include <utility>
#include <vector>

#include "Election.h"

#ifndef DC2019Q_ELECTION_COIN_CONFIGURATION_H
#define DC2019Q_ELECTION_COIN_CONFIGURATION_H

class Configuration {
public:
    std::vector<Election> elections_;
};

void to_json(json& value, const Configuration& config) {
    value = json{{"elections", config.elections_}};
}

void from_json(const json& value, Configuration& config) {
    value.at("elections").get_to(config.elections_);
}

#endif // DC2019Q_ELECTION_COIN_CONFIGURATION_H
