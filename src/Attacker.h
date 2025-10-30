#ifndef __ATTACKER_H
#define __ATTACKER_H

#include <omnetpp.h>
#include <string>

using namespace omnetpp;

class Attacker : public cSimpleModule
{
private:
    enum AttackType {
        DHCP_STARVATION,
        DNS_SPOOFING,
        MAC_SPOOFING
    };

    enum MessageType {
        DHCP_DISCOVER = 1,
        DNS_QUERY = 5,
        ATTACK_EVENT = 20
    };

    AttackType attackType;
    double attackRate;
    int attackCounter;
    cMessage *attackEvent;

protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;

    // Attack methods
    void launchDHCPStarvation();
    void launchDNSSpoofing();
    void launchMACSpoofing();
    std::string generateRandomMAC();
};

Define_Module(Attacker);

#endif
