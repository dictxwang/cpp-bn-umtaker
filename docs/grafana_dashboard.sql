-- Trading Volume
select sum(filled_size*average_price) as total_volume from tb_bnum_order 
where 1 = 1 
  AND (COALESCE('$Account', '') = '' OR account_flag = '$Account') 
  AND (COALESCE('$Symbol', '') = '' OR COALESCE('$Symbol', '1A-ALL') = '1A-ALL' OR symbol = '$Symbol') 
  AND (COALESCE('$OrderSide', '') = '' OR COALESCE('$OrderSide', 'ALL') = 'ALL' OR order_side = '$OrderSide') 
  AND (COALESCE('$OrderStatus', '') = '' OR COALESCE('$OrderStatus', 'ALL') = 'ALL' OR order_status = '$OrderStatus') 
  AND $__timeFilter(create_time);

-- Filled Order Percentage
select round(t2.filled_order/GREATEST(t1.total_order, 1), 5) as order_percentage from 
(select count(*) as total_order from tb_bnum_order 
where 1 = 1 
  AND (COALESCE('$Account', '') = '' OR account_flag = '$Account') 
  AND (COALESCE('$Symbol', '') = '' OR COALESCE('$Symbol', '1A-ALL') = '1A-ALL' OR symbol = '$Symbol') 
  AND (COALESCE('$OrderSide', '') = '' OR COALESCE('$OrderSide', 'ALL') = 'ALL' OR order_side = '$OrderSide') 
  AND $__timeFilter(create_time)
) as t1 join 
(select count(*) as filled_order from tb_bnum_order 
where has_trading_volume = 'Y' 
  AND (COALESCE('$Account', '') = '' OR account_flag = '$Account') 
  AND (COALESCE('$Symbol', '') = '' OR COALESCE('$Symbol', '1A-ALL') = '1A-ALL' OR symbol = '$Symbol') 
  AND (COALESCE('$OrderSide', '') = '' OR COALESCE('$OrderSide', 'ALL') = 'ALL' OR order_side = '$OrderSide') 
  AND $__timeFilter(create_time)
) as t2;

-- Filled Volume Percentage
select round(t2.filled_volume/GREATEST(t1.total_volume, 1), 5) as volume_percentage from 
(select sum(order_size*average_price) as total_volume from tb_bnum_order 
where 1 = 1 
  AND (COALESCE('$Account', '') = '' OR account_flag = '$Account') 
  AND (COALESCE('$Symbol', '') = '' OR COALESCE('$Symbol', '1A-ALL') = '1A-ALL' OR symbol = '$Symbol') 
  AND (COALESCE('$OrderSide', '') = '' OR COALESCE('$OrderSide', 'ALL') = 'ALL' OR order_side = '$OrderSide') 
  AND $__timeFilter(create_time)
) as t1 join 
(select sum(filled_size*average_price) as filled_volume from tb_bnum_order 
where 1 = 1 
  AND (COALESCE('$Account', '') = '' OR account_flag = '$Account') 
  AND (COALESCE('$Symbol', '') = '' OR COALESCE('$Symbol', '1A-ALL') = '1A-ALL' OR symbol = '$Symbol') 
  AND (COALESCE('$OrderSide', '') = '' OR COALESCE('$OrderSide', 'ALL') = 'ALL' OR order_side = '$OrderSide') 
  AND $__timeFilter(create_time)
) as t2;

-- Equity
select 
  cross_balance, usdc_balance as usdc_margin_balance, usdc_cross_balance, corss_un_pnl as cross_un_pnl, create_time 
from tb_bnum_pnl 
where 1 = 1
and (COALESCE('$Account', '') = '' OR account_flag = '$Account') 
and $__timeFilter(create_time)  
order by create_time desc;

