/*
 * CS 169 W25 - Project 1 Template
 * Performance of TCP in LTE Handover
 * Base on lena-x2-handover-measures.cc
 * ./ns3 run scratch/lte_project_template.cc 
 * ex: ./ns3 run scratch/lte_project_template.cc -- --numberOfEnbs=1 --distance=500 --simNumber=2
 * to open PCAP file = tshark -r <filename>.pcap | head -n 1

 changes:
 added check for simulation number if and else
 added handover time function at the notification ue handover function
 added check for simulation time since divided by zero can't happen
 created bash script to execute file 
 add pcap command using remote host node

 tips:
 to get out of env type "deactivate"

 */

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/lte-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include <string>
#include <fstream>
#include <vector>
#include <iostream>
#include <typeinfo>

using namespace ns3;
std::vector<double> handovers;

NS_LOG_COMPONENT_DEFINE("project-1");

void
NotifyHandoverStartUe(std::string context,
                      uint64_t imsi,
                      uint16_t cellid,
                      uint16_t rnti,
                      uint16_t targetCellId)
{
    // std::cout << "UE starting Handover" << std::endl;
    std::cout << "Found Handover: " << Simulator::Now().GetSeconds() << std::endl;
    handovers.push_back(Simulator::Now().GetSeconds());
}

/**
 * Sample simulation script for an automatic X2-based handover based on the RSRQ measures.
 * It instantiates two eNodeB, attaches one UE to the 'source' eNB.
 * The UE moves between both eNBs, it reports measures to the serving eNB and
 * the 'source' (serving) eNB triggers the handover of the UE towards
 * the 'target' eNB when it considers it is a better eNB.
 */

