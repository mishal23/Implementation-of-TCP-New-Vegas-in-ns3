# Linux Kernel
 Pseudo Code of TCP Vegas and TCP New Vegas, and comparison between each of the functions.

## Differences between TCP Vegas and TCP New Vegas

<table>
<tr>
  <td>Function</td>
  <td>Description</td>
  <td>TCP Vegas</td>
  <td>TCP New Vegas</td>
</tr>
<tr>
  <td>.init</td>
  <td>To initialize private data</td>
  <td>vegas_init</td>
  <td>tcpnv_init</td>
</tr>
<tr>
  <td>.pkts_acked</td>
  <td>Hook for packet ack accounting</td>
  <td>tcp_vegas_pkts_acked</td>
  <td>tcpnv_acked</td>
</tr>
<tr>
  <td>.set_state</td>
  <td>Call before changing ca_state</td>
  <td>tcp_vegas_state</td>
  <td>tcpnv_state</td>
</tr>
<tr>
  <td>.ssthresh</td>
  <td>Returns the slow start threshold</td>
  <td>tcp_reno_ssthresh</td>
  <td>tcpnv_recalc_ssthresh</td>
</tr>
<tr>
  <td>.cwnd_event</td>
  <td>Function for when cwnd event occurs</td>
  <td>tcp_vegas_cwnd_event</td>
  <td> - </td>
</tr>
<tr>
  <td>.cong_avoid</td>
  <td>New cwnd calculation</td>
  <td>tcp_vegas_cong_avoid</td>
  <td>tcpnv_cong_avoid</td>
</tr>
<tr>
  <td>.undo_cwnd</td>
  <td>New value of cwnd after loss</td>
  <td>tcp_reno_undo_cwnd</td>
  <td>tcp_reno_undo_cwnd</td>
</tr>
</table>

### Enable TCP Vegas `vegas_enable`
- Set `doing_vegas_now` variable true
- Set the beginning of the next send window
- Set `minRTT` to 0x7fffffff
- Set `cntRTT` to 0

### Disable TCP Vegas `vegas_disable`
- Set `doing_vegas_now` variable false

### Reset TCP New Vegas `tcpnv_reset`
- Set `nv_reset`: 0
- Set `nv_no_cong_cnt`: 0
- Set `nv_rtt_cnt` : 0
-	Set `nv_last_rtt` : 0
-	Set `nv_rtt_max_rate` : 0
-	Set `nv_rtt_start_seq`: tp->snd_una (snd_una is sequence number of 1st byte of data that has been sent but not acknowledged yet)
- Set `nv_eval_call_cnt`: 0;
-	Set `nv_last_snd_una` : tp->snd_una;

### Initilization `.init`
- TCPV : Initialize baseRTT = 0x7fffffff, enable TCP Vegas
- TCPNV :
  - Find the `base_rtt` from `tcp_call_bpf`
  - if `base_rtt` > 0,
    - Set `nv_base_rtt` : `base_rtt`
    - Set `nv_lower_bound_rtt` : 80% of `base_rtt`
  - else
    - Set `nv_base_rtt` : 0
    - Set `nv_lower_bound_rtt` : 0
  - Set `nv_allow_cwnd_growth` : 1
  - Set `nv_min_rtt_reset_jiffies` : jiffies + 2 * HZ;
	- Set `nv_min_rtt` : 32 bit MAX;
	- Set `nv_min_rtt_new` : 32 bit MAX;
	- Set `nv_min_cwnd` : 4;
	- Set `nv_catchup` : 0;
	- Set `cwnd_growth_factor` : 0;

### Returns the new Slow Start Threshold `.ssthresh`
  - **TCP Vegas Uses TCP Reno** : return max(`snd_cwnd`>>1U, 2U)
  - TCPNV: return max(`snd_cwnd` * `nv_loss_dec_factor` >> 10, 2U)

