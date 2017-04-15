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

#include "../include/megabit/utils.hpp"

#include <bitcoin/bitcoin.hpp>

namespace megabit {
namespace utils {

std::string bitcoin_address(const libbitcoin::ec_secret& secret,
                            uint8_t payment_address_version) {
  libbitcoin::ec_compressed point;
  libbitcoin::secret_to_public(point, secret);
  return bitcoin_address(point, payment_address_version);
}

std::string bitcoin_address(const libbitcoin::ec_compressed& point,
                            uint8_t payment_address_version) {
  const libbitcoin::wallet::payment_address address(point,
                                                    payment_address_version);
  return address.encoded();
}

std::string bitcoin_address(const std::string hex_compressed,
                            uint8_t payment_address_version) {
  const auto pub = libbitcoin::wallet::ec_public(hex_compressed);
  const auto address =
      libbitcoin::wallet::payment_address(pub, payment_address_version);
  return address.encoded();
}

std::string bitcoin_address(const libbitcoin::chain::script& script,
                            uint8_t payment_address_version) {
  const auto script_address = libbitcoin::wallet::payment_address::extract(
      script, payment_address_version);
  return script_address[0].encoded();
}

libbitcoin::data_chunk uncompressed_public_from_private(
    const libbitcoin::ec_secret& secret) {
  libbitcoin::wallet::ec_private private_key(secret, false);
  const auto public_key = private_key.to_public();

  libbitcoin::ec_uncompressed pub_key_data;
  public_key.to_uncompressed(pub_key_data);
  return libbitcoin::to_chunk(pub_key_data);
}

libbitcoin::data_chunk compressed_public_from_private(
    const libbitcoin::ec_secret& secret) {
  libbitcoin::wallet::ec_private private_key(secret, true);
  const auto public_key = private_key.to_public();

  libbitcoin::data_chunk pub_key_data;
  public_key.to_data(pub_key_data);
  return pub_key_data;
}

libbitcoin::data_chunk public_from_private(const libbitcoin::ec_secret& secret,
                                           const bool compress) {
  return (compress ? compressed_public_from_private(secret)
                   : uncompressed_public_from_private(secret));
}

std::string get_current_date_string() {
  time_t cur_time;
  time(&cur_time);
  struct tm* cur_tm = localtime(&cur_time);

  std::array<char, megabit::constants::max_date_string_length> datetime;
  std::strftime(datetime.data(), datetime.size(), "%F", cur_tm);

  return std::string(datetime.data());
}

std::string get_date_string(const uint32_t timestamp) {
  std::array<char, megabit::constants::max_date_string_length> datetime;
  time_t block_time = static_cast<time_t>(timestamp);
  const auto tm = localtime(&block_time);
  std::strftime(datetime.data(), datetime.size(), "%D %R", tm);
  return std::string(datetime.data());
}

void get_passphrase_key(libbitcoin::hash_digest& out_hash,
                        const std::string& passphrase) {
  const auto start = reinterpret_cast<const unsigned char*>(passphrase.c_str());
  const auto end = start + passphrase.length();
  out_hash =
      libbitcoin::bitcoin_hash(libbitcoin::array_slice<uint8_t>(start, end));
}

bool get_user_wallet_seed(libbitcoin::long_hash& seed,
                          libbitcoin::data_chunk& checksum,
                          libbitcoin::hash_digest& passphrase_hash) {
  auto seed_chunk = libbitcoin::to_chunk(seed);
  mem_lock_region(seed_chunk);

  auto passphrase_hash_chunk = libbitcoin::to_chunk(passphrase_hash);
  mem_lock_region(passphrase_hash_chunk);

  // verify the checksum using encrypted seed and passphrase hash
  libbitcoin::data_chunk extended(passphrase_hash_chunk);
  libbitcoin::extend_data(extended, seed_chunk);

  std::memset(seed_chunk.data(), 0, seed_chunk.size());
  std::memset(passphrase_hash_chunk.data(), 0, passphrase_hash_chunk.size());

  mem_unlock_region(seed_chunk);
  mem_unlock_region(passphrase_hash_chunk);

  if (!megabit::utils::verify_checksum(checksum, extended)) {
    std::memset(&seed, 0, sizeof(seed));
    std::memset(&passphrase_hash, 0, sizeof(passphrase_hash));
    return false;
  }

  megabit::utils::decrypt_data<libbitcoin::long_hash>(passphrase_hash, seed);
  return true;
}

std::string to_string(const libbitcoin::chain::output& output) {
  static constexpr auto flags = libbitcoin::machine::rule_fork::all_rules;

  std::ostringstream text;
  text << "\tvalue = " << output.value() << "\n"
       << "\t" << output.script().to_string(flags) << "\n";

  return text.str();
}

std::string to_string(const libbitcoin::chain::transaction& tx) {
  static constexpr auto flags = libbitcoin::machine::rule_fork::all_rules;

  std::ostringstream value;
  value << "Transaction:\n"
        << "\tversion = " << tx.version() << "\n"
        << "\tlocktime = " << tx.locktime() << "\nInputs:\n";

  for (const auto& input : tx.inputs()) {
    value << libbitcoin::encode_base16(input.previous_output().hash()) << ":"
          << input.previous_output().index() << "\n"
          << "\t" << input.script().to_string(flags) << "\n"
          << "\tsequence = " << input.sequence() << "\n";
  }
  value << "Outputs:\n";

  for (const auto& output : tx.outputs()) {
    value << to_string(output);
  }
  value << "\n";

  return value.str();
}

}  // namespace utils

}  // namespace megabit
