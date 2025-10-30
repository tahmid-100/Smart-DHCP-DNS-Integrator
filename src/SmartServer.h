#ifndef __SMARTSERVER_H
#define __SMARTSERVER_H

#include <omnetpp.h>
#include <map>
#include <string>
#include <set>

using namespace omnetpp;

class SmartServer : public cSimpleModule
{
private:
    // DHCP data structures
    struct IPLease {
        std::string clientMAC;
        std::string hostname;
        simtime_t leaseExpiry;
        int gateIndex;
    };

    // DNS data structure
    struct DNSRecord {
        std::string ipAddress;
        simtime_t expiry;
    };

    std::map<std::string, IPLease> ipLeases;  // IP -> Lease info
    std::set<std::string> availableIPs;        // Available IP pool
    std::map<std::string, DNSRecord> dnsRecords; // Hostname -> DNS record
    std::map<std::string, std::string> friendlyNameMap; // MAC -> friendly name

    // Security structures
    std::set<std::string> macWhitelist;  // Authorized MAC addresses
    std::map<std::string, int> requestCount;  // MAC -> request count
    std::map<std::string, simtime_t> lastRequestTime;  // MAC -> last request time
    std::set<std::string> blockedMACs;  // Blocked MAC addresses

    // Security parameters
    int maxRequestsPerMinute;
    bool enableSecurity;

    // Configuration
    std::string subnetMask;
    std::string gateway;
    std::string dnsServer;
    int leaseTime;

    // Message types
    enum MessageType {
        DHCP_DISCOVER = 1,
        DHCP_OFFER = 2,
        DHCP_REQUEST = 3,
        DHCP_ACK = 4,
        DNS_QUERY = 5,
        DNS_RESPONSE = 6,
        LEASE_EXPIRE = 7
    };

    // Self messages for lease expiration
    std::map<std::string, cMessage*> leaseTimers;

protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;

    // Helper methods
    void initializeIPPool(const char* poolRange);
    void parseFriendlyNames(const char* mappings);
    std::string allocateIP(const std::string& clientMAC);
    void handleDHCPDiscover(cMessage *msg);
    void handleDHCPRequest(cMessage *msg);
    void handleDNSQuery(cMessage *msg);
    void handleLeaseExpire(cMessage *msg);
    void registerDNS(const std::string& hostname, const std::string& ip, simtime_t expiry);
    void releaseIP(const std::string& ip);
    std::string generateHostname(const std::string& mac);
    std::string getClientMAC(int gateIndex);
};

Define_Module(SmartServer);

#endif
