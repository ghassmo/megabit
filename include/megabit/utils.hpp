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

#ifndef __UTILS_HPP
#define __UTILS_HPP

#include <QtGlobal>
#include <array>
#include <bitcoin/bitcoin.hpp>
#include <ctime>

#include "constants.hpp"

#define MEGABIT_ASSERT Q_ASSERT

namespace megabit {

namespace utils {

// converts data/types into a human readable bitcoin address
std::string bitcoin_address(const std::string hex_compressed,
                            uint8_t payment_address_version);
std::string bitcoin_address(const libbitcoin::ec_secret& secret,
                            uint8_t payment_address_version);
std::string bitcoin_address(const libbitcoin::ec_compressed& point,
                            uint8_t payment_address_version);
std::string bitcoin_address(const libbitcoin::chain::script& script,
                            uint8_t payment_address_version);

libbitcoin::data_chunk uncompressed_public_from_private(
    const libbitcoin::ec_secret& secret);
libbitcoin::data_chunk compressed_public_from_private(
    const libbitcoin::ec_secret& secret);
libbitcoin::data_chunk public_from_private(const libbitcoin::ec_secret& secret,
                                           const bool compress = true);

#ifndef _WIN32
#include <sys/mman.h>

template <class T>
void mem_lock_region(T& obj) {
  mlock(obj.data(), obj.size());
}

#else

template <class T>
void mem_lock_region(T& /* obj */) {
  // FIXME: implement on windows
}

#endif  // _WIN32

template <class T>
void mem_unlock_region(T& obj) {
  std::memset(obj.data(), 0, obj.size());
  munlock(obj.data(), obj.size());
}

std::string get_current_date_string();

std::string get_date_string(const uint32_t timestamp);

void get_passphrase_key(libbitcoin::hash_digest& out_hash,
                        const std::string& passphrase);

bool get_user_wallet_seed(libbitcoin::long_hash& seed,
                          libbitcoin::data_chunk& checksum,
                          libbitcoin::hash_digest& passphrase_hash);

std::string to_string(const libbitcoin::chain::output& output);
std::string to_string(const libbitcoin::chain::transaction& tx);

template <class T>
libbitcoin::data_chunk get_checksum(T& data) {
  libbitcoin::data_chunk checksum(megabit::constants::checksum_length);
  const auto hash = libbitcoin::bitcoin_hash(libbitcoin::bitcoin_hash(data));
  const auto hash_size = sizeof(hash);
  for (size_t i = 0, j = 0; i < hash_size;
       i += (hash_size / megabit::constants::checksum_length)) {
    checksum[j++] = hash[i];
  }

  MEGABIT_ASSERT(checksum.size() == megabit::constants::checksum_length);
  return checksum;
}

template <class T>
bool verify_checksum(libbitcoin::data_chunk& checksum, T& data) {
  MEGABIT_ASSERT(checksum.size() == megabit::constants::checksum_length);

  libbitcoin::data_chunk cur_checksum(megabit::constants::checksum_length);
  const auto hash = libbitcoin::bitcoin_hash(libbitcoin::bitcoin_hash(data));
  const auto hash_size = sizeof(hash);
  for (size_t i = 0, j = 0; i < hash_size;
       i += (hash_size / megabit::constants::checksum_length)) {
    cur_checksum[j++] = hash[i];
  }

  return ((cur_checksum.size() == checksum.size()) &&
          std::equal(checksum.begin(), checksum.end(), cur_checksum.begin()));
}

template <class T>
void encrypt_data(const libbitcoin::aes_secret& secret, T& data) {
  const auto data_size = sizeof(data);
  if ((data_size % libbitcoin::aes256_block_size) != 0) {
    throw std::runtime_error(
        "encrypt_data requires data size "
        "to be a multiple of 16");
  }

  if (sizeof(secret) != libbitcoin::aes256_key_size) {
    throw std::runtime_error(
        "encrypt_data requires secret size "
        "to be 32");
  }

  const auto iterations = data_size / libbitcoin::aes256_block_size;
  auto* start = reinterpret_cast<libbitcoin::aes_block*>(&data);

  for (size_t i = 0; i < iterations; i++) {
    libbitcoin::aes256_encrypt(secret, *start++);
  }
}

template <class T>
void decrypt_data(const libbitcoin::aes_secret& secret, T& data) {
  const auto data_size = sizeof(data);
  if ((data_size % libbitcoin::aes256_block_size) != 0) {
    throw std::runtime_error(
        "decrypt_data requires data size "
        "to be a multiple of 16");
  }

  if (sizeof(secret) != libbitcoin::aes256_key_size) {
    throw std::runtime_error(
        "decrypt_data requires secret size "
        "to be 32");
  }

  const auto iterations = data_size / libbitcoin::aes256_block_size;
  auto* start = reinterpret_cast<libbitcoin::aes_block*>(&data);

  for (size_t i = 0; i < iterations; i++) {
    libbitcoin::aes256_decrypt(secret, *start++);
  }
}

}  // namespace utils

}  // namespace megabit

#endif  // __UTILS_HPP
