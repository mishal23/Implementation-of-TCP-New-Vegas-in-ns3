#ifndef TCPNEWVEGAS_H
#define TCPNEWVEGAS_H

#include "ns3/tcp-congestion-ops.h"
#include "ns3/tcp-recovery-ops.h"
// #include "int64x64-128.h"

namespace ns3 {

class TcpNewVegas : public TcpNewReno
{
public:
	//
	static TypeId GetTypeId (void);

	//
	TcpNewVegas (void);

	// Init (tcpnv_init)
	TcpNewVegas (const TcpNewVegas& sock);
	
	// 
	virtual ~TcpNewVegas (void);

	//
	virtual std::string GetName () const;

	virtual void TcpNewVegasReset(Ptr<TcpSocketState> tcb);

	virtual void TcpNewVegasInit(Ptr<TcpSocketState> tcb);

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

	// Macro Parameters
	Time m_InitRtt;
 	double m_MinCwndNv;
 	double m_MinCwndGrow;  
 	double m_TsoCwndBound; 

	// User Parameters
	uint32_t m_Pad;
	uint32_t m_PadBuffer;
	Time m_ResetPeriod;
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
	uint64_t m_MinRttResetJiffies;
	uint32_t m_CwndGrowthFactor;
	bool m_NvAllowCwndGrowth;
	bool m_NvReset;
	bool m_NvCatchup;
	uint8_t m_EvalCallCount;
	uint8_t m_NvMinCwnd;
	uint32_t m_RttCount;
    Time m_LastRtt;	
	Time m_MinRtt;		
	Time m_MinRttNew;	
	Time m_BaseRtt;        
	Time m_LowerBoundRtt; 
	uint32_t m_RttMaxRate;	
	SequenceNumber32 m_RttStartSeq;	
	SequenceNumber32 m_LastSndUna;
    uint32_t m_NoCongCnt; 
	
};
}
#endif // TCPNEWVEGAS_H