-- PNL Breakdown
-- history version
select 
  symbol,
  sum(pnl_1s_percentage) as total_pnl_1s, sum(pnl_5s_percentage) as total_pnl_5s,
  sum(pnl_10s_percentage) as total_pnl_10s, sum(pnl_20s_percentage) as total_pnl_20s,
  sum(pnl_30s_percentage) as total_pnl_30s, sum(pnl_50s_percentage) as total_pnl_50s,
  sum(pnl_60s_percentage) as total_pnl_60s, sum(pnl_120s_percentage) as total_pnl_120s,
  sum(pnl_180s_percentage) as total_pnl_180s, sum(pnl_15min_percentage) as total_pnl_15min,
  sum(pnl_30min_percentage) as total_pnl_30min, sum(pnl_1hour_percentage) as total_pnl_1hour 
 from tb_bnum_order_pnl
 where 1 = 1 
  AND (COALESCE('$Account', '') = '' OR account_flag = '$Account') 
  AND (COALESCE('$Symbol', '') = '' OR COALESCE('$Symbol', '1A-ALL') = '1A-ALL' OR symbol = '$Symbol') 
  AND (COALESCE('$ExcludeNoVolume', 'N') = 'N' OR has_trading_volume = '$ExcludeNoVolume') 
  AND $__timeFilter(create_time)
 group by symbol 
 order by symbol; 
-- current version
select t1.*, t2.total_pnl_1s*100 as total_pnl_1s, t2.total_pnl_5s*100 as total_pnl_5s, t2.total_pnl_10s*100 as total_pnl_10s, t2.total_pnl_20s*100 as total_pnl_20s, t2.total_pnl_30s*100 as total_pnl_30s, t2.total_pnl_50s*100 as total_pnl_50s, t2.total_pnl_60s*100 as total_pnl_60s, t2.total_pnl_120s*100 as total_pnl_120s, t2.total_pnl_180s*100 as total_pnl_180s, t2.total_pnl_15min*100 as total_pnl_15min, t2.total_pnl_30min*100 as total_pnl_30min, t2.total_pnl_1hour*100 as total_pnl_1hour from 
(
 select tmp_1.symbol, tmp_1.price_precision, tmp_1.quantity_precision, tmp_2.total_order, tmp_2.total_notional
  from tb_bnum_exchange_info as tmp_1 left join 
 (select 
  symbol as tmp_2_symbol,
  count(0) as total_order, sum(filled_size*average_price) as total_notional
 from tb_bnum_order
 where 1 = 1 
  AND (COALESCE('$Account', '') = '' OR account_flag = '$Account') 
  AND (COALESCE('$Symbol', '') = '' OR COALESCE('$Symbol', '1A-ALL') = '1A-ALL' OR symbol = '$Symbol') 
  AND (COALESCE('$ExcludeNoVolume', 'N') = 'N' OR has_trading_volume = '$ExcludeNoVolume') 
  AND $__timeFilter(create_time)
 group by tmp_2_symbol 
 order by tmp_2_symbol) as tmp_2 
 on tmp_1.symbol = tmp_2.tmp_2_symbol
 where 1 = 1
  AND (COALESCE('$Account', '') = '' OR tmp_1.account_flag = '$Account') 
  AND (COALESCE('$Symbol', '') = '' OR COALESCE('$Symbol', '1A-ALL') = '1A-ALL' OR tmp_1.symbol = '$Symbol') 
) as t1 left join 
(
 select 
  symbol as t2_symbol,
  sum(pnl_1s_percentage) as total_pnl_1s, sum(pnl_5s_percentage) as total_pnl_5s,
  sum(pnl_10s_percentage) as total_pnl_10s, sum(pnl_20s_percentage) as total_pnl_20s,
  sum(pnl_30s_percentage) as total_pnl_30s, sum(pnl_50s_percentage) as total_pnl_50s,
  sum(pnl_60s_percentage) as total_pnl_60s, sum(pnl_120s_percentage) as total_pnl_120s,
  sum(pnl_180s_percentage) as total_pnl_180s, sum(pnl_15min_percentage) as total_pnl_15min,
  sum(pnl_30min_percentage) as total_pnl_30min, sum(pnl_1hour_percentage) as total_pnl_1hour 
 from tb_bnum_order_pnl
 where 1 = 1 
  AND (COALESCE('$Account', '') = '' OR account_flag = '$Account') 
  AND (COALESCE('$Symbol', '') = '' OR COALESCE('$Symbol', '1A-ALL') = '1A-ALL' OR symbol = '$Symbol') 
  AND (COALESCE('$ExcludeNoVolume', 'N') = 'N' OR has_trading_volume = '$ExcludeNoVolume') 
  AND $__timeFilter(create_time)
 group by t2_symbol 
) as t2
 on t1.symbol = t2.t2_symbol
 where 1 = 1 
 order by symbol;

