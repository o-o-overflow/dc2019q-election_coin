#ifndef DC2019Q_ELECTION_COIN_BALLOT_H
#define DC2019Q_ELECTION_COIN_BALLOT_H

#include <string>
#include <unordered_map>

#include "Vote.h"
#include "Exchange.h"

class Ballot {
public:
    std::vector<std::string> convertVotes(std::vector<std::shared_ptr<Exchange>>& exchanges) {
        std::vector<std::string> notes;

        for (auto& v_it : votes_) {
            std::size_t index = 0;
            auto& vote = v_it.second;
            if (vote.currency_ == "bitcoin") {
                index = 1;
            } else if (vote.currency_ == "dogecoin") {
                index = 2;
            } else if (vote.currency_ == "ethereum") {
                index = 3;
            }

            if (index) {
                auto note = exchanges[index]->convertCurrency(voter_, vote.amount_);
                notes.emplace_back(note);
            }
        }

        return notes;
    }

    std::string voter_;
    std::unordered_map<std::string, Vote> votes_;
};

void to_json(json& value, const Ballot& ballot) {
    value = json{
            {"voter", ballot.voter_},
            {"votes", ballot.votes_}
    };
}

void from_json(const json& value, Ballot& ballot) {
    value.at("voter").get_to(ballot.voter_);
    value.at("votes").get_to(ballot.votes_);
}

#endif // DC2019Q_ELECTION_COIN_BALLOT_H
