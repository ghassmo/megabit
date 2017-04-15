/*
 * This file is part of Megabit, a BIP44 HD wallet built on
 * libbitcoin.
 *
 * Copyright (C) 2017 Neill Miller (neillm@thecodefactory.org)
 *
 * Megabit is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Megabit is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Megabit.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __CONSTANTS_HPP
#define __CONSTANTS_HPP

#include <bitcoin/bitcoin.hpp>

namespace megabit {
namespace constants {

static constexpr size_t checksum_length = 4;
static constexpr size_t entropy_length = 32;

static constexpr uint8_t num_retries = 0;
static constexpr uint16_t timeout_seconds = 10;

static constexpr uint32_t locktime = 0;
static constexpr uint32_t script_version = 5;
static constexpr uint32_t transaction_version = 1;
static constexpr auto sighash_type =
    libbitcoin::machine::sighash_algorithm::all;

static constexpr size_t max_date_string_length = 16;

static constexpr uint32_t bip32_mainnet_public_version = 0x0488B21E;
static constexpr uint32_t bip32_mainnet_private_version = 0x0488ADE4;

static constexpr uint32_t bip32_testnet_public_version = 0x043587CF;
static constexpr uint32_t bip32_testnet_private_version = 0x04358394;

static constexpr int64_t max_int64 = std::numeric_limits<int64_t>::max();
static constexpr uint64_t max_uint64 = std::numeric_limits<uint64_t>::max();

static constexpr uint64_t satoshi_per_btc = 100000000;

static constexpr size_t num_confirmations = 6;

static constexpr size_t unspent_height = std::numeric_limits<size_t>::max();
static constexpr uint32_t unspent_index = std::numeric_limits<uint32_t>::max();

// bip44: https://github.com/bitcoin/bips/blob/master/bip-0044.mediawiki
static constexpr size_t bip44_gap_limit = 20;
static constexpr uint32_t bip44_hardened_derivation = 0x80000000;
static constexpr uint32_t bip44_hardened_derivation_testnet = 0x80000001;
static constexpr uint32_t bip44_purpose =
    bip44_hardened_derivation + 44;  // 0x8000002C
static constexpr uint32_t bip44_coin_type_mainnet = bip44_hardened_derivation;
static constexpr uint32_t bip44_coin_type_testnet =
    bip44_hardened_derivation_testnet;
static constexpr uint32_t bip44_account = bip44_hardened_derivation;

static constexpr uint32_t qr_code_size = 12;

// secure endpoint of the official mainnet community server
static const std::string libbitcoin_server_address =
    "tcp://mainnet.libbitcoin.net:9081";
static const std::string libbitcoin_server_public_key =
    "0HxgF^7dEzWu)$R^YE/q7mm0=t@o7RDUT}K.@oY@";

// secure endpoint of the official testnet community server
static const std::string libbitcoin_testnet_server_address =
    "tcp://testnet.libbitcoin.net:19081";
static const std::string libbitcoin_testnet_server_public_key =
    ")nNv4Ji=CU:}@<LOu-<QvB)b-PIh%PX[)?mH>XAl";

static const std::string default_currency = "USD";

static constexpr size_t default_account_index = 0;

// external (public) address index
static constexpr size_t external_address_index = 0;
// internal (private/change) address index
static constexpr size_t internal_address_index = 1;

}  // namespace constants
}  // namespace megabit

#endif  // __CONSTANTS_HPP
