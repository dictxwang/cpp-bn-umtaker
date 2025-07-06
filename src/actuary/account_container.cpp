#include "account_container.h";

using namespace std;

namespace actuary {
    void AccountBalancePositionComposite::init(vector<string>& assets, vector<string>& inst_ids) {
        for (string asset : assets) {
            AccountBalanceInfo balance;
            this->balanceWrapper.balance_map.insert({asset, balance});
        }
        for (string inst_id : inst_ids) {
            AccountPositionInfo position;
            this->positionWrapper.position_map.insert({inst_id, position});
        }

        for (string inst_id : inst_ids) {
            PositionThresholdInfo threshold;
            this->positionThresholdWrapper.threshold_map.insert({inst_id, threshold});
        }
    }

    bool AccountBalancePositionComposite::update_meta(binance::FuturesAccount& meta) {
        std::unique_lock<std::shared_mutex> w_lock(rw_lock);
        this->meta.multiAssetsMargin = meta.multiAssetsMargin;
        this->meta.totalInitialMargin = meta.totalInitialMargin;
        this->meta.totalMaintMargin = meta.totalMaintMargin;
        this->meta.totalWalletBalance = meta.totalWalletBalance;
        this->meta.totalUnrealizedProfit = meta.totalUnrealizedProfit;
        this->meta.totalCrossWalletBalance = meta.totalCrossWalletBalance;
        this->meta.totalCrossUnPnl = meta.totalCrossUnPnl;
        this->meta.updateTimeMillis = binance::get_current_ms_epoch();
        w_lock.unlock();
        return true;
    }
    
    bool AccountBalancePositionComposite::update_exist_balance(binance::FuturesAccountAsset& balance) {
        
        std::shared_lock<std::shared_mutex> r_lock(this->balanceWrapper.rw_lock);
        auto original = this->balanceWrapper.balance_map.find(balance.asset);
        r_lock.unlock();

        if (original != this->balanceWrapper.balance_map.end()) {
            AccountBalanceInfo item;
            item.asset = balance.asset;
            item.walletBalance = balance.walletBalance;
            item.crossWalletBalance = balance.crossWalletBalance;
            item.crossUnPnl = balance.crossUnPnl;
            item.updateTimeMillis = binance::get_current_ms_epoch();

            std::unique_lock<std::shared_mutex> w_lock(this->balanceWrapper.rw_lock);
            this->balanceWrapper.balance_map[item.asset] = item;
            w_lock.unlock();
            
            return true;
        } else {
            return false;
        }

    }

    bool AccountBalancePositionComposite::update_exist_position(binance::FuturesAccountPosition& position) {

        std::shared_lock<std::shared_mutex> r_lock(this->positionWrapper.rw_lock);
        auto original = this->positionWrapper.position_map.find(position.symbol);
        r_lock.unlock();

        if (original != this->positionWrapper.position_map.end()) {
            AccountPositionInfo item;
            item.symbol = position.symbol;
            item.initialMargin = position.initialMargin;
            item.maintMargin = position.maintMargin;
            item.unrealizedProfit = position.unrealizedProfit;
            item.entryPrice = position.entryPrice;
            item.positionSide = position.positionSide;
            if (position.positionSide == binance::PositionSide_BOTH) {
                if (position.positionAmt < 0) {
                    item.positionSide = binance::PositionSide_SHORT;
                } else if (position.positionAmt > 0) {
                    item.positionSide = binance::PositionSide_LONG;
                }
            }
            item.positionAmount = position.positionAmt;
            item.positionAmountAbs = std::abs(position.positionAmt);
            item.updateTimeMillis = position.updateTime;

            std::unique_lock<std::shared_mutex> w_lock(this->positionWrapper.rw_lock);
            this->positionWrapper.position_map[item.symbol] = item;
            w_lock.unlock();

            return true;
        } else {
            return false;
        }
    }

    bool AccountBalancePositionComposite::update_exist_position_threshold(PositionThresholdInfo& threshold) {
        
        std::shared_lock<std::shared_mutex> r_lock(this->positionThresholdWrapper.rw_lock);
        auto original = this->positionThresholdWrapper.threshold_map.find(threshold.symbol);
        r_lock.unlock();

        if (original != this->positionThresholdWrapper.threshold_map.end()) {
            PositionThresholdInfo item;
            item.symbol = threshold.symbol;
            item.positionSide = threshold.positionSide;
            item.positionReduceRatio = threshold.positionReduceRatio;
            item.totalNotional = threshold.totalNotional;
            item.updateTimeMillis = threshold.updateTimeMillis;

            std::unique_lock<std::shared_mutex> w_lock(this->positionThresholdWrapper.rw_lock);
            this->positionThresholdWrapper.threshold_map[item.symbol] = item;
            w_lock.unlock();

            return true;
        } else {
            return false;
        }
    }

