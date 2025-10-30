#include "Attacker.h"

void Attacker::initialize()
{
    attackCounter = 0;
    attackRate = par("attackRate");

    std::string attackTypeStr = par("attackType").stringValue();
    if (attackTypeStr == "dhcp_starvation") {
        attackType = DHCP_STARVATION;
    } else if (attackTypeStr == "dns_spoofing") {
        attackType = DNS_SPOOFING;
    } else if (attackTypeStr == "mac_spoofing") {
        attackType = MAC_SPOOFING;
    } else {
        attackType = DHCP_STARVATION;
    }

    // Schedule first attack
    attackEvent = new cMessage("ATTACK_EVENT");
    attackEvent->addPar("type") = ATTACK_EVENT;
    scheduleAt(par("startTime"), attackEvent);

    EV << "Attacker initialized - Type: " << attackTypeStr << "\n";
}

std::string Attacker::generateRandomMAC()
{
    char mac[18];
    sprintf(mac, "FF:FF:FF:FF:%02X:%02X",
            intuniform(0, 255), intuniform(0, 255));
    return std::string(mac);
}

void Attacker::launchDHCPStarvation()
{
    EV << "Launching DHCP Starvation Attack #" << attackCounter << "\n";

    // Send DHCP DISCOVER with fake MAC
    cMessage *attack = new cMessage("DHCP_DISCOVER");
    attack->addPar("type") = DHCP_DISCOVER;
    attack->addPar("clientMAC") = generateRandomMAC().c_str();
    attack->addPar("isAttack") = true;

    send(attack, "port$o");
    attackCounter++;
}

void Attacker::launchDNSSpoofing()
{
    EV << "Launching DNS Spoofing Attack #" << attackCounter << "\n";

    // Try to query for sensitive hostnames
    cMessage *attack = new cMessage("DNS_QUERY");
    attack->addPar("type") = DNS_QUERY;
    attack->addPar("hostname") = "admin";
    attack->addPar("isAttack") = true;

    send(attack, "port$o");
    attackCounter++;
}

void Attacker::launchMACSpoofing()
{
    EV << "Launching MAC Spoofing Attack #" << attackCounter << "\n";

    // Spoof legitimate client MAC
    cMessage *attack = new cMessage("DHCP_DISCOVER");
    attack->addPar("type") = DHCP_DISCOVER;
    attack->addPar("clientMAC") = "AA:BB:CC:DD:EE:01";  // Spoof client 1
    attack->addPar("isAttack") = true;

    send(attack, "port$o");
    attackCounter++;
}

void Attacker::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        // Launch attack based on type
        switch (attackType) {
            case DHCP_STARVATION:
                launchDHCPStarvation();
                break;
            case DNS_SPOOFING:
                launchDNSSpoofing();
                break;
            case MAC_SPOOFING:
                launchMACSpoofing();
                break;
        }

        // Schedule next attack
        scheduleAt(simTime() + exponential(1.0/attackRate), attackEvent);
    } else {
        // Received response from server (likely rejection)
        if (msg->hasPar("blocked") && msg->par("blocked").boolValue()) {
            emit(registerSignal("attackBlocked"), 1L);
            EV << "Attack blocked by server!\n";
        }
        delete msg;
    }
}

void Attacker::finish()
{
    if (attackEvent->isScheduled()) {
        cancelAndDelete(attackEvent);
    }
    EV << "=== Attacker Statistics ===\n";
    EV << "Total attacks launched: " << attackCounter << "\n";
}
