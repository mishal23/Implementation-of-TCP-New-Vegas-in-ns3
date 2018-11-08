#include "tcp-nv.h"
#include "ns3/log.h"
#include <bits/stdc++.h>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TcpNewVegas");
// NS_OBJECT_ENSURE_REGISTERED (TcpNewVegas);

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
	m_RttStartSeq(),
	m_LastSndUna(),
    m_NoCongCnt(0)

{
	NS_LOG_FUNCTION (this);
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
	m_RttStartSeq(),
	m_LastSndUna(),
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
	m_NvReset = 0;
	m_NoCongCnt = 0;
	m_RttCount = 0;
	m_LastRtt = Time::Min() ;
	m_RttMaxRate = 0;
	m_RttStartSeq = tcb->m_lastAckedSeq;
	m_EvalCallCount = 0;
	m_LastSndUna = tcb->m_lastAckedSeq;
}

void
TcpNewVegas::TcpNewVegasInit(Ptr<TcpSocketState> tcb)
{
	NS_LOG_FUNCTION (this << tcb);

	TcpNewVegasReset(tcb);

	m_NvAllowCwndGrowth = 1;
	//m_MinRttResetJiffies = 0;
	m_MinRtt = m_InitRtt;
	m_MinRttNew = m_InitRtt;
	m_MinCwnd = m_MinCwndNv;
	m_NvCatchup = 0;
	m_CwndGrowthFactor = 0;
}

