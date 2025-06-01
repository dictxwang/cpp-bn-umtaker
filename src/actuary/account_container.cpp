#include "account_container.h";

using namespace std;

namespace actuary {
    void AccountBalancePositionComposite::init(vector<string> assets, vector<string> inst_ids) {
        for (string asset : assets) {
            AccountBalanceInfo balance;
            this->balanceWrapper.balance_map.insert({asset, balance});
        }
        for (string inst_id : inst_ids) {
            AccountPositionInfo position;
            this->positionWrapper.position_map.insert({inst_id, position});
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
        this->meta.updateTimeMills = binance::get_current_ms_epoch();
        w_lock.unlock();
    }
    
    bool AccountBalancePositionComposite::update_balance(binance::FuturesAccountAsset& balance) {
        
        AccountBalanceInfo item;
        item.asset = balance.asset;
        item.walletBalance = balance.walletBalance;
        item.crossWalletBalance = balance.crossWalletBalance;
        item.crossUnPnl = balance.crossUnPnl;
        item.updateTimeMills = balance.updateTime;

        std::unique_lock<std::shared_mutex> w_lock(this->balanceWrapper.rw_lock);
        this->balanceWrapper.balance_map[item.asset] = item;
        w_lock.unlock();
    }

    bool AccountBalancePositionComposite::update_position(binance::FuturesAccountPosition& position) {

        AccountPositionInfo item;
        item.symbol = position.symbol;
        item.initialMargin = position.initialMargin;
        item.maintMargin = position.maintMargin;
        item.unrealizedProfit = position.unrealizedProfit;
        item.entryPrice = position.entryPrice;
        item.positionSide = position.positionSide;
        item.positionAmt = position.positionAmt;
        item.updateTimeMills = position.updateTime;

        std::unique_lock<std::shared_mutex> w_lock(this->positionWrapper.rw_lock);
        this->positionWrapper.position_map[item.symbol] = item;
        w_lock.unlock();
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
        info.updateTimeMills = this->meta.updateTimeMills;
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
            info.updateTimeMills = (*item).second.updateTimeMills;
            return info;
        } else {
            return nullopt;
        }
    }

    optional<AccountPositionInfo> AccountBalancePositionComposite::copy_position(string inst_id) {
        std::shared_lock<std::shared_mutex> lock(this->positionWrapper.rw_lock);
        // TODO
    }
}