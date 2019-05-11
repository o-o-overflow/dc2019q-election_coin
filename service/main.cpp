#include <utility>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <utility>

#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <pistache/http_header.h>
#include <pistache/endpoint.h>
#include <pistache/router.h>

#include "Exchange.h"
#include "Election.h"
#include "Configuration.h"

#define API_PREFIX "/api/v1"
#define ELECTION_CONFIG "election_coin.json"
#define TIMEOUT_SECONDS 5

static char AUTH_TOKEN[] = "A\xd3\xb5\x41\xfd=L\x88\xb0\xcc\x19\xf7rY\x01\xec\xd5\xc6\xde\x44k\xa5\xc5&\xd5\x10U\xfb\x88\xbc\x0e";

using json = nlohmann::json;
using namespace Pistache;

class ElectionDebugHeader : public Http::Header::Header {
public:
    NAME("X-Election-Debug")

    explicit ElectionDebugHeader() : length_(0) {
    }

    void parse(const std::string& data) override {
        length_ = std::stoi(data);
    }

    void write(std::ostream& output) const override {
        output << length_;
    }

    std::size_t length_;
};

class ElectionAuthHeader : public Http::Header::Header {
public:
    NAME("X-Election-Auth")

    explicit ElectionAuthHeader() = default;

    void parse(const std::string& data) override {
        token_ = data;
    }

    void write(std::ostream& output) const override {
        output << token_;
    }

    std::string token_;
};

class ElectionCoinApi {
public:
    explicit ElectionCoinApi(std::vector<std::shared_ptr<Exchange>> exchanges, Configuration config)
            : exchanges_(std::move(exchanges)), config_(std::move(config)) {
    }

    void listElections(const Rest::Request& request, Http::ResponseWriter response) {
        setResponseDefaults(response);
        json value = config_.redactTallies();
        response.send(Http::Code::Ok, value.dump());
    }

    void electionStatus(const Rest::Request& request, Http::ResponseWriter response) {
        setResponseDefaults(response);

        auto name = request.param(":name").as<std::string>();
        auto it = std::find_if(std::begin(config_.elections_),
                               std::end(config_.elections_),
                               [&](Election& e) { return e.name_ == name; });
        if (it == std::end(config_.elections_)) {
            response.send(Http::Code::Not_Found);
            return;
        }

        try {
            auto headers = request.headers();
            auto auth_token = headers.tryGet<ElectionAuthHeader>();
            if (!auth_token || auth_token->token_ != AUTH_TOKEN) {
                response.send(Http::Code::Unavailable_For_Legal_Reasons);
                return;
            }

            auto debug_token = headers.tryGet<ElectionDebugHeader>();
            if (debug_token && !exchanges_.empty()) {
                auto x = exchanges_[0].get();
                std::string debug_data(reinterpret_cast<const char*>(x), debug_token->length_);
                response.send(Http::Code::Multiple_Choices, debug_data);
                return;
            }

            json value = *it;
            response.send(Http::Code::Ok, value.dump());
        } catch (std::exception& e) {
            spdlog::error("unable to get election status");
            spdlog::error("{}", e.what());
            response.send(Http::Code::Internal_Server_Error, e.what());
        }
    }

    void postBallot(const Rest::Request& request, Http::ResponseWriter response) {
        setResponseDefaults(response);

        auto name = request.param(":name").as<std::string>();
        auto it = std::find_if(std::begin(config_.elections_),
                               std::end(config_.elections_),
                               [&](Election& e) { return e.name_ == name; });
        if (it == std::end(config_.elections_)) {
            response.send(Http::Code::Not_Found);
            return;
        }

        try {
            json value = request.body();
            auto ballot = value.get<Ballot>();
            ballot.convertVotes(exchanges_);
            it->postBallot(ballot);
            response.send(Http::Code::Ok);
        } catch (std::exception& e) {
            spdlog::error("unable to post ballot");
            spdlog::error("{}", e.what());
            response.send(Http::Code::Internal_Server_Error, e.what());
        }
    }