int
main(int argc, char* argv[])
{
    uint16_t numberOfUes = 1;
    uint16_t numberOfEnbs = 2;
    double distance = 500.0;                                        // m
    double speed = 35;                                              // m/s
    double simTime;
    double enbTxPowerDbm = 5;
    double simPart = 1;
    uint16_t simNumber = 1;
    

    Config::SetDefault("ns3::LteHelper::UseIdealRrc", BooleanValue(true));

    // Command line arguments
    CommandLine cmd(__FILE__);
    cmd.AddValue("simTime", "Total duration of the simulation (in seconds)", simTime);
    cmd.AddValue("speed", "Speed of the UE", speed);
    cmd.AddValue("numberOfEnbs", "number of Enbs", numberOfEnbs);
    cmd.AddValue("simNumber", "Simulation Number", simNumber);
    cmd.AddValue("distance", "distance of simulation", distance);
    cmd.AddValue("simPart", "simulation part", simPart);



    cmd.Parse(argc, argv);

    std::string filename = "./results/handoverSim.txt";
    std::ofstream myFile(filename);

    if(speed == 0)
    {
        simTime = 40;
    }
    else if(simPart == 1)
    {
        simTime = (double)(numberOfEnbs + 1) * distance / speed;
    }
    else if(simPart == 2)
    {
        simTime = 20;
    }
 


    Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();
    Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper>();
    lteHelper->SetEpcHelper(epcHelper);
    lteHelper->SetSchedulerType("ns3::RrFfMacScheduler");

    lteHelper->SetHandoverAlgorithmType("ns3::A2A4RsrqHandoverAlgorithm");
    lteHelper->SetHandoverAlgorithmAttribute("ServingCellThreshold", UintegerValue(30));
    lteHelper->SetHandoverAlgorithmAttribute("NeighbourCellOffset", UintegerValue(1));


    Ptr<Node> pgw = epcHelper->GetPgwNode();

    // Create a single RemoteHost
    NodeContainer remoteHostContainer;
    remoteHostContainer.Create(1);
    Ptr<Node> remoteHost = remoteHostContainer.Get(0);
    InternetStackHelper internet;
    internet.Install(remoteHostContainer);

    // Create the Internet
    PointToPointHelper p2ph;
    p2ph.SetDeviceAttribute("DataRate", DataRateValue(DataRate("100Gb/s")));
    p2ph.SetDeviceAttribute("Mtu", UintegerValue(1500));
    p2ph.SetChannelAttribute("Delay", TimeValue(Seconds(0.010)));
    NetDeviceContainer internetDevices = p2ph.Install(pgw, remoteHost);
    Ipv4AddressHelper ipv4h;
    ipv4h.SetBase("1.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign(internetDevices);
    // Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress(1);

    // Routing of the Internet Host (towards the LTE network)
    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    Ptr<Ipv4StaticRouting> remoteHostStaticRouting =
        ipv4RoutingHelper.GetStaticRouting(remoteHost->GetObject<Ipv4>());
    // interface 0 is localhost, 1 is the p2p device
    remoteHostStaticRouting->AddNetworkRouteTo(Ipv4Address("7.0.0.0"), Ipv4Mask("255.0.0.0"), 1);

    NodeContainer ueNodes;
    NodeContainer enbNodes;
    enbNodes.Create(numberOfEnbs);
    ueNodes.Create(numberOfUes);

    // Install Mobility Model in eNB

    Ptr<ListPositionAllocator> enbPositionAlloc = CreateObject<ListPositionAllocator>();
    if(simPart == 1)
    {
        for (uint16_t i = 0; i < numberOfEnbs; i++)
        {
            Vector enbPosition(distance * (i + 1), 0, 0);
            enbPositionAlloc->Add(enbPosition);
        }
    } else if(simPart == 2)
    {
        double divided = distance / (numberOfEnbs + 1);
        for (uint16_t i = 0; i < numberOfEnbs; i++)
        {
            Vector enbPosition(divided * (i + 1), 0, 0);
            enbPositionAlloc->Add(enbPosition);
            std::cout << "Distance " << i << " = " << divided * (i+1) << std::endl;
        }
    } else
    {
        std::cout << "Invalid Number" << std::endl;
        return 0;
    }
    MobilityHelper enbMobility;
    enbMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    enbMobility.SetPositionAllocator(enbPositionAlloc);
    enbMobility.Install(enbNodes);

    // Install Mobility Model in UE
    MobilityHelper ueMobility;
    ueMobility.SetMobilityModel("ns3::ConstantVelocityMobilityModel");
    ueMobility.Install(ueNodes);
    ueNodes.Get(0)->GetObject<MobilityModel>()->SetPosition(Vector(0, distance, 0));
    ueNodes.Get(0)->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(Vector(speed, 0, 0));

    // Install LTE Devices in eNB and UEs
    Config::SetDefault("ns3::LteEnbPhy::TxPower", DoubleValue(enbTxPowerDbm));
    NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice(enbNodes);
    NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice(ueNodes);

    // Install the IP stack on the UEs
    internet.Install(ueNodes);
    Ipv4InterfaceContainer ueIpIfaces;
    ueIpIfaces = epcHelper->AssignUeIpv4Address(NetDeviceContainer(ueLteDevs));

    // Attach all UEs to the first eNodeB
    for (uint16_t i = 0; i < numberOfUes; i++)
    {
        lteHelper->Attach(ueLteDevs.Get(i), enbLteDevs.Get(0));
    }

    // Install and start applications on UEs and remote host
    uint16_t dlPort = 10000;

    // randomize a bit start times to avoid simulation artifacts
    // (e.g., buffer overflows due to packet transmissions happening
    // exactly at the same time)
    Ptr<UniformRandomVariable> startTimeSeconds = CreateObject<UniformRandomVariable>();
    startTimeSeconds->SetAttribute("Min", DoubleValue(0));
    startTimeSeconds->SetAttribute("Max", DoubleValue(0.010));

    Ptr<Node> ue = ueNodes.Get(0);
    // Set the default gateway for the UE
    Ptr<Ipv4StaticRouting> ueStaticRouting =
        ipv4RoutingHelper.GetStaticRouting(ue->GetObject<Ipv4>());
    ueStaticRouting->SetDefaultRoute(epcHelper->GetUeDefaultGatewayAddress(), 1);

    ApplicationContainer clientApps;
    ApplicationContainer serverApps;

    // TCP sender at remote host, receiver at UE
    Address dlSinkAddress(InetSocketAddress(ueIpIfaces.GetAddress(0), dlPort));
    PacketSinkHelper dlPacketSinkHelper("ns3::TcpSocketFactory", dlSinkAddress);
    serverApps.Add(dlPacketSinkHelper.Install(ue));

    BulkSendHelper dlClientHelper("ns3::TcpSocketFactory",
                                  InetSocketAddress(ueIpIfaces.GetAddress(0), dlPort));
    dlClientHelper.SetAttribute("MaxBytes", UintegerValue(0)); // Keep sending unstopped
    clientApps.Add(dlClientHelper.Install(remoteHost));

    Ptr<EpcTft> tft = Create<EpcTft>();

    EpcTft::PacketFilter dlpf;
    dlpf.localPortStart = dlPort;
    dlpf.localPortEnd = dlPort;
    tft->Add(dlpf);

    EpsBearer bearer(EpsBearer::NGBR_VIDEO_TCP_DEFAULT);
    lteHelper->ActivateDedicatedEpsBearer(ueLteDevs.Get(0), bearer, tft);

    Time startTime = Seconds(startTimeSeconds->GetValue());
    serverApps.Start(startTime);
    clientApps.Start(startTime);

    // Add X2 interface
    lteHelper->AddX2Interface(enbNodes);

    lteHelper->EnablePhyTraces();
    // lteHelper->EnableMacTraces();
    // lteHelper->EnableRlcTraces();
    // lteHelper->EnablePdcpTraces();
    // Ptr<RadioBearerStatsCalculator> rlcStats = lteHelper->GetRlcStats();
    // rlcStats->SetAttribute("EpochDuration", TimeValue(Seconds(1.0)));
    // Ptr<RadioBearerStatsCalculator> pdcpStats = lteHelper->GetPdcpStats();
    // pdcpStats->SetAttribute("EpochDuration", TimeValue(Seconds(1.0)));

    // connect custom trace sinks for RRC connection establishment and handover notification
    Config::Connect("/NodeList/*/DeviceList/*/LteUeRrc/HandoverStart",
                    MakeCallback(&NotifyHandoverStartUe));

    Simulator::Stop(Seconds(simTime));
    internet.EnablePcapIpv4("capture", remoteHostContainer);




    Simulator::Run();
    Simulator::Destroy();
    std::cout << "Size of vector: " << handovers.size() << std::endl;
    for(long unsigned int i = 0; i < handovers.size(); ++i)
    {
        myFile << handovers[i] << std::endl;
    }
    myFile.close();
    
    return 0;
}


