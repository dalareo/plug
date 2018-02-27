/*
 * PLUG - software to operate Fender Mustang amplifier
 *        Linux replacement for Fender FUSE software
 *
 * Copyright (C) 2017-2018  offa
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

#pragma once

#include "data_structs.h"
#include "effects_enum.h"
#include "MustangConstants.h"
#include <memory>

namespace plug
{
    class UsbComm;


    class Mustang
    {
    public:
        Mustang();
        ~Mustang();
        int start_amp(char list[][32] = nullptr, char* name = nullptr, amp_settings* amp_set = nullptr, fx_pedal_settings* effects_set = nullptr); // initialize communication
        void stop_amp();                                                                                                                           // terminate communication
        int set_effect(fx_pedal_settings);
        int set_amplifier(amp_settings);
        int save_on_amp(std::string_view name, std::uint8_t slot);
        int load_memory_bank(int, char* name = nullptr, amp_settings* amp_set = nullptr, fx_pedal_settings* effects_set = nullptr);
        int save_effects(int, char*, int, fx_pedal_settings*);

    private:
        std::unique_ptr<UsbComm> comm;
        unsigned char execute[LENGTH];       // "apply" command sent after each instruction
        unsigned char prev_array[4][LENGTH]; // array used to clear the effect

        int decode_data(unsigned char[7][LENGTH], char* name = nullptr, amp_settings* amp_set = nullptr, fx_pedal_settings* effects_set = nullptr);
    };
}
