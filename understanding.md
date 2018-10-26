#

## Initilization
#### TCP NV:
- To set the value of base_rtt, it calls `tcp_call_bpf`, TCP BPF is a new mechanism that supports setting TCP parameters [paper here](https://netdevconf.org/2.2/papers/brakmo-tcpbpf-talk.pdf)
-

## Packets acked
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