-- Position
select log_time, sum(long_position_notional) as long_position_notional, sum(short_position_notional) as short_position_notional from 
 (select log_time, case position_side when 'LONG' then position_notional else 0 end as long_position_notional, case position_side when 'SHORT' then position_notional else 0 end as short_position_notional 
  from tb_bnum_position
  where 1=1 AND (COALESCE('$Account', '') = '' OR account_flag = '$Account') AND (COALESCE('$Symbol', '') = '' OR COALESCE('$Symbol', '1A-ALL') = '1A-ALL' OR symbol = '$Symbol') AND $__timeFilter(log_time)
 ) as tmp
 group by log_time
 order by log_time;

-- Orders
select 
 t1.symbol, t1.client_order_id, t1.order_side, t1.order_type, t1.order_status, t1.order_size, t1.filled_size, t1.average_price, t1.create_time,
 t2.price_after_1s, t2.pnl_1s_percentage*100 as pnl_1s_percentage, t2.price_after_5s, t2.pnl_5s_percentage*100 as pnl_5s_percentage, t2.price_after_10s, t2.pnl_10s_percentage*100 as pnl_10s_percentage, t2.price_after_20s, t2.pnl_20s_percentage*100 as pnl_20s_percentage,
 t2.price_after_30s, t2.pnl_30s_percentage*100 as pnl_30s_percentage, t2.price_after_50s, t2.pnl_50s_percentage*100 as pnl_50s_percentage, t2.price_after_60s, t2.pnl_60s_percentage*100 as pnl_60s_percentage, t2.price_after_120s, t2.pnl_120s_percentage*100 as pnl_120s_percentage,
 t2.price_after_180s, t2.pnl_180s_percentage*100 as pnl_180s_percentage, t2.price_after_15min, t2.pnl_15min_percentage*100 as pnl_15min_percentage, t2.price_after_30min, t2.pnl_30min_percentage*100 as pnl_30min_percentage, t2.price_after_1hour, t2.pnl_1hour_percentage*100 as pnl_1hour_percentage
from tb_bnum_order as t1 left join tb_bnum_order_pnl as t2 on t1.client_order_id = t2.client_order_id
where 1 = 1
  AND (COALESCE('$Account', '') = '' OR t1.account_flag = '$Account') 
  AND (COALESCE('$Symbol', '') = '' OR COALESCE('$Symbol', '1A-ALL') = '1A-ALL' OR t1.symbol = '$Symbol') 
  AND (COALESCE('$OrderSide', '') = '' OR COALESCE('$OrderSide', 'ALL') = 'ALL' OR t1.order_side = '$OrderSide')
  AND (COALESCE('$OrderStatus', '') = '' OR COALESCE('$OrderStatus', 'ALL') = 'ALL' OR t1.order_status = '$OrderStatus')  
  AND (COALESCE('$ExcludeNoVolume', 'N') = 'N' OR t1.has_trading_volume = '$ExcludeNoVolume') 
  AND $__timeFilter(t1.create_time) 
order by create_time desc;