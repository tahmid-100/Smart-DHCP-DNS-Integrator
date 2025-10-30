#ifndef __SMARTCLIENT_H
#define __SMARTCLIENT_H

#include <omnetpp.h>
#include <string>

using namespace omnetpp;

class SmartClient : public cSimpleModule
{
private:
    enum State {
        INIT,
        WAIT_OFFER,
        WAIT_ACK,
        BOUND,
        RENEWING
    };

    enum MessageType {
        DHCP_DISCOVER = 1,
        DHCP_OFFER = 2,
        DHCP_REQUEST = 3,
        DHCP_ACK = 4,
        DNS_QUERY = 5,
        DNS_RESPONSE = 6,
        START_DHCP = 10,
        RENEW_LEASE = 11,
        SEND_DNS_QUERY = 12
    };

    State state;
    std::string myIP;
    std::string myHostname;
    std::string offeredIP;
    int leaseTime;

    cMessage *startEvent;
    cMessage *renewEvent;
    cMessage *dnsQueryEvent;

protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;

    // DHCP methods
    void sendDHCPDiscover();
    void handleDHCPOffer(cMessage *msg);
    void sendDHCPRequest(const std::string& requestIP);
    void handleDHCPAck(cMessage *msg);

    // DNS methods
    void sendDNSQuery(const std::string& hostname);
    void handleDNSResponse(cMessage *msg);

    // Helper
    std::string getMyMAC();
};

Define_Module(SmartClient);

#endif
