#include "Slave.h"
#include <variant>

std::vector<Slave*> slaves;
std::queue<ICTR_t> newSlavesCTR;
std::queue<Slave*> slavesWaitingForID;
ICTR_t masterCTR;


Slave::Slave(auto ctr)
{
    fCTR = ctr;
}

ICTR* Slave::GetSlaveCTR(uint8_t slaveID)
{
    for (const auto& slave : slaves)
    {
        if (slave->fSlaveID == slaveID || slave->HasSubSlave(slaveID))
            return std::visit([](auto&& ctr) -> ICTR* {

					using T = std::decay_t<decltype(ctr)>;

					if constexpr (!std::is_same_v<T, std::monostate>)
						return &ctr;

					else
						return nullptr;

				}, slave->fCTR);
    }

    return nullptr;
}

ICTR* Slave::GetCTR()
{
    return std::visit([](auto&& ctr) -> ICTR* {

			using T = std::decay_t<decltype(ctr)>;

			if constexpr (std::is_same_v<T, std::monostate>)
				return nullptr;

			else
				return &ctr;

		}, fCTR);
}

uint8_t Slave::GetID()
{
    return fSlaveID;
}

bool Slave::HasSubSlave(uint8_t id)
{
    bool found = false;

    for (auto i = fSubSlave.begin(); i != fSubSlave.end(); i++)
    {
        if (*i == id)
        {
            found = true;
            break;
        }
    }

    return found;
}

void Slave::AddSubSlave(uint8_t id)
{
    fSubSlave.push_back(id);
}

void Slave::SetID(uint8_t id)
{
    fSlaveID = id;
}

uint16_t Slave::GetLinkInfoSize() const
{
		return std::visit([](auto&& ctr) -> uint16_t {

				using T = std::decay_t<decltype(ctr)>;

				if constexpr (!std::is_same_v<T, std::monostate>)
					return ctr.GetLinkInfoSize() + 2;

				else
					return 0;
			}, fCTR);
}

void Slave::WriteLinkInfoToBuffer(uint8_t* msgBuffer) const
{
	if (fCTR.index())
	{
		msgBuffer[0] = GetLinkInfoSize();
		msgBuffer[1] = fSlaveID;

		std::visit([msgBuffer](auto&& ctr) {

				using T = std::decay_t<decltype(ctr)>;

				if constexpr (!std::is_same_v<T, std::monostate>)
					ctr.WriteLinkInfoToBuffer(msgBuffer + 2);
			
			}, fCTR);
	}
}
