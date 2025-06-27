#ifndef _ACTUARY_ACCOUNT_CONTAINER_H_
#define _ACTUARY_ACCOUNT_CONTAINER_H_

#include <cstdint>
#include <iostream>
#include <vector>
#include <mutex>
#include <cmath>
#include <shared_mutex>
#include <unordered_map>
#include <string>
#include <optional>
#include "binancecpp/binance_futures.h"
#include "binancecpp/binance_ws_model.h"
#include "binancecpp/util/string_helper.h"

using namespace std;

namespace actuary {
    
    struct AccountMetaInfo {
        bool multiAssetsMargin;
        double totalInitialMargin = 0;
        double totalMaintMargin = 0;
        double totalWalletBalance = 0;
        double totalUnrealizedProfit = 0;
        double totalCrossWalletBalance = 0;
        double totalCrossUnPnl = 0;
        uint64_t updateTimeMillis = 0;
    };

    struct AccountBalanceInfo {
        string asset;
        double walletBalance = 0;
        double crossWalletBalance = 0;
        double crossUnPnl = 0;
        uint64_t updateTimeMillis = 0;
    };
    
    struct AccountPositionInfo {
        string symbol;
        double initialMargin = 0;
        double maintMargin = 0;
        double unrealizedProfit = 0;
        double entryPrice = 0;
        string positionSide;
        double positionAmount = 0;
        double positionAmountAbs = 0;
        uint64_t updateTimeMillis = 0;
    };

    struct PositionThresholdInfo {
        string symbol;
        string positionSide;
        double positionReduceRatio = 0;
        double totalNotional = 0;
        bool reachMaxPosition = false;
        uint64_t updateTimeMillis = 0;
    };

    struct AccountBalanceInfoWrapper {
        unordered_map<string, AccountBalanceInfo> balance_map;
        shared_mutex rw_lock;
    };

    struct AccountPositionInfoWrapper {
        unordered_map<string, AccountPositionInfo> position_map;
        shared_mutex rw_lock;
    };

    struct PositionThresholdInfoWrapper {
        unordered_map<string, PositionThresholdInfo> threshold_map;
        shared_mutex rw_lock;
    };

    class AccountBalancePositionComposite {
    public:
        AccountBalancePositionComposite() {}
        ~AccountBalancePositionComposite() {}
    
    private:
        AccountMetaInfo meta;
        AccountBalanceInfoWrapper balanceWrapper;
        AccountPositionInfoWrapper positionWrapper;
        PositionThresholdInfoWrapper positionThresholdWrapper;
        shared_mutex rw_lock;
    
    public:
        void init(vector<string>& assets, vector<string>& inst_ids);
        bool update_meta(binance::FuturesAccount& meta);
        bool update_exist_balance(binance::FuturesAccountAsset& balance);
        bool update_exist_position(binance::FuturesAccountPosition& position);
        bool update_exist_position_threshold(PositionThresholdInfo& threshold);
        bool update_exist_balance_event(binance::WsFuturesAccountUpdateBalanceEvent& event);
        bool update_exist_position_event(binance::WsFuturesAccountUpdatePositionEvent& position);
        AccountMetaInfo copy_meta();
        optional<AccountBalanceInfo> copy_balance(string asset);
        optional<AccountPositionInfo> copy_position(string inst_id);
        optional<PositionThresholdInfo> copy_position_threshold(string inst_id);
    };
}

#endif