#include "SmartServer.h"
#include <sstream>
#include <algorithm>

void SmartServer::initialize()
{
    // Load configuration
    const char* poolRange = par("ipPool").stringValue();
    subnetMask = par("subnetMask").stringValue();
    gateway = par("gateway").stringValue();
    dnsServer = par("dnsServer").stringValue();
    leaseTime = par("leaseTime");

    // Initialize IP pool
    initializeIPPool(poolRange);

    // Parse friendly names
    const char* friendlyNamesStr = par("friendlyNames").stringValue();
    parseFriendlyNames(friendlyNamesStr);

    EV << "SmartServer initialized with " << availableIPs.size() << " available IPs\n";
    EV << "Friendly name mappings: " << friendlyNameMap.size() << "\n";
}

void SmartServer::initializeIPPool(const char* poolRange)
{
    std::string range(poolRange);
    size_t dashPos = range.find('-');

    if (dashPos == std::string::npos) return;

    std::string startIP = range.substr(0, dashPos);
    std::string endIP = range.substr(dashPos + 1);

    // Simple IP generation (assumes class C: 192.168.1.x)
    size_t lastDot = startIP.rfind('.');
    std::string prefix = startIP.substr(0, lastDot + 1);
    int startHost = std::stoi(startIP.substr(lastDot + 1));
    int endHost = std::stoi(endIP.substr(endIP.rfind('.') + 1));

    for (int i = startHost; i <= endHost; i++) {
        availableIPs.insert(prefix + std::to_string(i));
    }
}

void SmartServer::parseFriendlyNames(const char* mappings)
{
    if (strlen(mappings) == 0) return;

    std::string str(mappings);
    std::stringstream ss(str);
    std::string pair;

    while (std::getline(ss, pair, ',')) {
        // Trim whitespace
        pair.erase(0, pair.find_first_not_of(" \t"));
        pair.erase(pair.find_last_not_of(" \t") + 1);

        size_t eqPos = pair.find('=');
        if (eqPos != std::string::npos) {
            std::string mac = pair.substr(0, eqPos);
            std::string name = pair.substr(eqPos + 1);
            friendlyNameMap[mac] = name;
        }
    }
}

std::string SmartServer::getClientMAC(int gateIndex)
{
    // Generate simple MAC based on gate index
    char mac[18];
    sprintf(mac, "AA:BB:CC:DD:EE:%02d", gateIndex);
    return std::string(mac);
}

std::string SmartServer::generateHostname(const std::string& mac)
{
    // Check if there's a friendly name mapping
    auto it = friendlyNameMap.find(mac);
    if (it != friendlyNameMap.end()) {
        return it->second;
    }

    // Generate from MAC: AA:BB:CC:DD:EE:01 -> host-01
    size_t lastColon = mac.rfind(':');
    std::string lastByte = mac.substr(lastColon + 1);
    return "host-" + lastByte;
}

std::string SmartServer::allocateIP(const std::string& clientMAC)
{
    // Check if client already has an IP
    for (auto& lease : ipLeases) {
        if (lease.second.clientMAC == clientMAC) {
            return lease.first; // Return existing IP
        }
    }

    // Allocate new IP
    if (availableIPs.empty()) {
        return ""; // No IPs available
    }

    std::string ip = *availableIPs.begin();
    availableIPs.erase(availableIPs.begin());
    return ip;
}

void SmartServer::registerDNS(const std::string& hostname, const std::string& ip, simtime_t expiry)
{
    DNSRecord record;
    record.ipAddress = ip;
    record.expiry = expiry;
    dnsRecords[hostname] = record;

    emit(registerSignal("dnsRegistered"), 1L);
    EV << "DNS registered: " << hostname << " -> " << ip << " (expires at " << expiry << ")\n";
}

void SmartServer::releaseIP(const std::string& ip)
{
    auto it = ipLeases.find(ip);
    if (it != ipLeases.end()) {
        // Remove DNS entry
        std::string hostname = it->second.hostname;
        dnsRecords.erase(hostname);

        // Cancel timer
        if (leaseTimers.count(ip) > 0) {
            cancelAndDelete(leaseTimers[ip]);
            leaseTimers.erase(ip);
        }

        // Return IP to pool
        availableIPs.insert(ip);
        ipLeases.erase(it);

        EV << "Released IP " << ip << " and removed DNS entry for " << hostname << "\n";
    }
}

