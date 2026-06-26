#pragma once

#include "Definitions.h"

#if STR_HAS_LORA

#include "Communicator.h"
#include "LORACore.h"
#include <array>
#include <cstdint>
#include <initializer_list>
#include <optional>

class LORACTR : public ICTR
{
public:
    LORACTR() : fCore(LORACore::GetInstance()) {}

    void Update() {}

    int Write(std::initializer_list<uint8_t> message) const {
        return fCore.Write(fPeerAddress, fCore.GetChannel(), message);
    }

    int Write() const {
        return fCore.Write(fPeerAddress, fCore.GetChannel());
    }

    void SetPeerAddress(uint16_t address) {
        fPeerAddress = address;
        fActivated = true;
    }

    uint16_t GetPeerAddress() const { return fPeerAddress; }

    // TODO: ajouter LORA dans ICTR::LinkType pour remplacer UNKNOWN
    uint16_t GetLinkInfoSizeImpl() const { return 3; }

    void WriteLinkInfoToBufferImpl(uint16_t index) const {
        messageBuffer[index]     = static_cast<uint8_t>(ICTR::LinkType::UNKNOWN);
        messageBuffer[index + 1] = static_cast<uint8_t>(fPeerAddress >> 8);
        messageBuffer[index + 2] = static_cast<uint8_t>(fPeerAddress & 0xFF);
    }

private:
    LORACore&   fCore;
    uint16_t    fPeerAddress = 0;
};

using OptLORACtrRef = std::optional<std::reference_wrapper<LORACTR>>;

inline std::array<LORACTR, NB_LORA_CTR> loraCtrArray;
inline uint8_t registeredLoRaCtr;

inline OptLORACtrRef GetLORACommunicatorByAddress(uint16_t address)
{
    for (auto& ctr : loraCtrArray)
    {
        if (!ctr)
            break;
        if (ctr.GetPeerAddress() == address)
            return ctr;
    }
    return std::nullopt;
}

#endif
