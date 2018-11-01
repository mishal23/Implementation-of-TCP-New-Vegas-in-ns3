#ifndef TCPNEWVEGAS_H
#define TCPNEWVEGAS_H

#include "ns3/tcp-congestion-ops.h"
#include "ns3/tcp-recovery-ops.h"
#include "int64x64-128.h"

namespace ns3 {

class TcpNewVegas : public TcpNewReno
{
public:
	//
	TcpNewVegas (void);

	// Init (tcpnv_init)
	TcpNewVegas (const TcpNewVegas& sock);
	
	// 
	virtual ~TcpNewVegas (void);

	//
	virtual std::string GetName () const;

	// Hook for packet ack accounting (tcpnv_acked)
	virtual void PktsAcked (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked, const Time& rtt);

	// Call before changing ca_state (tcpnv_state)
	virtual void CongestionStateSet (Ptr<TcpSocketState> tcb, const TcpSocketState::TcpCongState_t newState);

	// New cwnd calculation	(tcpnv_cong_avoid)
	virtual void IncreaseWindow (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);

	// Returns the slow start threshold	(tcpnv_recalc_ssthresh)
	virtual uint32_t GetSsThresh (Ptr<const TcpSocketState> tcb, uint32_t bytesInFlight);

	virtual Ptr<TcpCongestionOps> Fork ();
protected:

private:

	// User Parameters
	uint32_t m_Pad;
	uint32_t m_PadBuffer;
	uint32_t m_ResetPeriod;
	uint32_t m_MinCwnd;
	uint32_t m_CongDecMult;
	uint32_t m_SsThreshFactor;
	uint32_t m_RttFactor;
	uint32_t m_LossDecFactor;
	uint32_t m_CwndGrowthRateNeg;
	uint32_t m_CwndGrowthRatePos;
	uint32_t m_DecEvalMinCalls;
	uint32_t m_IncEvalMinCalls;
	uint32_t m_SsThreshEvalMinCalls;
	uint32_t m_StopRttCnt;
	uint32_t m_RttMinCnt;

	// TcpNV Parameters
	// int64x64_t m_MinRttResetJiffies;
	
}
}
#endif // TCPNEWVEGAS_H