void SmartServer::handleDHCPDiscover(cMessage *msg)
{
    int gateIndex = msg->getArrivalGate()->getIndex();
    std::string clientMAC = getClientMAC(gateIndex);

    EV << "DHCP DISCOVER from " << clientMAC << "\n";

    // Allocate IP
    std::string offeredIP = allocateIP(clientMAC);

    if (offeredIP.empty()) {
        EV << "No IP available for " << clientMAC << "\n";
        delete msg;
        return;
    }

    // Send DHCP OFFER
    cMessage *offer = new cMessage("DHCP_OFFER");
    offer->addPar("type") = DHCP_OFFER;
    offer->addPar("offeredIP") = offeredIP.c_str();
    offer->addPar("subnetMask") = subnetMask.c_str();
    offer->addPar("gateway") = gateway.c_str();
    offer->addPar("dnsServer") = dnsServer.c_str();
    offer->addPar("leaseTime") = leaseTime;

    send(offer, "port$o", gateIndex);
    EV << "DHCP OFFER sent: " << offeredIP << " to " << clientMAC << "\n";

    delete msg;
}

void SmartServer::handleDHCPRequest(cMessage *msg)
{
    int gateIndex = msg->getArrivalGate()->getIndex();
    std::string clientMAC = getClientMAC(gateIndex);
    std::string requestedIP = msg->par("requestedIP").stringValue();
    std::string hostname = msg->par("hostname").stringValue();

    EV << "DHCP REQUEST from " << clientMAC << " for IP " << requestedIP << "\n";

    // If no hostname provided, generate one
    if (hostname.empty()) {
        hostname = generateHostname(clientMAC);
    }

    // Create lease
    IPLease lease;
    lease.clientMAC = clientMAC;
    lease.hostname = hostname;
    lease.leaseExpiry = simTime() + leaseTime;
    lease.gateIndex = gateIndex;
    ipLeases[requestedIP] = lease;

    // Register DNS
    registerDNS(hostname, requestedIP, lease.leaseExpiry);

    // Set lease expiration timer
    cMessage *expireMsg = new cMessage("LEASE_EXPIRE");
    expireMsg->addPar("ip") = requestedIP.c_str();
    leaseTimers[requestedIP] = expireMsg;
    scheduleAt(lease.leaseExpiry, expireMsg);

    // Send DHCP ACK
    cMessage *ack = new cMessage("DHCP_ACK");
    ack->addPar("type") = DHCP_ACK;
    ack->addPar("assignedIP") = requestedIP.c_str();
    ack->addPar("hostname") = hostname.c_str();
    ack->addPar("leaseTime") = leaseTime;

    send(ack, "port$o", gateIndex);

    emit(registerSignal("dhcpAssigned"), 1L);
    EV << "DHCP ACK sent: " << requestedIP << " assigned to " << hostname << " (" << clientMAC << ")\n";

    delete msg;
}

void SmartServer::handleDNSQuery(cMessage *msg)
{
    std::string queryHostname = msg->par("hostname").stringValue();
    int gateIndex = msg->getArrivalGate()->getIndex();

    EV << "DNS QUERY for " << queryHostname << "\n";

    // Lookup DNS record
    cMessage *response = new cMessage("DNS_RESPONSE");
    response->addPar("type") = DNS_RESPONSE;
    response->addPar("hostname") = queryHostname.c_str();

    auto it = dnsRecords.find(queryHostname);
    if (it != dnsRecords.end() && it->second.expiry > simTime()) {
        response->addPar("resolved") = true;
        response->addPar("ipAddress") = it->second.ipAddress.c_str();
        EV << "DNS RESPONSE: " << queryHostname << " -> " << it->second.ipAddress << "\n";
    } else {
        response->addPar("resolved") = false;
        response->addPar("ipAddress") = "";
        EV << "DNS RESPONSE: " << queryHostname << " not found\n";
    }

    send(response, "port$o", gateIndex);
    delete msg;
}

void SmartServer::handleLeaseExpire(cMessage *msg)
{
    std::string ip = msg->par("ip").stringValue();
    EV << "Lease expired for IP " << ip << "\n";

    releaseIP(ip);
    delete msg;
}

void SmartServer::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        handleLeaseExpire(msg);
        return;
    }

    int msgType = msg->par("type");

    switch (msgType) {
        case DHCP_DISCOVER:
            handleDHCPDiscover(msg);
            break;
        case DHCP_REQUEST:
            handleDHCPRequest(msg);
            break;
        case DNS_QUERY:
            handleDNSQuery(msg);
            break;
        default:
            delete msg;
            break;
    }
}

void SmartServer::finish()
{
    // Clean up timers
    for (auto& timer : leaseTimers) {
        cancelAndDelete(timer.second);
    }
    leaseTimers.clear();

    EV << "=== Server Statistics ===\n";
    EV << "Active leases: " << ipLeases.size() << "\n";
    EV << "DNS records: " << dnsRecords.size() << "\n";
    EV << "Available IPs: " << availableIPs.size() << "\n";
}
