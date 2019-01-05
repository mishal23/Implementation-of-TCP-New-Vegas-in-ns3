/*
 * TCP NV: TCP with Congestion Avoidance
 *
 * TCP-NV is a successor of TCP-Vegas that has been developed to
 * deal with the issues that occur in modern networks.
 * Like TCP-Vegas, TCP-NV supports true congestion avoidance,
 * the ability to detect congestion before packet losses occur.
 * When congestion (queue buildup) starts to occur, TCP-NV
 * predicts what the cwnd size should be for the current
 * throughput and it reduces the cwnd proportionally to
 * the difference between the current cwnd and the predicted cwnd.
 *
 * NV is only recommeneded for traffic within a data center, and when
 * all the flows are NV (at least those within the data center). This
 * is due to the inherent unfairness between flows using losses to
 * detect congestion (congestion control) and those that use queue
 * buildup to detect congestion (congestion avoidance).
 *
 * Note: High NIC coalescence values may lower the performance of NV
 * due to the increased noise in RTT values. In particular, we have
 * seen issues with rx-frames values greater than 8.
 *
 */

#include "tcp-nv.h"
#include "ns3/log.h"


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TcpNewVegas");
NS_OBJECT_ENSURE_REGISTERED (TcpNewVegas);

TypeId
TcpNewVegas::GetTypeId (void)
{
	static TypeId tid = TypeId ("ns3::TcpNewVegas")
	.SetParent<TcpNewReno> ()
    .AddConstructor<TcpNewVegas> ()
    .SetGroupName ("Internet")
    .AddAttribute ("m_Pad", "Max queued packets allowed in Network",
                   UintegerValue (10),
                   MakeUintegerAccessor (&TcpNewVegas::m_Pad),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("m_ResetPeriod", "m_MinRtt reset period (secs)",
                   TimeValue (Seconds (5)),
                   MakeTimeAccessor (&TcpNewVegas::m_ResetPeriod),
                   MakeTimeChecker())
    .AddAttribute ("m_MinCwnd", "NV will not decrease cwnd below this value without losses",
                   UintegerValue (2),
                   MakeUintegerAccessor (&TcpNewVegas::m_MinCwnd),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute("m_NvAllowCwndGrowth", "",
    	           UintegerValue(1),
    	           MakeUintegerAccessor(&TcpNewVegas::m_NvAllowCwndGrowth),
    	           MakeUintegerChecker<uint32_t> ())
    .AddAttribute("m_CwndGrowthFactor", "",
    	           UintegerValue(0),
    	           MakeUintegerAccessor(&TcpNewVegas::m_NvCwndGrowthFactor),
    	           MakeUintegerChecker<uint32_t> ())
    

    
  ;

	return tid;
}

TcpNewVegas::TcpNewVegas (void)
	: TcpNewReno (),
  	m_InitRtt (Time::Max ()),
	m_MinCwndNv (4),
	m_MinCwndGrow (2),
	m_TsoCwndBound (80),
	m_Pad (10),
	m_PadBuffer (2),
	m_ResetPeriod (5),
	m_MinCwnd (2),
	m_CongDecMult (30*128/100),
	m_SsThreshFactor (8),
	m_RttFactor (128),
	m_LossDecFactor (819),
	m_CwndGrowthRateNeg (8),  
	m_CwndGrowthRatePos (0),
	m_DecEvalMinCalls (60),
	m_IncEvalMinCalls (20),
	m_SsThreshEvalMinCalls (30),
	m_StopRttCnt (10),
	m_RttMinCnt (2),
	
	m_MinRttResetJiffies (),
    m_CwndGrowthFactor(0),
    m_NvAllowCwndGrowth (1),
    m_NvReset(0),
    m_NvCatchup(0),
    m_EvalCallCount(0),
    m_NvMinCwnd(m_MinCwndNv),
    m_RttCount(0),
    m_LastRtt(0),
	m_MinRtt (Time::Max()),
	m_MinRttNew (Time::Max()),
    m_BaseRtt(0),        
	m_LowerBoundRtt(0), 
	m_RttMaxRate(0),
	m_RttStartSeq(0),
	m_LastSndUna(0),
    m_NoCongCnt(0)

{
	NS_LOG_FUNCTION (this);
	NS_LOG_LOGIC ("Main init");	
}

TcpNewVegas::TcpNewVegas (const TcpNewVegas& sock)
	: TcpNewReno (sock),
  	m_InitRtt (Time::Max ()),
	m_MinCwndNv (sock.m_MinCwndNv),
	m_MinCwndGrow (sock.m_MinCwndGrow),
	m_TsoCwndBound (sock.m_TsoCwndBound),
	m_Pad (sock.m_Pad),
	m_PadBuffer (sock.m_PadBuffer),
	m_ResetPeriod (sock.m_ResetPeriod),
	m_MinCwnd (sock.m_MinCwnd),
	m_CongDecMult (sock.m_CongDecMult),
	m_SsThreshFactor (sock.m_SsThreshFactor),
	m_RttFactor (sock.m_RttFactor),
	m_LossDecFactor (sock.m_LossDecFactor),
	m_CwndGrowthRateNeg (sock.m_CwndGrowthRateNeg),  
	m_CwndGrowthRatePos (sock.m_CwndGrowthRatePos),
	m_DecEvalMinCalls (sock.m_DecEvalMinCalls),
	m_IncEvalMinCalls (sock.m_IncEvalMinCalls),
	m_SsThreshEvalMinCalls (sock.m_SsThreshEvalMinCalls),
	m_StopRttCnt (sock.m_StopRttCnt),
	m_RttMinCnt (sock.m_RttMinCnt),
	
	m_MinRttResetJiffies (),
    m_CwndGrowthFactor(sock.m_CwndGrowthFactor),
    m_NvAllowCwndGrowth (sock.m_NvAllowCwndGrowth),
    m_NvReset (sock.m_NvReset),
    m_NvCatchup (sock.m_NvCatchup),
    m_EvalCallCount(sock.m_EvalCallCount),
    m_NvMinCwnd(sock.m_NvMinCwnd),
    m_RttCount(sock.m_RttCount),
    m_LastRtt(sock.m_LastRtt),
	m_MinRtt (sock.m_MinRtt),
	m_MinRttNew (sock.m_MinRttNew),
    m_BaseRtt(sock.m_BaseRtt),        
	m_LowerBoundRtt(sock.m_LowerBoundRtt), 
	m_RttMaxRate(sock.m_RttMaxRate),
	m_RttStartSeq(sock.m_RttStartSeq),
	m_LastSndUna(sock.m_LastSndUna),
    m_NoCongCnt(sock.m_NoCongCnt)
{
	NS_LOG_FUNCTION (this);
}

TcpNewVegas::~TcpNewVegas (void)
{
	NS_LOG_FUNCTION (this);
}

Ptr<TcpCongestionOps>
TcpNewVegas::Fork (void)
{
  return CopyObject<TcpNewVegas> (this);
}

void
TcpNewVegas::TcpNewVegasReset(Ptr<TcpSocketState> tcb)
{
	NS_LOG_LOGIC ("Reset");
	m_NvReset = 0;
	m_NoCongCnt = 0;
	m_RttCount = 0;
	// m_LastRtt = Time::Min() ;
	m_LastRtt = Seconds(0);
	m_RttMaxRate = 0;
	m_RttStartSeq = tcb->m_lastAckedSeq;
	m_EvalCallCount = 0;
	m_LastSndUna = tcb->m_lastAckedSeq;
}


/* Do congestion avoidance calculations for TCP-NV
 */
void
TcpNewVegas::PktsAcked (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked,
                     const Time& rtt)
{
	NS_LOG_LOGIC ("Packets acked");
	NS_LOG_FUNCTION (this << tcb << segmentsAcked << rtt);
	uint64_t rate64;
	uint32_t rate, max_win, cwnd_by_slope;
	Time avg_rtt;
	SequenceNumber32 bytes_acked;
	

	/* Some calls are for duplicates without timetamps */
	if (rtt.IsZero ())
    {
      return;
    }



    uint32_t segCwnd = tcb->GetCwndInSegments ();

	/* If not in TCP_CA_Open or TCP_CA_Disorder states, skip. */
  	if(tcb->m_congState != TcpSocketState::CA_OPEN && 
              tcb->m_congState!=TcpSocketState::CA_DISORDER)
      return;

	/* Stop cwnd growth if we were in catch up mode */
	if (m_NvCatchup && segCwnd >= m_MinCwnd) {
		m_NvCatchup = false;
		m_NvAllowCwndGrowth = false;
	}

	bytes_acked =(tcb->m_lastAckedSeq - m_LastSndUna);
	uint32_t bytes = bytes_acked.GetValue();
	m_LastSndUna = tcb->m_lastAckedSeq;

	if (tcb->m_bytesInFlight.Get()==0)
		return;



	/* Calculate moving average of RTT */
	if (m_RttFactor > 0) {
		if (m_LastRtt > 0) {
			avg_rtt = ((rtt) * m_RttFactor + (m_LastRtt)*(256 - m_RttFactor)) / 256;
		} else {
			avg_rtt = rtt;
			m_MinRtt = avg_rtt * 2;
		}
		m_LastRtt = avg_rtt;
	} else {
		avg_rtt = rtt;
	}

	/* rate in 100's bits per second */
	rate64 = tcb->m_bytesInFlight.Get() * 80000;

	double tmp = 1.0 / avg_rtt.GetDouble ();

	if(avg_rtt != 0)
		rate = static_cast<uint32_t>(rate64 * tmp);
	else
		rate = rate64;

	/* Remember the maximum rate seen during this RTT
	 * Note: It may be more than one RTT. This function should be
	 *       called at least nv_dec_eval_min_calls times.
	 */

	if (m_RttMaxRate < rate)
		m_RttMaxRate = rate;

	/* We have valid information, increment counter */
	if (m_EvalCallCount < 255)
		m_EvalCallCount++;


    /* If provided, apply upper (base_rtt) and lower (lower_bound_rtt)
     * bounds to RTT.
     */



	if (m_LowerBoundRtt > 0 && avg_rtt < m_LowerBoundRtt)
		avg_rtt = m_LowerBoundRtt;
	else if (m_BaseRtt > 0 && avg_rtt > m_BaseRtt)
		avg_rtt = m_BaseRtt;
	
	/* update min rtt if necessary */
	if (avg_rtt < m_MinRtt)
		m_MinRtt = avg_rtt;

	/* update future min_rtt if necessary */
	if (avg_rtt < m_MinRttNew)
		m_MinRttNew = avg_rtt;

	/* Once per RTT check if we need to do congestion avoidance */	
	if (m_RttStartSeq < tcb->m_lastAckedSeq) {
		
		m_RttStartSeq = tcb->m_nextTxSequence;
		if (m_RttCount < 0xff)
		{	
			/* Increase counter for RTTs without CA decision */
			m_RttCount++;
		}

		/* If this function is only called once within an RTT
		 * the cwnd is probably too small (in some cases due to
		 * tso, lro or interrupt coalescence), so we increase
		 * ca->nv_min_cwnd.
		 */

		if (m_EvalCallCount == 1 &&
		    (bytes) >= (m_NvMinCwnd - 1) * tcb->m_segmentSize &&
		    m_NvMinCwnd < (m_TsoCwndBound + 1)) {
			
		     if(m_NvMinCwnd + m_MinCwndGrow < m_TsoCwndBound + 1)
		     	m_NvMinCwnd = m_NvMinCwnd + m_MinCwndGrow;
		     else
		     	m_NvMinCwnd = m_TsoCwndBound + 1;
            m_RttStartSeq = tcb->m_nextTxSequence + (m_NvMinCwnd * tcb->m_segmentSize);
			m_EvalCallCount = 0;
			m_NvAllowCwndGrowth = true;
			return;
		}

	    /* Find the ideal cwnd for current rate from slope
		 * slope = 80000.0 * mss / nv_min_rtt
		 * cwnd_by_slope = nv_rtt_max_rate / slope
		 */
        
        tmp = m_MinRtt.GetDouble();
		
		cwnd_by_slope = static_cast<uint32_t> (m_RttMaxRate * tmp) / (80000 * tcb->m_segmentSize);
		
		max_win = cwnd_by_slope + m_Pad;

		/* If cwnd > max_win, decrease cwnd
		 * if cwnd < max_win, grow cwnd
		 * else leave the same
		 */
		
		if (segCwnd > max_win) {
			/* there is congestion, check that it is ok
			 * to make a CA decision
			 * 1. We should have at least nv_dec_eval_min_calls
			 *    data points before making a CA  decision
			 * 2. We only make a congesion decision after
			 *    nv_rtt_min_cnt RTTs
			 */
			 if (m_RttCount < m_RttMinCnt) {
			 	return;
			 } 
			else if (tcb->m_ssThresh == tcb->m_initialSsThresh)
			 { 
			
				if (m_EvalCallCount <
				    m_SsThreshEvalMinCalls)
					return;
			 }
           	/* otherwise we will decrease cwnd */
			 else if (m_EvalCallCount <
				   m_DecEvalMinCalls) {

			
				if (m_NvAllowCwndGrowth &&
				    m_RttCount > m_StopRttCnt)
				{
					m_NvAllowCwndGrowth = false;
				}
				return;
			}
	
         	/* We have enough data to determine we are congested */
			m_NvAllowCwndGrowth = false;
			tcb->m_ssThresh = (m_SsThreshFactor * max_win) / 8;
			if (segCwnd - max_win > 2) {
				/* gap > 2, we do exponential cwnd decrease */
				uint32_t dec;
                if(((segCwnd - max_win)*m_CongDecMult) / 128 > 2 )
                {
                	dec = ((segCwnd - max_win)*m_CongDecMult) / 128;
                }
                else
                {	
                	dec = 2;
                }
                segCwnd = segCwnd - dec;
                tcb->m_cWnd = segCwnd * tcb->m_segmentSize;
			} else if (m_CongDecMult > 0) {
				
				segCwnd = max_win;
			}
			if (m_CwndGrowthFactor > 0)
				m_CwndGrowthFactor = 0;
			m_NoCongCnt = 0;
		} else if (segCwnd <= max_win - m_PadBuffer) {
				/* There is no congestion, grow cwnd if allowed*/
			if (m_EvalCallCount < m_IncEvalMinCalls)
				return;

			m_NvAllowCwndGrowth = true;
			m_NoCongCnt++;
			if (m_CwndGrowthFactor < 0 &&
			    m_CwndGrowthRateNeg > 0 &&
			    m_NoCongCnt > m_CwndGrowthRateNeg) {
				m_CwndGrowthFactor++;
				m_NoCongCnt = 0;
			} else if (m_CwndGrowthFactor >= 0 &&
				   m_CwndGrowthRatePos > 0 &&
				   m_NoCongCnt > m_CwndGrowthRatePos) {
				m_CwndGrowthFactor++;
				m_NoCongCnt = 0;
			}
		} else {
			/* cwnd is in-between, so do nothing */
			return;
		}

        	/* update state */
        m_EvalCallCount = 0;
		m_RttCount = 0;
		m_RttMaxRate = 0;

        /* Don't want to make cwnd < nv_min_cwnd
		 * (it wasn't before, if it is now is because nv
		 *  decreased it).
		 */
		if (segCwnd < m_MinCwnd)
			segCwnd = m_MinCwnd;
	}
}


void
TcpNewVegas::CongestionStateSet (Ptr<TcpSocketState> tcb,
                              const TcpSocketState::TcpCongState_t newState)
{
	// NS_LOG_LOGIC ("Congestion State Set");
	if (newState == TcpSocketState::CA_OPEN && m_NvReset)
	{
		TcpNewVegasReset(tcb);
	}
	else if (newState == TcpSocketState::CA_LOSS || newState == TcpSocketState::CA_RECOVERY || newState == TcpSocketState::CA_CWR || newState == TcpSocketState::CA_DISORDER)
	{
		m_NvReset = 1;
		m_NvAllowCwndGrowth = 0;
		if (newState == TcpSocketState::CA_LOSS)
		{

			if( m_CwndGrowthFactor > 0)
				m_CwndGrowthFactor = 0;

			if (m_CwndGrowthRateNeg>0 && m_CwndGrowthFactor>0)
			{
				m_CwndGrowthFactor--;
			}
		}
	}

}


void
TcpNewVegas::IncreaseWindow (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
	NS_LOG_FUNCTION (this << tcb << segmentsAcked);

    /* Only grow cwnd if NV has not detected congestion */

	if(!m_NvAllowCwndGrowth)
		return;

	if (tcb->m_cWnd < tcb->m_ssThresh)
	{
		segmentsAcked = TcpNewReno::SlowStart(tcb, segmentsAcked);		
		if(!segmentsAcked)
		{
			return;
		}
	}

	uint32_t cnt;
    	/* Reset cwnd growth factor to Reno value */
	if (m_CwndGrowthFactor < 0)
	{
		cnt = tcb->m_cWnd << -1*(m_CwndGrowthFactor);
		TcpNewReno::CongestionAvoidance(tcb, cnt);
	}
	else 	/* Decrease growth rate if allowed */
	{
		if (static_cast<uint32_t>	(4) > (tcb->m_cWnd >> m_CwndGrowthFactor))
		{
			cnt = 4;
		}
		else
		{
			cnt = (tcb->m_cWnd >> m_CwndGrowthFactor);
		}
		TcpNewReno::CongestionAvoidance(tcb, cnt);
	}


}

std::string
TcpNewVegas::GetName () const
{
  return "TcpNewVegas";
}

uint32_t
TcpNewVegas::GetSsThresh (Ptr<const TcpSocketState> tcb,
                       uint32_t bytesInFlight)
{
	NS_LOG_LOGIC ("SS Thresh");
	uint32_t segCwnd = tcb->GetCwndInSegments ();
	return (segCwnd*m_LossDecFactor)>>10;
}

}	// namespace ns3