### Congestion Avoidance `.cong_avoid`
##### TCP Vegas
  - If vegas isn't on, go to `tcp_reno_cong_avoid`
  - If `beg_send_nxt` > `ack`
    - `beg_snd_nxt` : `snd_nxt`
    - if `cntRTT` <= 2, go to `tcp_reno_cong_avoid`
    - else
      - set local `rtt` : `minRTT`
      - set `target_cwnd` : `snd_cwnd` * `baseRTT`
      - set `target_cwnd` : `target_cwnd`/`rtt`
      - set diff : `snd_cwnd`\*(`rtt` - `baseRTT`) / (`baseRTT`)
      - if diff > `gamma` (1) and `tcp_in_slow_start`
        - set `snd_cwnd` : min(`snd_cwnd`, `target_cwnd`+1)
        - set `snd_ssthresh` : min(`snd_ssthresh`, `snd_cwnd` - 1)
      - else if `tcp_in_slow_start` : `tcp_slow_start`
      - else
        - if diff > beta :
          - set `snd_cwnd` : `snd_cwnd`--
          - set `snd_ssthresh` : min(`snd_cwnd`, `target_cwnd`+1)
        - else if diff < alpha : set `snd_cwnd` : `snd_cwnd`++
      - if `snd_cwnd` < 2 : set `snd_cwnd` : 2
      - else if `snd_cwnd` > `snd_cwnd_clamp`:
        - set `snd_cwnd` : `snd_cwnd_clamp`
      - set `snd_ssthresh` : `tcp_current_ssthresh`
    - set `cntRTT` : 0
    - set `minRTT` : 0x7fffffff
  - else if `tcp_in_slow_start` : `tcp_slow_start`


#### TCP New Vegas
  - if `tcp_is_cwnd_limited` is 0 : return
  - if `nv_allow_cwnd_growth` is 0 : return
  - if `tcp_in_slow_start`:
    - set acked : `tcp_slow_start`
    - if not acked : return
  - if `cwnd_growth_factor` < 0 :
    - set cnt : `snd_cwnd` << -`cwnd_growth_factor`
    - `tcp_cong_avoid_ai(tp, cnt, acked)`
  - else:
    - set cnt : max(4U, `snd_cwnd` >> `cwnd_growth_factor`)
    - `tcp_cong_avoid_ai(tp, cnt, acked)`


### State `.set_state`
- TCP V: when connection open, `vegas_enable` else `vegas_disable`
- TCP NV:
  - if connection is open and `nv_reset` : tcpnv_reset
  - else if **Loss state**(congestion window was reduced due to
RTO timeout) or **CWR** (handle some Congestion Notification
event) or **Recovery** (Fast Retransmit Stage):
    - Set `nv_reset` : 1
    - Set `nv_allow_cwnd_growth`: 0
    - If **Loss State**:
      - if `cwnd_growth_factor`>0, set `cwnd_growth_factor` : 0 [Reno Value]
      - if `nv_cwnd_growth_rate_neg`>0 & `cwnd_growth_factor`>-8 : `cwnd_growth_factor`--

### Undo Current window(New Value of cwnd after loss) `.undo_cwnd`
- Both TCP Vegas and TCP NV uses `tcp_reno_undo_cwnd`

### Current Window Event [TCP Vegas only] `cwnd_event`
- if connection **RESTART** or **START** : `tcp_vegas_init`

## Congestion Avoidance Calculations `.pkts_acked`

#### TCP Vegas
- if `sample->rtt_us` < 0 : return
- set vrtt : `sample->rtt_us` + 1
- if vrtt < `vegas->baseRTT`
  - `vegas->baseRTT` : vrtt
- set `vegas->minRTT` : min(vegas->minRTT, vrtt)
- ser `vegas->cntRTT` : `vegas->cntRTT` + 1

#### TCP New Vegas

- If some timestamp is missing for a sample(`bytes`, `pkts_acked`, `rtt`), than we return
- If TCP is in Loss State or CWR State or Recovery State, then return
- If we are catching CWND(whether we are growing because of temporary cwnd decrease) and `snd_cwnd` > 2, then set catchup to 0, and do not allow growth
- If bytes in flight is zero, return
- If `nv_last_rtt` > 0, calculate averageRTT, formula in TCP-NV document, else `avg_rtt` = rtt and `nv_min_rtt` = `avg_rtt`/2
- `nv_last_rtt` = `avg_rtt`
- rate64 = bytes in flight * 80000,
- if `avg_rtt` > 0, then rate64 = rate64/`avg_rtt`
- if `avg_rtt` = 0, then rate64 remains unaltered
- Update `nv_rtt_max_rate` = rate
- Increment `nv_eval_call_cnt` if its less than 255
- We apply `nv_get_bounded_rtt` function that returns the min of `nv_lower_bound_rtt`,`nv_base_rtt` and `avg_rtt` and `avg_rtt` is set to that
- Update `nv_min_rtt` and `nv_min_rtt_new` if necessary
- if now > `nv_min_rtt_reset_jiffies`
  - `nv_min_rtt` = `nv_min_rtt_new`  nv_min_rtt is updated with the minimum (possibley averaged) rtt
  - `nv_min_rtt_new` = `infinity`
  - `nv_min_rtt_reset_jiffies` = now + (`nv_reset_period` * (384+rand) * HZ)>>9  In practice we introduce some randomness
  - `nv_min_cwnd` = max(`nv_min_cwnd`/2,4)