void
TcpNewVegas::PktsAcked (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked, uint32_t bytesInFlight,
                     const Time& rtt)
{


	
    NS_LOG_FUNCTION (this << tcb << segmentsAcked << rtt << bytesInFlight);
	uint64_t rate64;
	uint32_t rate, max_win, cwnd_by_slope;
	Time avg_rtt;
	SequenceNumber32 bytes_acked;

	
	if (rtt.IsZero ())
    {
      return;
    }

    uint32_t segCwnd = tcb->GetCwndInSegments ();

    //I don't know how to do it
	/* If not in TCP_CA_Open or TCP_CA_Disorder states, skip. */
	/*if (icsk->icsk_ca_state != TCP_CA_Open &&
	    icsk->icsk_ca_state != TCP_CA_Disorder)
		return;*/

  if(tcb->m_congState != TcpSocketState::CA_OPEN && 
              tcb->m_congState!=TcpSocketState::CA_DISORDER)
      return;

	
	if (m_NvCatchup && segCwnd >= m_MinCwnd) {
		m_NvCatchup = false;
		m_NvAllowCwndGrowth = false;
	}

	bytes_acked =(tcb->m_lastAckedSeq - m_LastSndUna);
	uint32_t bytes = bytes_acked.GetValue();
	m_LastSndUna = tcb->m_lastAckedSeq;

	if (bytesInFlight==0)
		return;


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


	rate64 = bytesInFlight * 80000;

	double tmp = 1.0 / avg_rtt.GetDouble ();

	if(avg_rtt > 0)
		rate = static_cast<uint32_t>(rate64 * tmp);
	else
		rate = rate64;

	

	if (m_RttMaxRate < rate)
		m_RttMaxRate = rate;


	if (m_EvalCallCount < 255)
		m_EvalCallCount++;


	

	if (m_LowerBoundRtt > 0 && avg_rtt < m_LowerBoundRtt)
		avg_rtt = m_LowerBoundRtt;
	else if (m_BaseRtt > 0 && avg_rtt > m_BaseRtt)
		avg_rtt = m_BaseRtt;
	

	if (avg_rtt < m_MinRtt)
		m_MinRtt = avg_rtt;


	if (avg_rtt < m_MinRttNew)
		m_MinRttNew = avg_rtt;

	
	if (m_RttStartSeq >= tcb->m_lastAckedSeq) {
		m_RttStartSeq = tcb->m_nextTxSequence;
		if (m_RttCount < 0xff)
			m_RttCount++;

		if (m_EvalCallCount == 1 &&
		    (bytes) >= (m_NvMinCwnd - 1) * tcb->m_segmentSize &&
		    m_NvMinCwnd < (m_TsoCwndBound + 1)) {
			
		     if(m_NvMinCwnd + m_MinCwndGrow < m_TsoCwndBound + 1)
		     	m_NvMinCwnd = m_NvMinCwnd + m_MinCwndGrow;
		     else
		     	m_NvMinCwnd = m_TsoCwndBound + 1;
            m_RttStartSeq = tcb->m_nextTxSequence +(m_NvMinCwnd * tcb->m_segmentSize);
			m_EvalCallCount = 0;
			m_NvAllowCwndGrowth = true;
			return;
		}


	
        tmp = m_MinRtt.GetDouble();
		cwnd_by_slope = static_cast<uint32_t> (m_RttMaxRate * tmp) / (80000 * tcb->m_segmentSize);
		
		max_win = cwnd_by_slope + m_Pad;
      
		if (segCwnd > max_win) {
			if (m_RttCount < m_RttMinCnt) {
				return;
			} 
			else if (tcb->m_ssThresh == tcb->m_initialSsThresh)
			    { 
				if (m_EvalCallCount <
				    m_SsThreshEvalMinCalls)
					return;
			    }

			 else if (m_EvalCallCount <
				   m_DecEvalMinCalls) {
				if (m_NvAllowCwndGrowth &&
				    m_RttCount > m_StopRttCnt)
					m_NvAllowCwndGrowth = false;
				return;
			}
		
	

			m_NvAllowCwndGrowth = false;
			tcb->m_ssThresh = (m_SsThreshFactor * max_win) / 8;
			if (segCwnd - max_win > 2) {
				uint32_t dec;
                if(((segCwnd - max_win)*m_CongDecMult) / 128 > 2 )
                	dec = ((segCwnd - max_win)*m_CongDecMult) / 128;
                else
                	dec = 2;
                segCwnd -= dec;
			} else if (m_CongDecMult > 0) {
				segCwnd = max_win;
			}
			if (m_CwndGrowthFactor > 0)
				m_CwndGrowthFactor = 0;
			m_NoCongCnt = 0;
		} else if (segCwnd <= max_win - m_PadBuffer) {
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
			return;
		}

        
        m_EvalCallCount = 0;
		m_RttCount = 0;
		m_RttMaxRate = 0;

		if (segCwnd < m_MinCwnd)
			segCwnd = m_MinCwnd;
	}
	



}

void
TcpNewVegas::CongestionStateSet (Ptr<TcpSocketState> tcb,
                              const TcpSocketState::TcpCongState_t newState)
{
	if (newState == TcpSocketState::CA_OPEN && m_NvReset)
	{
		// tcp reset
	}
	else if (newState == TcpSocketState::CA_LOSS || newState == TcpSocketState::CA_RECOVERY || newState == TcpSocketState::CA_CWR)
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

	// if (tcb->m_cWnd < tcb->m_ssThresh)
 //    {
 //      tcb->m_cWnd += tcb->m_segmentSize;
 //      segmentsAcked -= 1;

 //      NS_LOG_INFO ("In SlowStart, updated to cwnd " << tcb->m_cWnd <<
 //                   " ssthresh " << tcb->m_ssThresh);
 //    }

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

	if (m_CwndGrowthFactor < 0)
	{
		cnt = tcb->m_cWnd << -1*(m_CwndGrowthFactor);
		TcpNewReno::CongestionAvoidance(tcb, cnt);
	}
	else 
	{
		// cnt = std::max(static_cast<uint32_t>(4), (tcb->m_cWnd >> m_CwndGrowthFactor));
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
	uint32_t segCwnd = tcb->GetCwndInSegments ();
	return (segCwnd*m_LossDecFactor)>>10;
}

}	// namespace ns3