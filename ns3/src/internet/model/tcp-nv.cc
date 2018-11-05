#include "tcp-nv.h"
#include "ns3/log.h"

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
TcpNewVegas::PktsAcked (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked,
                     const Time& rtt)
{

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