- if `nv_rtt_start_seq` - `snd_una` is positive then: Once every RTT check if CA needs to be done
    - update `nv_rtt_start_seq` as `snd_nxt`
    - increment `nv_rtt_cnt` if required without CA decision
    - if `nv_eval_call_cnt` = 1 AND bytes_acked >= (`nv_min_cwnd` - 1) * `mss_cache` AND `nv_min_cwnd` < 81 :
     - `nv_min_cwnd` = min(`nv_min_cwnd` + 2 , 81 )
     - `nv_rtt_start_ses` = `snd_next` + `nv_min_cwnd` * `mss_cache`
     - `nv_eval_call_cnt` = 0
     - `nv_allow_cwnd_growth` = 1
    - if above function is called once within a RTT cwnd is small hence need to inc nv_min_cwnd
-  slope = 80000 * `mss_cache` / `nv_min_rtt`
- cwnd_by_slope = `nv_rtt_max_rate`/slope
-  max_win = cwnd_by_slope + `nv_pad`
- If cwnd > max_win, decrease cwnd if cwnd < max_win, grow cwnd else leave the same
- if `snd_cwnd` > max_win :
     -  CA decision can be made when
       - We should have at least nv_dec_eval_min_calls data points before making a CA  decision
       - We only make a congesion decision after nv_rtt_min_cnt RTTs
     - if `nv_rtt_cnt` < `nv_rtt_min_cnt` return
     - else if `ssthresh` = infinity
      - if `nv_eval_call_cnt` < `nv_ssthresh_eval_min_calls` return
  - else if `nv_eval_call_cnt` < `nv_dec_eval_min_call`
         - if `nv_allow_cwnd_growth` AND `nv_rtt_cnt` > `nv_stop_rtt_cnt`
            - `nv_allow_cwnd_growth` = 0 return
  - We have enough data to determine we are congested
  - `nv_allow_cwnd_growth` = 0
  - `snd_ssthresh` = (`nv_ssthresh_factor` * max_win) >> 3
  - if (`snd_cwnd` - max_win) > 2
   - dec = max(2U, ((`snd_cwnd` - max_win) * `nv_cong_dec_mult`)>>7
   - `snd_cwnd` = dec
  - else if (`nv_cong_dec_mult` > 0)
   - `snd_cwnd` = max_win
   - if `cwnd_growth_facto`r > 0 then `cwnd_growth_factor` = 0
   - `nv_no_cong_cnt` = 0
- else if `snd_cwnd` < max_win - `nv_pad_buffer`
   - There is no congestion, grow cwnd if allowed
   - if `nv_eval_call_cnt` < `nv_inc_eval_min_calls` then return
   - `nv_allow_cwnd_growth` = 1
   - increment `nv_no_cong_cnt`
   - if `cwnd_growth_factor` < 0 AND `nv_cwnd_growth_rate_neg` > 0 AND `nv_no_cong_cnt` > `nv_cwnd_growth_rate_neg`
    - increment `cwnd_growth_factor`
    - `nv_no_cong_cnt` = 0
   - else if `cwnd_growth_factor` >=0 AND `nv_cwnd_growth_rate_pos` > 0 AND `nv_no_cong_cnt` > `nv_cwnd_growth_rate_pos`
    - increment `cwnd_growth_factor`
    - `nv_no_cong_cnt` = 0
- else
   - cwnd is in-between
   - don't do anything return
- update state
- `nv_eval_call_cnt` = 0
- `nv_rtt_cnt` = 0
- `nv_rtt_max_rate` = 0
- We don't want to make cwnd < min_cwnd, if it is then update
- if `snd_cwnd` < `nv_min_cwnd`
    - `snd_cwnd` = `nv_min_cwnd`
