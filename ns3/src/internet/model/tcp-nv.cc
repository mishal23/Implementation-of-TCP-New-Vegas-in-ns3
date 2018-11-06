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
    .AddAttribute ("InitRtt", "Initial value of the RTT",
                   TimeValue (MilliSeconds(2)),
                   MakeTimeAccessor (&TcpNewVegas::m_InitRtt),
                   MakeTimeChecker ())
    .AddAttribute ("MinCwnd", "Minimum size of Congestion Window",
                   UintegerValue (4),
                   MakeUintegerAccessor (&TcpNewVegas::m_MinCwndNv),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("MinCwndGrow", "Growth of Minimum Cwnd",
                   UintegerValue (2),
                   MakeUintegerAccessor (&TcpNewVegas::m_MinCwndGrow),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("TsoCwndBound", "Cwnd bound by TSO",
                   UintegerValue (80),
                   MakeUintegerAccessor (&TcpNewVegas::m_TsoCwndBound),
                   MakeUintegerChecker<uint32_t> ())
    
  ;
return tid;


	return tid;
}

TcpNewVegas::TcpNewVegas (void)
	: TcpNewReno ()

{
	NS_LOG_FUNCTION (this);
}

TcpNewVegas::TcpNewVegas (const TcpNewVegas& sock)
	: TcpNewReno (sock)
{

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

}


void
TcpNewVegas::IncreaseWindow (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{

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
	// returning 1 for now,make sure to change
	return 1;
}

}	// namespace ns3