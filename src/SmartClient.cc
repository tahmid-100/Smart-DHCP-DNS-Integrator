#include "SmartClient.h"

void SmartClient::initialize()
{
    state = INIT;
    leaseTime = par("leaseTime");
    myHostname = par("hostname").stringValue();

    // Schedule DHCP start
    startEvent = new cMessage("START_DHCP");
    startEvent->addPar("type") = START_DHCP;
    scheduleAt(par("startTime"), startEvent);

    renewEvent = nullptr;
    dnsQueryEvent = nullptr;

    EV << "Client initialized, will start DHCP at " << par("startTime").doubleValue() << "s\n";
}

std::string SmartClient::getMyMAC()
{
    // Generate MAC based on module index
    int index = getIndex();
    char mac[18];
    sprintf(mac, "AA:BB:CC:DD:EE:%02d", index + 1);
    return std::string(mac);
}

void SmartClient::sendDHCPDiscover()
{
    EV << "Sending DHCP DISCOVER\n";

    cMessage *discover = new cMessage("DHCP_DISCOVER");
    discover->addPar("type") = DHCP_DISCOVER;
    discover->addPar("clientMAC") = getMyMAC().c_str();

    send(discover, "port$o");
    state = WAIT_OFFER;
}

void SmartClient::handleDHCPOffer(cMessage *msg)
{
    offeredIP = msg->par("offeredIP").stringValue();
    EV << "Received DHCP OFFER: " << offeredIP << "\n";

    // Send DHCP REQUEST
    sendDHCPRequest(offeredIP);
    state = WAIT_ACK;
}

void SmartClient::sendDHCPRequest(const std::string& requestIP)
{
    EV << "Sending DHCP REQUEST for " << requestIP << "\n";

    cMessage *request = new cMessage("DHCP_REQUEST");
    request->addPar("type") = DHCP_REQUEST;
    request->addPar("requestedIP") = requestIP.c_str();
    request->addPar("hostname") = myHostname.c_str();
    request->addPar("clientMAC") = getMyMAC().c_str();

    send(request, "port$o");
}

void SmartClient::handleDHCPAck(cMessage *msg)
{
    myIP = msg->par("assignedIP").stringValue();
    myHostname = msg->par("hostname").stringValue();
    leaseTime = msg->par("leaseTime");

    state = BOUND;

    emit(registerSignal("ipAssigned"), 1L);
    EV << "IP assigned: " << myIP << " with hostname " << myHostname << "\n";
    EV << "Lease time: " << leaseTime << "s\n";

    // Schedule lease renewal (at 50% of lease time)
    renewEvent = new cMessage("RENEW_LEASE");
    renewEvent->addPar("type") = RENEW_LEASE;
    scheduleAt(simTime() + (leaseTime * 0.5), renewEvent);

    // Schedule a DNS query to test the system (query for another host)
    if (getIndex() == 0) {  // Only first client does DNS queries
        dnsQueryEvent = new cMessage("SEND_DNS_QUERY");
        dnsQueryEvent->addPar("type") = SEND_DNS_QUERY;
        scheduleAt(simTime() + 10, dnsQueryEvent);  // Query after 10s
    }
}

void SmartClient::sendDNSQuery(const std::string& hostname)
{
    EV << "Sending DNS QUERY for " << hostname << "\n";

    cMessage *query = new cMessage("DNS_QUERY");
    query->addPar("type") = DNS_QUERY;
    query->addPar("hostname") = hostname.c_str();

    send(query, "port$o");

    emit(registerSignal("dnsQuerySent"), 1L);
}

void SmartClient::handleDNSResponse(cMessage *msg)
{
    std::string hostname = msg->par("hostname").stringValue();
    bool resolved = msg->par("resolved");

    if (resolved) {
        std::string ipAddress = msg->par("ipAddress").stringValue();
        EV << "DNS RESPONSE: " << hostname << " -> " << ipAddress << "\n";
    } else {
        EV << "DNS RESPONSE: " << hostname << " not found\n";
    }
}

void SmartClient::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        int msgType = msg->par("type");

        if (msgType == START_DHCP) {
            sendDHCPDiscover();
        } else if (msgType == RENEW_LEASE) {
            EV << "Renewing lease...\n";
            sendDHCPRequest(myIP);
            state = WAIT_ACK;
        } else if (msgType == SEND_DNS_QUERY) {
            // Query for different hosts based on index
            if (getIndex() == 0) {
                sendDNSQuery("host-02");  // Query for second client

                // Schedule another query
                dnsQueryEvent = new cMessage("SEND_DNS_QUERY");
                dnsQueryEvent->addPar("type") = SEND_DNS_QUERY;
                scheduleAt(simTime() + 20, dnsQueryEvent);
            }
        }
        return;
    }

    int msgType = msg->par("type");

    switch (msgType) {
        case DHCP_OFFER:
            if (state == WAIT_OFFER) {
                handleDHCPOffer(msg);
            }
            break;
        case DHCP_ACK:
            if (state == WAIT_ACK) {
                handleDHCPAck(msg);
            }
            break;
        case DNS_RESPONSE:
            handleDNSResponse(msg);
            break;
        default:
            break;
    }

    delete msg;
}

void SmartClient::finish()
{
    if (renewEvent != nullptr && renewEvent->isScheduled()) {
        cancelAndDelete(renewEvent);
    }
    if (dnsQueryEvent != nullptr && dnsQueryEvent->isScheduled()) {
        cancelAndDelete(dnsQueryEvent);
    }

    EV << "=== Client " << getIndex() << " Statistics ===\n";
    EV << "Final IP: " << myIP << "\n";
    EV << "Hostname: " << myHostname << "\n";
}
