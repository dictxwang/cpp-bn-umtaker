CREATE TABLE `tb_bnum_order` (
  `tid` bigint NOT NULL AUTO_INCREMENT,
  `account_flag` varchar(32) NOT NULL DEFAULT '',
  `symbol` varchar(64) NOT NULL,
  `order_side` varchar(8) NOT NULL DEFAULT '',
  `order_id` varchar(64) NOT NULL,
  `client_order_id` varchar(64) NOT NULL,
  `order_type` varchar(16) NOT NULL,
  `order_status` varchar(32) NOT NULL,
  `ct_value` decimal(16,8) DEFAULT '1.00000000',
  `order_size` decimal(16,8) DEFAULT '0.00000000',
  `filled_size` decimal(16,8) DEFAULT '0.00000000',
  `average_price` decimal(16,8) DEFAULT '0.00000000',
  `create_time` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`tid`),
  UNIQUE KEY `uidx_corder_id` (`client_order_id`),
  KEY `idx_af` (`account_flag`),
  KEY `idx_ct` (`create_time`),
  KEY `idx_afos` (`account_flag`,`order_status`)
)

CREATE TABLE `tb_bnum_pnl` (
  `tid` bigint NOT NULL AUTO_INCREMENT,
  `account_flag` varchar(32) NOT NULL DEFAULT '',
  `total_eq` float(16,6) NOT NULL DEFAULT '0.000000',
  `init_eq` float(16,6) NOT NULL DEFAULT '0.000000',
  `gross_profit` float(16,6) NOT NULL DEFAULT '0.000000',
  `gross_pnl` float(16,6) NOT NULL DEFAULT '0.000000',
  `log_ts` bigint NOT NULL DEFAULT '0',
  `create_time` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`tid`),
  UNIQUE KEY `uidx_logts_af` (`account_flag`,`log_ts`),
  KEY `ct_idx` (`create_time`) USING BTREE,
  KEY `idx_ctaf` (`account_flag`,`create_time`),
  KEY `idx_acc` (`account_flag`)
);