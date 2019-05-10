#include <string>
#include <unordered_map>

#include "Vote.h"
#include "Exchange.h"

#ifndef DC2019Q_ELECTION_COIN_BALLOT_H
#define DC2019Q_ELECTION_COIN_BALLOT_H

class Ballot {
public:
    void convertVotes(const std::vector<std::shared_ptr<Exchange>>& exchanges) {
        for (auto& it : votes_) {
            it.second.convertCurrency(exchanges);
        }
    }

    std::string voter_;
    std::string election_;
    std::unordered_map<std::string, Vote> votes_;
};

void to_json(json& value, const Ballot& ballot) {
    value = json{
            {"voter",    ballot.voter_},
            {"election", ballot.election_},
            {"votes",    ballot.votes_}
    };
}

void from_json(const json& value, Ballot& ballot) {
    value.at("voter").get_to(ballot.voter_);
    value.at("election").get_to(ballot.election_);
    value.at("votes").get_to(ballot.votes_);
}

#endif // DC2019Q_ELECTION_COIN_BALLOT_H