    bool AccountBalancePositionComposite::update_exist_balance_event(binance::WsFuturesAccountUpdateBalanceEvent& event) {

        std::shared_lock<std::shared_mutex> r_lock(this->balanceWrapper.rw_lock);
        auto original = this->balanceWrapper.balance_map.find(event.asset);
        r_lock.unlock();

        if (original != this->balanceWrapper.balance_map.end()) {
            AccountBalanceInfo item;
            item.asset = event.asset;
            item.walletBalance = event.walletBalance;
            item.crossWalletBalance = event.crossWalletBalance;
            item.updateTimeMillis = binance::get_current_ms_epoch();

            std::unique_lock<std::shared_mutex> w_lock(this->balanceWrapper.rw_lock);
            this->balanceWrapper.balance_map[item.asset] = item;
            w_lock.unlock();

            return true;
        } else {
            return false;
        }
    }

    bool AccountBalancePositionComposite::update_exist_position_event(binance::WsFuturesAccountUpdatePositionEvent& event) {

        std::shared_lock<std::shared_mutex> r_lock(this->positionWrapper.rw_lock);
        auto original = this->positionWrapper.position_map.find(event.symbol);
        r_lock.unlock();

        if (original != this->positionWrapper.position_map.end()) {
            AccountPositionInfo item;
            item.symbol = event.symbol;
            item.unrealizedProfit = event.unrealizedPnL;
            item.entryPrice = event.entryPrice;
            item.positionSide = event.positionSide;
            if (event.positionSide == binance::PositionSide_BOTH) {
                if (event.positionAmout < 0) {
                    item.positionSide = binance::PositionSide_SHORT;
                } else if (event.positionAmout > 0) {
                    item.positionSide = binance::PositionSide_LONG;
                }
            }
            item.positionAmount = event.positionAmout;
            item.positionAmountAbs = std::abs(event.positionAmout);
            item.updateTimeMillis = binance::get_current_ms_epoch();

            std::unique_lock<std::shared_mutex> w_lock(this->positionWrapper.rw_lock);
            this->positionWrapper.position_map[item.symbol] = item;
            w_lock.unlock();

            return true;
        } else {
            return false;
        }
    }

    AccountMetaInfo AccountBalancePositionComposite::copy_meta() {
        std::shared_lock<std::shared_mutex> lock(rw_lock);
        AccountMetaInfo info;
        info.multiAssetsMargin = this->meta.multiAssetsMargin;
        info.totalInitialMargin = this->meta.totalInitialMargin;
        info.totalMaintMargin = this->meta.totalMaintMargin;
        info.totalWalletBalance = this->meta.totalWalletBalance;
        info.totalUnrealizedProfit = this->meta.totalUnrealizedProfit;
        info.totalCrossWalletBalance = this->meta.totalCrossWalletBalance;
        info.totalCrossUnPnl = this->meta.totalCrossUnPnl;
        info.updateTimeMillis = this->meta.updateTimeMillis;
        return info;
    }

    optional<AccountBalanceInfo> AccountBalancePositionComposite::copy_balance(string asset) {
        std::shared_lock<std::shared_mutex> lock(this->balanceWrapper.rw_lock);
        auto item = this->balanceWrapper.balance_map.find(asset);
        if (item != this->balanceWrapper.balance_map.end()) {
            AccountBalanceInfo info;
            info.asset = (*item).second.asset;
            info.walletBalance = (*item).second.walletBalance;
            info.crossWalletBalance = (*item).second.crossWalletBalance;
            info.crossUnPnl = (*item).second.crossUnPnl;
            info.updateTimeMillis = (*item).second.updateTimeMillis;
            return info;
        } else {
            return nullopt;
        }
    }

    optional<AccountPositionInfo> AccountBalancePositionComposite::copy_position(string inst_id) {
        std::shared_lock<std::shared_mutex> lock(this->positionWrapper.rw_lock);
        auto item = this->positionWrapper.position_map.find(inst_id);
        if (item != this->positionWrapper.position_map.end()) {
            AccountPositionInfo info;
            info.symbol = (*item).second.symbol;
            info.initialMargin = (*item).second.initialMargin;
            info.maintMargin = (*item).second.maintMargin;
            info.unrealizedProfit = (*item).second.unrealizedProfit;
            info.entryPrice = (*item).second.entryPrice;
            info.positionSide = (*item).second.positionSide;
            info.positionAmount = (*item).second.positionAmount;
            info.positionAmountAbs = (*item).second.positionAmountAbs;
            info.updateTimeMillis = (*item).second.updateTimeMillis;
            return info;
        } else {
            return nullopt;
        }
    }

    optional<PositionThresholdInfo> AccountBalancePositionComposite::copy_position_threshold(string inst_id) {
        std::shared_lock<std::shared_mutex> lock(this->positionThresholdWrapper.rw_lock);
        auto item = this->positionThresholdWrapper.threshold_map.find(inst_id);
        if (item != this->positionThresholdWrapper.threshold_map.end()) {
            PositionThresholdInfo info;
            info.symbol = item->second.symbol;
            info.positionReduceRatio = item->second.positionReduceRatio;
            info.totalNotional = item->second.totalNotional;
            info.updateTimeMillis = item->second.updateTimeMillis;
            return info;
        } else {
            return nullopt;
        }
    }
}