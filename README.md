# Implementation of TCP-NV (New Vegas) in ns-3
## Course Code - CO300
An attempt to implement TCP - New Vegas in ns3
## Overview 
TCP-NV(New Vegas) is a major update to TCP-Vegas. Like Vegas, NV is a delay based congestion avoidance mechanism for TCP. Its filtering mechanism is similar: it uses the best measurement in a particular period to detect and measure congestion. More details can be found [here](https://github.com/mishal23/Implementation-of-TCP-New-Vegas-in-ns3/wiki/TCP-New-Vegas)

### When use TCP New Vegas?
- TCP New Vegas is mainly recommended for traffic within a data center, and when all the flows are NV (at least those within the data center). This is due to the inherent unfairness between flows using losses to detect congestion and those that use queue buildup to detect congestion (congestion avoidance).
- TCP New Vegas is developed to coexist with modern networks where links bandwidths are 10 Gbps or higher, where the RTTs can be 10’s of microseconds, where interrupt coalescence and TSO/GSO can introduce noise and nonlinear effects, etc.

## Results

TCP New Vegas vs TCP Vegas (cwnd vs time)
<img src="https://srv2.imgonline.com.ua/result_img/imgonline-com-ua-twotoone-n6EMttlWH91G.png" alt="CWND vs Time">

View other graphs plotted [here](https://github.com/mishal23/Implementation-of-TCP-New-Vegas-in-ns3/tree/master/ns3/scratch/plots)

### Inferences from the plot of TCP New Vegas
- As there was no congestion, so cwnd increased exponentially, according to the slow start algorithm,
- Now it's uses avgRTT to find out that the network is getting congested, so it reduces the cwnd (there was no packet loss)
- Depending upon the avgRTT, either cwnd increases, decreases or remains constant.

## Implementation
- The files edited/added by us to implement TCP New Vegas are `ns3/src/internet/model/tcp-nv.{h, cc}` and `ns3/src/internet/wscript`
- The `tcp-variants-comparison.cc` from `ns3/examples/tcp/` was copied to the scratch directory and the plots were plotted.

### Reproducing Results
``` shell
$ git clone https://github.com/mishal23/Implementation-of-TCP-New-Vegas-in-ns3/
$ cd Implementation-of-TCP-New-Vegas-in-ns3/ns3/
$ ./waf
$ ./waf configure --enable-examples
$ mkdir -p scratch/data
$ ./waf --run tcp-variants-comparison --command-template="%s --transport_prot=TcpNewVegas --tracing=true --prefix_name='scratch/data/TcpNewVegas'"
```
- This would generate the following files in the data directory `TcpNewVegas-ascii`, `TcpNewVegas-cwnd.data`, `TcpNewVegas-inflight.data`, `TcpNewVegas-next-rx.data`, `TcpNewVegas-next-tx.data`, `TcpNewVegas-rto.data`, `TcpNewVegas-rtt.data`, `TcpNewVegas-ssth.data`. 
- Plot the graphs using gnuplot
``` shell
$ sudo apt-get install gnuplot-nox
```
- On the terminal type `gnuplot` which will open the gnuplot shell, after that type
```shell
$ set terminal png size 640,480
$ set output "cwnd.png"
$ plot "TcpNewVegas-cwnd.data"
```
- You may similarly plot other graphs

## Implementation Limitations & Future Scope
- Future MinRTTs which was set based on `jiffies`(in Linux), isn't implemented. <sup>[[1]](#FootNotes)</sup>
- The Linux implementation uses [Berkeley Packet Filter(bpf)](https://en.wikipedia.org/wiki/Berkeley_Packet_Filter) to initialize the `base_rtt` to know if we have resonable RTTs, however ns3 doesn't have bpf implementation because of which it hasn't been implemented. <sup>[[2]](#FootNotes)</sup>

## References
- TCP New Vegas by Brakmo - [https://docs.google.com/document/d/1o-53jbO_xH-m9g2YCgjaf5bK8vePjWP6Mk0rYiRLK-U/](https://docs.google.com/document/d/1o-53jbO_xH-m9g2YCgjaf5bK8vePjWP6Mk0rYiRLK-U/)
- TCP New Vegas [https://github.com/torvalds/linux/blob/master/net/ipv4/tcp_nv.c](https://github.com/torvalds/linux/blob/master/net/ipv4/tcp_nv.c)

### FootNotes
[1] [Jiffies Implementation in Linux](https://elixir.bootlin.com/linux/latest/source/include/linux/jiffies.h)<br>
[2] BPF - Berkeley Packet Filter [Linux Patch](https://lwn.net/Articles/726694/) | [Paper](https://netdevconf.org/2.2/papers/brakmo-tcpbpf-talk.pdf)
