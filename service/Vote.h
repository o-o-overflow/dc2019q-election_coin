#include <nlohmann/json.hpp>

#include "Exchange.h"

#ifndef DC2019Q_ELECTION_COIN_VOTE_H
#define DC2019Q_ELECTION_COIN_VOTE_H

using json = nlohmann::json;

class Vote {
public:
    std::string candidate_;
    std::string currency_;
    float amount_;
};

void to_json(json& value, const Vote& vote) {
    value = json{
            {"candidate", vote.candidate_},
            {"currency",  vote.currency_},
            {"amount",    vote.amount_}
    };
}

void from_json(const json& value, Vote& vote) {
    value.at("candidate").get_to(vote.candidate_);
    value.at("currency").get_to(vote.currency_);
    value.at("amount").get_to(vote.amount_);
}

#endif // DC2019Q_ELECTION_COIN_VOTE_H
