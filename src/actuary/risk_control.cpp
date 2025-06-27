#include "risk_control.h"

using namespace std;

namespace actuary {

    void start_watchdog(ActuaryConfig& config, GlobalContext& context) {

        thread account_meta_thread(watch_account_meta, ref(config), ref(context));
        account_meta_thread.detach();
        info_log("start thread of watching account meata.");

        thread bnb_balance_thread(watch_bnb_balance, ref(config), ref(context));
        bnb_balance_thread.detach();
        info_log("start thread of watching bnb balance.");
    }

    void watch_account_meta(ActuaryConfig& config, GlobalContext& context) {

        while (true) {
            this_thread::sleep_for(chrono::minutes(5));
            AccountMetaInfo meta = context.get_balance_position_composite().copy_meta();
            uint64_t now = binance::get_current_ms_epoch();
            if (meta.updateTimeMillis + 300*100 < now) {
                // expired
                if (!(*(context.get_dynamic_config())).is_stop_make_order_as_reason(STOP_REASON_ACCOUNT_META)) {
                    (*(context.get_dynamic_config())).stop_make_order(STOP_REASON_ACCOUNT_META);
                    send_warning_message(config, context, string("stop make order: account meta is expired."));
                }
                err_log("stop make order as meta info is expired, latest update time {} now is {}", meta.updateTimeMillis, now);
                continue;
            } else {
                if (context.get_dynamic_config()->is_stop_make_order_as_reason(STOP_REASON_ACCOUNT_META)) {
                    context.get_dynamic_config()->resume_make_order(STOP_REASON_ACCOUNT_META);
                    send_warning_message(config, context, string("resume make order: account meta refresh time is normal."));
                    err_log("resume make order as meta refresh time is normal, latest update time {} now is {}", meta.updateTimeMillis, now);
                }
            }

			double marginUseRatio;
			if (meta.totalCrossWalletBalance <= 0) {
				marginUseRatio = 1;
			} else {
				marginUseRatio = meta.totalInitialMargin / meta.totalCrossWalletBalance;
			}

            if (marginUseRatio > config.margin_ratio_thresholds[1]) {
                if (!(*(context.get_dynamic_config())).is_stop_make_open_order()) {
                    (*(context.get_dynamic_config())).stop_make_open_order();
                    send_warning_message(config, context, string("stop make open position order: margin ratio exceeds limit."));
                }
                err_log("stop make open position order as margin ratio exceeds limit {}", config.margin_ratio_thresholds[1]);
                continue;
            }

            if (marginUseRatio < config.margin_ratio_thresholds[0]) {
                if ((*(context.get_dynamic_config())).is_stop_make_open_order()) {
                    (*(context.get_dynamic_config())).resume_make_open_order();
                    send_warning_message(config, context, string("resume make open position order: margin ratio has been recovery."));
                    err_log("resume make open position order as margin ratio has been recovery {}", config.margin_ratio_thresholds[0]);
                }
            }

            info_log("finish to watch account margin ratio");
        }
    }

    void watch_bnb_balance(ActuaryConfig& config, GlobalContext& context) {
        
        while (true) {
            this_thread::sleep_for(chrono::minutes(5));
            optional<AccountBalanceInfo> balance = context.get_balance_position_composite().copy_balance("BNB");
            if (!balance.has_value()) {
                // null
                if (!(*(context.get_dynamic_config())).is_stop_make_order_as_reason(STOP_REASON_BNB)) {
                    (*(context.get_dynamic_config())).stop_make_order(STOP_REASON_BNB);
                    send_warning_message(config, context, string("stop make order: balance of bnb not found."));
                }
                err_log("stop make order as balance of bnb not found");
                continue;
            }
            uint64_t now = binance::get_current_ms_epoch();
            if ((*balance).updateTimeMillis + 300*100 < now) {

                // expired
                if (!(*(context.get_dynamic_config())).is_stop_make_order_as_reason(STOP_REASON_BNB)) {
                    (*(context.get_dynamic_config())).stop_make_order(STOP_REASON_BNB);
                    send_warning_message(config, context, string("stop make order: balance of bnb is expired."));
                }
                err_log("stop make order as balance of bnb is expired, latest update time {} now is {}", (*balance).updateTimeMillis, now);
                continue;
            } else {
                if (context.get_dynamic_config()->is_stop_make_order_as_reason(STOP_REASON_BNB)) {
                    context.get_dynamic_config()->resume_make_order(STOP_REASON_BNB);
                    send_warning_message(config, context, string("resume make order: balance of bnb refresh time is normal."));
                    err_log("resume make order as balance of bnb refresh time is normal, latest update time {} now is {}", (*balance).updateTimeMillis, now);
                }
            }

            if ((*balance).crossWalletBalance < config.bnb_balance_thresholds[0]) {
                // too little
                if (!(*(context.get_dynamic_config())).is_stop_make_order_as_reason(STOP_REASON_BNB_SHORTAGE)) {
                    (*(context.get_dynamic_config())).stop_make_order(STOP_REASON_BNB_SHORTAGE);
                    send_warning_message(config, context, string("stop make order: only few balance of bnb."));
                }
                err_log("stop make order as only few balance of bnb {}", (*balance).crossWalletBalance);
                continue;
            }
            if ((*balance).crossWalletBalance > config.bnb_balance_thresholds[1]) {
                // enough
                if ((*(context.get_dynamic_config())).is_stop_make_order_as_reason(STOP_REASON_BNB_SHORTAGE)) {
                    bool resumed = (*(context.get_dynamic_config())).resume_make_order(STOP_REASON_BNB_SHORTAGE);
                    if (resumed) {
                        send_warning_message(config, context, string("resume make order: has enough balance of bnb."));
                    }
                    err_log("resume make order as has enough balance of bnb {} {}", (*balance).crossWalletBalance, resumed);
                }
            }
        }
    }

    void send_warning_message(ActuaryConfig& config, GlobalContext& context, string message) {
        if (!config.tg_send_message) {
            warn_log("close send tg messag: {}", message);
        } else if (!config.dt_group_main_node) {
            info_log("not main node, skip send tg message: {}", message);
        } else {
            message = "[" + config.account_flag + "]" + message;
            pair<int, string> res = context.get_tg_bot().send_message(config.tg_chat_id, message);
            if (res.first != 0) {
                err_log("fail to send tg message: {} {}", res.first, res.second);
            }
        }
    }
}