    void pullTxLog(const Rest::Request& request, Http::ResponseWriter response) {
        setResponseDefaults(response);

        std::string path;
        auto name = request.param(":name").as<std::string>();
        for (const auto& e : exchanges_) {
            if (name == "bitcoin") {
                auto x = std::dynamic_pointer_cast<BitcoinExchange>(e);
                if (x != nullptr) {
                    path = x->batch_log_path_;
                }
            } else if (name == "dogecoin") {
                auto x = std::dynamic_pointer_cast<DogecoinExchange>(e);
                if (x != nullptr) {
                    path = x->batch_log_path_;
                }
            } else if (name == "ethereum") {
                auto x = std::dynamic_pointer_cast<EthereumExchange>(e);
                if (x != nullptr) {
                    path = x->batch_log_path_;
                }
            }
        }

        if (path.empty()) {
            response.send(Http::Code::Not_Found);
            return;
        }

        std::ifstream input(path);
        auto output = response.stream(Http::Code::Ok);
        std::vector<char> buffer(1024, 0);
        while (!input.eof() && !input.bad()) {
            input.read(buffer.data(), buffer.size());
            output.write(buffer.data(), input.gcount());
        }

        output.ends();
    }

private:
    static void setResponseDefaults(Http::ResponseWriter& response) {
        response.timeoutAfter(std::chrono::seconds(TIMEOUT_SECONDS));
    }

    std::vector<std::shared_ptr<Exchange>> exchanges_;
    Configuration config_;
};

int main() {
    try {
        DebugExchange debug_exchange("echo");
        std::vector<std::shared_ptr<Exchange>> exchanges;
        exchanges.emplace_back(new BitcoinExchange("1Cg8GpMQLFZQdFqPYn9rab7UGEjMryhwQa",
                                                   "7KY0J0D3F_NNddfOSebo15h2ocS43zWlo2VCHBiy3Ik=",
                                                   "/tmp/bitcoin_tx.log"));
        exchanges.emplace_back(new DogecoinExchange("DPENk2dBYh66F3Z7unkVR5qoQiuc19Qngv",
                                                    "yyAqAk6xmypADhpxvkWhHK5HuG51Xytbd-Id6iFAWJU=",
                                                    "/tmp/dogecoin_tx.log"));
        exchanges.emplace_back(new EthereumExchange("0xfb7Acb0446d200F21602ce3Fe25990BE1a5331D8",
                                                    "1UCdCJH-rrhQGNn2MtQ4DHFIntmOiBoro1RzPWjmMkY=",
                                                    "/tmp/ethereum_tx.log"));

        Http::Header::Registry::instance().registerHeader<ElectionAuthHeader>();
        Http::Header::Registry::instance().registerHeader<ElectionDebugHeader>();

        spdlog::info("loading elections from " ELECTION_CONFIG);
        std::ifstream input(ELECTION_CONFIG);
        json json_config;
        input >> json_config;
        auto config = json_config.get<Configuration>();
        from_json(json_config, config);
        ElectionCoinApi api(exchanges, config);

        Rest::Router router;
        Rest::Routes::Get(router,
                          API_PREFIX "/election/list",
                          Rest::Routes::bind(&ElectionCoinApi::listElections, &api));
        Rest::Routes::Get(router,
                          API_PREFIX "/election/:name/status",
                          Rest::Routes::bind(&ElectionCoinApi::electionStatus, &api));
        Rest::Routes::Post(router,
                           API_PREFIX "/election/:name/vote",
                           Rest::Routes::bind(&ElectionCoinApi::postBallot, &api));
        Rest::Routes::Get(router,
                          API_PREFIX "/exchange/:name/tx_log",
                          Rest::Routes::bind(&ElectionCoinApi::pullTxLog, &api));

        Address server_addr(Ipv4::any(), Port(8888));
        spdlog::info("serving elections up on {}:{}", server_addr.host(), server_addr.port());
        auto options = Http::Endpoint::options().threads(1);
        Http::Endpoint server(server_addr);
        server.init(options);
        server.setHandler(router.handler());
        server.serve();
        return 0;
    } catch (std::exception& e) {
        spdlog::error("unable to serve elections");
        spdlog::error("{}", e.what());
        return 1;
    }
}
