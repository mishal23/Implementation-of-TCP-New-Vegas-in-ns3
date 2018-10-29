# TCP New Vegas

## Initilization
#### TCP NV:
- To set the value of base_rtt, it calls `tcp_call_bpf`, TCP BPF is a new mechanism that supports setting TCP parameters [paper here](https://netdevconf.org/2.2/papers/brakmo-tcpbpf-talk.pdf)
- TCP NV is meant to be used in environments, such as communication within datacenters, where we'll have reasonable RTTs
- Initialize `nv_allow_cwnd_growth`(1), `nv_min_rtt_reset_jiffies`(), `nv_min_rtt` (INT_MAX), `nv_min_rtt_new`(INT_MAX), `nv_min_cwnd`(4), `nv_catchup` (0), `cwnd_growth_factor` (0)

#### New cwnd calculation
- In slow start we allow cwnd to grow as long as the application has used half the cwnd.(ensure that cwnd grows twice what was ACKED)
- Allow to grow only if it has not detected congestion.
- If we are in slow start phase, we check for the leftover acknowledgements after slow start exits to adjust cwnd in congestion avoidance mode, if there aren't any acknowledgements, we return.
- if `cwnd_growth_factor` < 0 :
  - set cnt : `snd_cwnd` << -`cwnd_growth_factor`
  - and we perform congestion avoidance additive inverse.
- else:
  - set cnt : max(4U, `snd_cwnd` >> `cwnd_growth_factor`)
  - and we perform congestion avoidance additive inverse.

#### Slow Start Threshold
- Return maximum of (`snd_cwnd`\ `nv_loss_dec_factor`)>>10, 2U)
- Equivalent to Reno, works well when network is so congested that NV cannot
prevent many losses.

#### Set state (Called before changing ca_state)
- If the connection is open and `nv_reset`, reset.
- Else if **Loss state**(congestion window was reduced due to
RTO timeout) or **CWR** (handle some Congestion Notification
event) or **Recovery** (Fast Retransmit Stage):
    - Set `nv_reset` to 1
    - Set `nv_allow_cwnd_growth` to 0
    - If **Loss State**:
      - if `cwnd_growth_factor`>0, set `cwnd_growth_factor` : 0 **[Reno Value]**
      - if `nv_cwnd_growth_rate_neg`>0 & `cwnd_growth_factor`>-8, decrease `cwnd_growth_factor` by 1

#### Packets acked
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
- We apply bounds to RTT, and update the `nv_min_rtt` and `nv_min_rtt_new` if necessary
- *To be updated*
