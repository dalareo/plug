/*
 * PLUG - software to operate Fender Mustang amplifier
 *        Linux replacement for Fender FUSE software
 *
 * Copyright (C) 2017-2019  offa
 * Copyright (C) 2010-2016  piorekf <piorek@piorekf.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "com/Mustang.h"
#include "com/UsbComm.h"
#include "com/IdLookup.h"
#include "com/PacketSerializer.h"
#include "com/MustangConstants.h"
#include <algorithm>

namespace plug::com
{
    namespace
    {
        inline constexpr std::initializer_list<std::uint16_t> pids{
            SMALL_AMPS_USB_PID,
            BIG_AMPS_USB_PID,
            SMALL_AMPS_V2_USB_PID,
            BIG_AMPS_V2_USB_PID,
            MINI_USB_PID,
            FLOOR_USB_PID};


        inline constexpr std::uint8_t endpointSend{0x01};
        inline constexpr std::uint8_t endpointRecv{0x81};
    }

    Mustang::Mustang()
        : comm(std::make_unique<UsbComm>())
    {
    }

    Mustang::~Mustang()
    {
        this->stop_amp();
    }

    MemoryBank Mustang::start_amp(char list[][32])
    {
        if (comm->isOpen() == false)
        {
            comm->openFirst(USB_VID, pids);
        }

        initializeAmp();

        return loadInitialData(list);
    }

    void Mustang::stop_amp()
    {
        comm->close();
    }

    void Mustang::set_effect(fx_pedal_settings value)
    {
        const auto clearEffectPacket = serializeClearEffectSettings();
        sendCommand(clearEffectPacket);
        sendApplyCommand();

        if (value.effect_num != effects::EMPTY)
        {
            const auto settingsPacket = serializeEffectSettings(value);
            sendCommand(settingsPacket);
            sendApplyCommand();
        }
    }

    void Mustang::set_amplifier(amp_settings value)
    {
        const auto settingsPacket = serializeAmpSettings(value);
        sendCommand(settingsPacket);
        sendApplyCommand();

        const auto settingsGainPacket = serializeAmpSettingsUsbGain(value);
        sendCommand(settingsGainPacket);
        sendApplyCommand();
    }

    void Mustang::save_on_amp(std::string_view name, std::uint8_t slot)
    {
        const auto data = serializeName(slot, name);
        sendCommand(data);
        loadBankData(slot);
    }

    MemoryBank Mustang::load_memory_bank(std::uint8_t slot)
    {
        const auto data = loadBankData(slot);
        const auto name = decodeNameFromData(data);
        const auto amp = decodeAmpFromData(data);
        const auto effects = decodeEffectsFromData(data);

        return {name, amp, effects};
    }

    MemoryBank Mustang::decode_data(const std::array<Packet, 7>& data)
    {
        const std::string name = decodeNameFromData(data);
        const amp_settings amp = decodeAmpFromData(data);
        const std::array<fx_pedal_settings, 4> effects = decodeEffectsFromData(data);

        return {name, amp, effects};
    }

    void Mustang::save_effects(std::uint8_t slot, std::string_view name, const std::vector<fx_pedal_settings>& effects)
    {
        const auto saveNamePacket = serializeSaveEffectName(slot, name, effects);
        sendCommand(saveNamePacket);

        const auto packets = serializeSaveEffectPacket(slot, effects);
        std::for_each(packets.cbegin(), packets.cend(), [this](const auto& p) { sendCommand(p); });

        sendCommand(serializeApplyCommand(effects[0]));
    }

    MemoryBank Mustang::loadInitialData(char list[][32])
    {
        if (list != nullptr)
        {
            std::array<Packet, 296> recieved_data{{}};
            std::size_t i{0};
            std::size_t j{0};

            const auto loadCommand = serializeLoadCommand();
            auto recieved = sendPacket(loadCommand);

            for (i = 0; recieved != 0; i++)
            {
                const auto recvData = receivePacket();
                recieved = recvData.size();
                std::copy(recvData.cbegin(), recvData.cend(), recieved_data[i].begin());
            }

            const std::size_t max_to_receive = (i > 143 ? 200 : 48);

            if (list != nullptr)
            {
                for (i = 0, j = 0; i < max_to_receive; i += 2, ++j)
                {
                    std::copy_n(std::next(recieved_data[i].cbegin(), 16), 32, list[j]);
                }
            }

            std::array<Packet, 7> data{{}};

            for (i = max_to_receive, j = 0; j < 7; ++i, ++j)
            {
                std::copy(recieved_data[i].cbegin(), recieved_data[i].cend(), data[j].begin());
            }

            return decode_data(data);
        }

        return {};
    }

    std::array<Packet, 7> Mustang::loadBankData(std::uint8_t slot)
    {
        std::array<Packet, 7> data{{}};

        const auto loadCommand = serializeLoadSlotCommand(slot);
        auto n = sendPacket(loadCommand);

        for (std::size_t i = 0; n != 0; ++i)
        {
            const auto recvData = receivePacket();
            n = recvData.size();

            if (i < 7)
            {
                std::copy(recvData.cbegin(), recvData.cend(), data[i].begin());
            }
        }
        return data;
    }

    void Mustang::initializeAmp()
    {
        const auto packets = serializeInitCommand();
        std::for_each(packets.cbegin(), packets.cend(), [this](const auto& p) { sendCommand(p); });
    }

    std::size_t Mustang::sendPacket(const Packet& packet)
    {
        return comm->interruptWrite(endpointSend, packet);
    }

    std::vector<std::uint8_t> Mustang::receivePacket()
    {
        return comm->interruptReceive(endpointRecv, packetSize);
    }

    void Mustang::sendCommand(const Packet& packet)
    {
        sendPacket(packet);
        receivePacket();
    }

    void Mustang::sendApplyCommand()
    {
        sendCommand(serializeApplyCommand());
    }
}
