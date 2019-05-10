#include <string>

#include <nlohmann/json.hpp>

#ifndef DC2019Q_ELECTION_COIN_CANDIDATE_H
#define DC2019Q_ELECTION_COIN_CANDIDATE_H

using json = nlohmann::json;

class Candidate {
public:
    void postVote(const Vote& vote) {
        if (name_ != vote.candidate_) {
            throw std::runtime_error("unknown candidate");
        }

        tally_ += vote.amount_;
    }

    std::string name_;
    float tally_;
};

void to_json(json& value, const Candidate& candidate) {
    value = json{
            {"name",  candidate.name_},
            {"tally", candidate.tally_},
    };
}

void from_json(const json& value, Candidate& candidate) {
    value.at("name").get_to(candidate.name_);
    value.at("tally").get_to(candidate.tally_);
}

#endif // DC2019Q_ELECTION_COIN_CANDIDATE_H
