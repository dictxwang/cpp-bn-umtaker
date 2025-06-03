#include "risk_control.h"

using namespace std;

namespace actuary {

    void start_watchdog(ActuaryConfig& config, GlobalContext& context) {

    }

    void watch_account_meta(ActuaryConfig& config, GlobalContext& context) {

        while (true) {
            this_thread::sleep_for(chrono::minutes(2));
            AccountMetaInfo meta = context.get_balance_position_composite().copy_meta();
            uint64_t now = binance::get_current_ms_epoch();
            if (meta.updateTimeMills + 300*100 < now) {
                // expired
                context.stop_make_order();
                send_warning_message(config, context, string("stop make order: account meta is expired"));
                err_log("meta info is expired, latest update time {} now is {}", meta.updateTimeMills, now);
                continue;
            }
            // TODO
        }
    }

    void watch_bnb_balance(ActuaryConfig& config, GlobalContext& context) {}


    void send_warning_message(ActuaryConfig& config, GlobalContext& context, std::string message) {
        if (!config.tg_send_message) {
            warn_log("close send tg messag: {}", message);
        } else {
            message = "[" + config.account_flag + "]" + message;
            pair<int, string> res = context.get_tg_bot().send_message(config.tg_chat_id, message);
            if (res.first != 0) {
                err_log("fail to send tg message: {} {}", res.first, res.second);
            }
        }
    }
}