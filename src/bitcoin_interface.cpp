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

#include "include/megabit/bitcoin_interface.hpp"

#include "include/megabit/constants.hpp"

BitcoinInterface::BitcoinInterface() {
  initialized_ = false;
  num_accounts_ = 1;
  block_height_ = 0;
  gap_limit_ = megabit::constants::bip44_gap_limit;
  prefixes_ = libbitcoin::wallet::hd_private::mainnet;
  payment_address_version_ = libbitcoin::wallet::payment_address::mainnet_p2kh;
  bip44_coin_type_ = megabit::constants::bip44_coin_type_mainnet;
}

void BitcoinInterface::SetNetwork(const std::string& network) {
  auto network_lower = network;
  std::transform(network.begin(), network.end(), network_lower.begin(),
                 [](unsigned char c) { return std::tolower(c); });

  if (network_lower == "mainnet") {
    prefixes_ = libbitcoin::wallet::hd_private::mainnet;
    payment_address_version_ =
        libbitcoin::wallet::payment_address::mainnet_p2kh;
    bip44_coin_type_ = megabit::constants::bip44_coin_type_mainnet;
  } else {
    prefixes_ = libbitcoin::wallet::hd_private::testnet;
    payment_address_version_ =
        libbitcoin::wallet::payment_address::testnet_p2kh;
    bip44_coin_type_ = megabit::constants::bip44_coin_type_testnet;
  }
}

void BitcoinInterface::SetNumAccounts(const size_t num_accounts) {
  num_accounts_ = num_accounts;
}

void BitcoinInterface::SetServerInfo(
    const std::string& libbitcoin_server_address,
    const std::string& libbitcoin_server_public_key) {
  libbitcoin_server_address_ = libbitcoin_server_address;
  libbitcoin_server_public_key_ = libbitcoin_server_public_key;
}

bool BitcoinInterface::InitializeFromSeed(const Seed& seed) {
  std::cout << "Connecting to " << libbitcoin_server_address_
            << " using public key " << libbitcoin_server_public_key_
            << std::endl;
  bool ret = false;
  if (libbitcoin_server_public_key_.empty()) {
    ret = client_.connect(libbitcoin_server_address_) &&
          block_height_client_.connect(libbitcoin_server_address_);
  } else {
    ret = client_.connect(
              libbitcoin_server_address_, {},
              libbitcoin::config::sodium(libbitcoin_server_public_key_), {}) &&
          block_height_client_.connect(
              libbitcoin_server_address_, {},
              libbitcoin::config::sodium(libbitcoin_server_public_key_), {});
  }

  if (ret) {
    auto seed_chunk = libbitcoin::to_chunk(seed);
    megabit::utils::mem_lock_region(seed_chunk);

    bip32_root_private_key_ =
        libbitcoin::wallet::hd_private(seed_chunk, prefixes_);

    std::memset(seed_chunk.data(), 0, seed_chunk.size());
    megabit::utils::mem_unlock_region(seed_chunk);

    // Cache bip44 derived addresses for each account
    size_t cur_gap_limit = 0;
    for (size_t account = 0; account < num_accounts_; account++) {
      /* std::cout << "bip44 m/" */
      /*           << "44'" */
      /*           << "/" */
      /*           << "0'" */
      /*           << "/" << std::endl; */
      for (size_t internal = 0; internal < 2; internal++) {
        const auto chain = (internal ? "change" : "external");
        /*     std::cout << " " << chain << " addresses m/44'/0'/" << account <<
         */
        /* "'/" */
        /*               << internal << "/" << std::endl; */

        cur_gap_limit = gap_limit_;
        for (size_t index = 0; index < cur_gap_limit; index++) {
          // cache all addresses associated with our wallet accounts
          const auto& key = GetKey(account, internal, index);
          libbitcoin::ec_compressed point{};
          libbitcoin::secret_to_public(point, key);
          libbitcoin::wallet::payment_address address(point,
                                                      payment_address_version_);
          if (internal) {
            internal_address_cache_.insert(address.encoded());
          } else {
            external_address_cache_.insert(address.encoded());
          }

          /*       std::cout << "   m/44'/0'/" << account << "'/" << internal <<
           * "/" */
          /*                 << std::setfill('0') << std::setw(4) << index */
          /*                 << std::setfill(' ') << std::setw(35) << */
          /* address.encoded() */
          /*                 << ", (key=" */
          /*                 << libbitcoin::wallet::ec_private(secret).encoded()
           * << */
          /* ")" */
          /*                 << std::endl; */
        }
      }
    }

    initialized_ = ret;
  }
  return ret;
}

libbitcoin::wallet::payment_address BitcoinInterface::GetPaymentAddress(
    const std::string address) {
  return libbitcoin::wallet::payment_address(
      address /* , payment_address_version_ */);
}

const libbitcoin::wallet::hd_private BitcoinInterface::GetKey(
    uint32_t account, uint32_t internal, uint32_t index) const {
  static const auto purpose_key =
      bip32_root_private_key_.derive_private(megabit::constants::bip44_purpose);
  static const auto coin_type_key =
      purpose_key.derive_private(bip44_coin_type_);

  const auto account_key =
      coin_type_key.derive_private(megabit::constants::bip44_account + account);

  const auto key = account_key.derive_private(internal);
  return key.derive_private(index);
}

const libbitcoin::data_chunk BitcoinInterface::GetAddressAsPngData(
    const std::string& address, bool prefix) {
  libbitcoin::data_chunk png_data;
  libbitcoin::data_sink png_ostream(png_data);

  const std::string final_address = (prefix ? "bitcoin:" + address : address);
  const auto qr_data =
      libbitcoin::wallet::qr::encode(libbitcoin::to_chunk(final_address));
  libbitcoin::png::write_png(qr_data, megabit::constants::qr_code_size,
                             png_ostream);

  return png_data;
}

const std::string BitcoinInterface::GetNextAddressForAccount(
    uint32_t account_index, uint32_t internal) {
  MEGABIT_ASSERT(internal < 2);

  size_t cur_gap_limit = gap_limit_;
  for (size_t index = 0; index < cur_gap_limit; index++) {
    const auto& key = GetKey(account_index, internal, index);
    libbitcoin::wallet::payment_address address(
        libbitcoin::wallet::ec_public(key), payment_address_version_);

    AddressHistory history{};
    if (!GetAddressHistory(key.secret(), history)) {
      return {};
    }

    // if the address was already used, or has a balance, process
    // past it by extending the cur_gap_limit
    if (history.total_value || history.is_spent()) {
      ++cur_gap_limit;
    } else {
      // insert next receive address into external cache
      external_address_cache_.insert(address.encoded());

      return address.encoded();
    }
  }
  return {};
}

/* const uint64_t BitcoinInterface::GetAccountBalance( */
/*     uint32_t account_index, TxUpdaterFunction update_fn) { */
/*   uint64_t total_balance = 0; */
/*   size_t cur_gap_limit = gap_limit_; */

/*   // internal == 0 indicates a receive address */
/*   // internal == 1 indicates a change address */
/*   for (size_t index = 0; index < cur_gap_limit; index++) { */
/*     for (size_t internal = 0; internal < 2; internal++) { */
/*       const auto& key = GetKey(account_index, internal, index); */

/*       AddressHistory history{}; */
/*       if (!GetAddressHistory(key.secret(), history)) { */
/*         break; */
/*       } */

/*       // if the address was already used, or has a balance, process */
/*       // past it by extending the cur_gap_limit */
/*       if (history.total_value || history.is_spent()) { */
/*         total_balance += history.total_value; */
/*         ++cur_gap_limit; */
/*       } */

/*       size_t height = 0; */
/*       uint64_t amount = 0; */
/*       uint64_t input_amount = 0; */
/*       uint64_t change_amount = 0; */
/*       uint64_t fee = 0; */

/*       libbitcoin::hash_digest hash{}; */
/*       libbitcoin::chain::point tx_point{}; */
/*       bool is_spend = history.is_spent(); */

/*       bool valid = false; */
/*       TxBlockInfo tx_block_info{}; */
/*       TxBlockInfo output_tx_block_info{}; */
/*       for (const auto& transfer : history.transfers) { */
/*         valid = false; */
/*         if (is_spend) { */
/*           std::cout <<
 * "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" <<
 * std::endl; */
/*           // Used to compute the spent amount only */
/*           GetTransactionInfo(transfer.spend.hash(), tx_block_info); */
/*           GetTransactionInfo(transfer.output.hash(), output_tx_block_info);
 */

/*           for(const auto& input : tx_block_info.tx.inputs()) { */
/*               auto output  = input.previous_output(); */
/*               std::cout << "Got INPUT INDEX: " << output.index() <<
 * std::endl; */
/*               // Find the value of this input */
/*               input_amount +=
 * output_tx_block_info.tx.outputs()[input.previous_output().index()].value();
 */
/*               std::cout << "AMOUNT IS NOW: " << amount << std::endl; */
/*           } */

/*           for(const auto& output : tx_block_info.tx.outputs()) { */
/*               std::cout << "OUTPUT VALUE (SPEND AMOUNT?): " << output.value()
 * << std::endl; */
/*               /\* for(auto& cur_address :
 * output.addresses(payment_address_version_)) { *\/ */
/*               auto addresses =
 * libbitcoin::wallet::payment_address::extract(output.script(),
 * payment_address_version_); */
/*               for(auto& cur_address : addresses) { */
/*                   if (internal_address_cache_.find(cur_address.encoded()) !=
 * internal_address_cache_.end()) { */
/*                       change_amount += output.value(); */
/*                       std::cout << "  SPEND ADDRESS " <<
 * cur_address.encoded() << " *IS* A CHANGE ADDRESS" << std::endl; */
/*                   } else { */
/*                       amount += output.value(); */
/*                       std::cout << "  SPEND ADDRESS " <<
 * cur_address.encoded() << " IS NOT ONE OF OURS" << std::endl; */
/*                   } */
/*               } */
/*           } */

/*           fee = input_amount - amount - change_amount; */
/*           std::cout << "INPUT AMOUNT: " << input_amount << " TX AMOUNT: " <<
 * amount */
/*                     << " CHANGE AMOUNT: " << change_amount << " FEE: " << fee
 * << std::endl; */
/*           amount += fee; */
/*           std::cout <<
 * "----------------------------------------------------------------" <<
 * std::endl; */
/*           valid = true; */
/*         } else { */
/*           GetTransactionInfo(transfer.output.hash(), tx_block_info); */
/*           amount += transfer.value; */
/*           std::cout << "GOT RECEIVE AMOUNT OF: " << amount << std::endl; */
/*           valid = true; */
/*         } */

/*         if (!height) { */
/*           height = tx_block_info.height; */
/*         } else if (tx_block_info.height > height) { */
/*           height = tx_block_info.height; */
/*         } */

/*         hash = libbitcoin::hash_digest(transfer.output.hash()); */
/*         tx_point = libbitcoin::chain::point(transfer.output); */
/*       } */

/*       if (valid ) { */
/*         libbitcoin::wallet::payment_address address( */
/*             libbitcoin::wallet::ec_public(key), payment_address_version_); */

/*         std::reverse(hash.begin(), hash.end()); */

/*         TxInfo tx_info{is_spend, */
/*                        amount, */
/*                        height, */
/*                        address.encoded(), */
/*                        hash, */
/*                        tx_point, */
/*                        tx_block_info.header, */
/*                        tx_block_info.tx}; */

/*         update_fn(account_index, index, cur_gap_limit, tx_info); */
/*       } */
/*     } */
/*   } */

/*   return total_balance; */
/* } */

const uint64_t BitcoinInterface::GetAccountBalance(
    bool& error, uint32_t account_index, TxUpdaterFunction update_fn) {
  uint64_t total_balance = 0;
  size_t cur_gap_limit = gap_limit_;

  // internal == 0 indicates a receive address
  // internal == 1 indicates a change address
  for (size_t index = 0; index < cur_gap_limit; index++) {
    if (error) {
      std::cout << "Aborting due to unrecoverable error" << std::endl;
      break;
    }

    for (size_t internal = 0; internal < 2; internal++) {
      const auto& key = GetKey(account_index, internal, index);

      AddressHistory history{};
      if (!GetAddressHistory(key.secret(), history)) {
        error = true;
        break;
      }

      // if the address was already used, or has a balance, process
      // past it by extending the cur_gap_limit
      if (history.total_value || history.is_spent()) {
        total_balance += history.total_value;
        ++cur_gap_limit;
      }

      size_t height = 0;
      uint64_t amount = 0;
      uint64_t input_amount = 0;
      uint64_t change_amount = 0;
      uint64_t fee = 0;

      uint64_t received_amount = 0;
      uint64_t received_input_amount = 0;
      uint64_t received_change_amount = 0;
      uint64_t received_fee = 0;

      libbitcoin::hash_digest hash{};
      libbitcoin::chain::point tx_point{};
      bool is_spend = history.is_spent();

      TxBlockInfo tx_block_info{};
      TxBlockInfo output_tx_block_info{};
      for (const auto& transfer : history.transfers) {
        if (is_spend) {
          std::cout << "+++++++++++++++++++++++++++++++++++++++++++++++++++++++"
                       "+++++++++"
                    << std::endl;
          // Used to compute the spent amount only
          GetTransactionInfo(transfer.spend.hash(), tx_block_info);
          GetTransactionInfo(transfer.output.hash(), output_tx_block_info);

          // FIXME: Sent from account 5 shows, but there's no Received from
          // account 5(!!!)

          // calculate the amount received
          for (const auto& input : output_tx_block_info.tx.inputs()) {
            auto output = input.previous_output();
            std::cout << "got input index: " << output.index() << std::endl;
            // Find the value of this input
            received_input_amount +=
                output_tx_block_info.tx
                    .outputs()[input.previous_output().index()]
                    .value();
            std::cout << "amount is now: " << amount << std::endl;
          }

          for (const auto& output : output_tx_block_info.tx.outputs()) {
            std::cout << "output value (spend amount?): " << output.value()
                      << std::endl;
            for (auto& cur_address :
                 output.addresses(payment_address_version_)) {
              /* auto addresses =
               * libbitcoin::wallet::payment_address::extract(output.script(),
               * payment_address_version_); */
              /* for(auto& cur_address : addresses) { */
              if (external_address_cache_.find(cur_address.encoded()) !=
                  internal_address_cache_.end()) {
                received_change_amount += output.value();
                std::cout << "  spend address " << cur_address.encoded()
                          << " *is* a change address" << std::endl;
              } else {
                received_amount += output.value();
                std::cout << "  spend address " << cur_address.encoded()
                          << " is not one of ours" << std::endl;
              }
            }
          }

          // calculate the amount spent
          for (const auto& input : tx_block_info.tx.inputs()) {
            auto output = input.previous_output();
            std::cout << "Got INPUT INDEX: " << output.index() << std::endl;
            // Find the value of this input
            input_amount += output_tx_block_info.tx
                                .outputs()[input.previous_output().index()]
                                .value();
            std::cout << "AMOUNT IS NOW: " << amount << std::endl;
          }

          for (const auto& output : tx_block_info.tx.outputs()) {
            std::cout << "OUTPUT VALUE (SPEND AMOUNT?): " << output.value()
                      << std::endl;
            for (auto& cur_address :
                 output.addresses(payment_address_version_)) {
              /* auto addresses =
               * libbitcoin::wallet::payment_address::extract(output.script(),
               * payment_address_version_); */
              /* for(auto& cur_address : addresses) { */
              if (internal_address_cache_.find(cur_address.encoded()) !=
                  internal_address_cache_.end()) {
                change_amount += output.value();
                std::cout << "  spend address " << cur_address.encoded()
                          << " *is* a change address" << std::endl;
              } else {
                amount += output.value();
                std::cout << "  spend address " << cur_address.encoded()
                          << " is not one of ours" << std::endl;
              }
            }
          }

          fee = input_amount - amount - change_amount;
          received_fee =
              received_input_amount - received_amount - received_change_amount;
          std::cout << "input amount: " << input_amount
                    << " tx amount: " << amount
                    << " change amount: " << change_amount << " fee: " << fee
                    << std::endl;

          std::cout << "received input amount: " << received_input_amount
                    << " received tx amount: " << received_amount
                    << " received change amount: " << received_change_amount
                    << " received fee: " << received_fee << std::endl;
          amount += fee;
          received_amount += received_fee;
          std::cout << "-------------------------------------------------------"
                       "---------"
                    << std::endl;
        } else {
          GetTransactionInfo(transfer.output.hash(), tx_block_info);
          /* amount += transfer.value; */
          amount = transfer.value;
          std::cout << "got receive amount of: " << amount << std::endl;
        }

        if (!height) {
          height = tx_block_info.height;
        } else if (tx_block_info.height > height) {
          height = tx_block_info.height;
        }

        if (is_spend) {
          hash = libbitcoin::hash_digest(transfer.output.hash());
          tx_point = libbitcoin::chain::point(transfer.output);

          libbitcoin::wallet::payment_address address(
              libbitcoin::wallet::ec_public(key), payment_address_version_);

          std::reverse(hash.begin(), hash.end());

          TxInfo tx_info{!is_spend,
                         amount,
                         height,
                         address.encoded(),
                         hash,
                         tx_point,
                         output_tx_block_info.header,
                         output_tx_block_info.tx};

          update_fn(account_index, index, cur_gap_limit, tx_info);
        }

        hash = libbitcoin::hash_digest(transfer.output.hash());
        tx_point = libbitcoin::chain::point(transfer.output);

        libbitcoin::wallet::payment_address address(
            libbitcoin::wallet::ec_public(key), payment_address_version_);

        std::reverse(hash.begin(), hash.end());

        TxInfo tx_info{is_spend,
                       amount,
                       height,
                       address.encoded(),
                       hash,
                       tx_point,
                       tx_block_info.header,
                       tx_block_info.tx};

        update_fn(account_index, index, cur_gap_limit, tx_info);
      }
    }
  }

  error = false;
  return total_balance;
}

uint64_t BitcoinInterface::GetUnconfirmedTransactionAmount(
    const libbitcoin::chain::transaction& tx) {
  uint64_t amount = 0;
  for (const auto& output : tx.outputs()) {
    auto addresses = libbitcoin::wallet::payment_address::extract(
        output.script(), payment_address_version_);
    for (auto& cur_address : addresses) {
      if (external_address_cache_.find(cur_address.encoded()) !=
          external_address_cache_.end()) {
        amount += output.value();
      }
    }
  }

  return amount;
}

bool BitcoinInterface::GetAddressHistory(const libbitcoin::ec_secret& key,
                                         AddressHistory& history) {
  libbitcoin::wallet::payment_address address(
      libbitcoin::wallet::ec_public(key), payment_address_version_);

  return GetAddressHistory(address.encoded(), history);
}

bool BitcoinInterface::GetAddressHistory(const std::string& address,
                                         AddressHistory& history) {
  auto ret = true;
  auto on_done =
      [address, &ret,
       /* &history](const libbitcoin::client::history::list& rows) { */
       &history](const libbitcoin::chain::history::list& rows) {
        history.transfers.reserve(rows.size());

        /* bool spent = false; */
        for (const auto& row : rows) {
          if ((row.spend_height == megabit::constants::unspent_height) &&
              (row.spend.index() == megabit::constants::unspent_index)) {
            history.total_value += row.value;
            /* } else if (row.spend_height !=
             * megabit::constants::unspent_height) { */
            /*     spent = true; */
          } else if (row.output.hash() == libbitcoin::null_hash) {
            std::cout << "GOT UNSPENT ROW WITH OUTPUT HEIGHT: "
                      << row.output_height << std::endl;
          }

          /* std::cout << "Got Address history for " << address << " with total
           * value " */
          /*           << history.total_value << " -- SPENT? " << spent <<
           * std::endl; */

          history.transfers.emplace_back(row.output, row.output_height,
                                         row.spend, row.spend_height,
                                         row.value);
        }
      };

  auto on_error = [this, &address, &ret](const libbitcoin::code& error) {
    if (error) {
      std::cout << "Failed to retrieve address information for " << address
                << ": " << error << std::endl;

      ret = false;
    }
  };

  client_.blockchain_fetch_history3(on_error, on_done, address);
  client_.wait();

  return ret;
}

bool BitcoinInterface::TransactionIsValid(
    const libbitcoin::chain::transaction& transaction) {
  bool ret = false;

  auto on_done = [&ret, &transaction](const libbitcoin::code& error) {
    std::cout << "Transaction is valid!" << std::endl
              << megabit::utils::to_string(transaction) << std::endl;
    ret = (error == libbitcoin::error::success);
  };

  auto on_error = [&transaction](const libbitcoin::code& error) {
    std::cout << "Failed to validate transaction: " << error.message()
              << std::endl;
    std::cout << "Failed raw transaction:"
              << libbitcoin::encode_base16(transaction.to_data()) << std::endl;
    std::cout << "Failed transaction:" << megabit::utils::to_string(transaction)
              << std::endl;
  };

  client_.transaction_pool_validate2(on_error, on_done, transaction);
  client_.wait();

  return ret;
}

bool BitcoinInterface::GetTransactionInfo(
    const libbitcoin::hash_digest& tx_hash, TxBlockInfo& tx_block_info,
    bool unconfirmed) {
  // NOTE: we reverse the hash ONLY for logging/printing
  auto hash = libbitcoin::hash_digest(tx_hash);
  std::reverse(hash.begin(), hash.end());
  std::cout << "-> Getting transaction info for: "
            << libbitcoin::encode_base16(hash) << std::endl;

  auto ret = false;

  auto on_error = [&ret](const libbitcoin::code& error) {
    std::cerr << "ERROR: " << error.message() << std::endl;
    ret = false;
  };

  auto on_done = [&tx_block_info,
                  &ret](const libbitcoin::chain::transaction& tx) {
    tx_block_info.tx = tx;
    ret = true;
  };

  auto on_fetch_tx_index_done = [&tx_block_info, &ret](size_t height,
                                                       size_t index) {
    tx_block_info.height = height;
    tx_block_info.index = index;
    ret = true;
    std::cout << "fetch tx done: height " << height << ", " << index << index
              << std::endl;
  };

  auto on_fetch_header_done = [&tx_block_info,
                               &ret](const libbitcoin::chain::header& header) {
    tx_block_info.header = header;
    ret = true;
    std::cout << "fetch header done called" << std::endl;
  };

  if (unconfirmed) {
    client_.transaction_pool_fetch_transaction(on_error, on_done, tx_hash);
  } else {
    client_.blockchain_fetch_transaction(on_error, on_done, tx_hash);
  }
  client_.wait();

  if (ret && !unconfirmed) {
    /* MEGABIT_ASSERT(tx_hash == tx_block_info.tx.hash()); */

    client_.blockchain_fetch_transaction_index(on_error, on_fetch_tx_index_done,
                                               tx_block_info.tx.hash());
    client_.wait();

    if (ret) {
      std::cout << "about to fetch block header of height: "
                << tx_block_info.height << std::endl;
      client_.blockchain_fetch_block_header(
          on_error, on_fetch_header_done,
          ((tx_block_info.height > 0) ? tx_block_info.height : block_height_));
      client_.wait();
    }
  }
  return ret;
}

void BitcoinInterface::GetBlockHeight(ErrorHandler on_error,
                                      BlockHeightHandler handler) {
  // this is called from a thread, so it uses a completely different
  // connection to avoid any network corruption
  block_height_client_.blockchain_fetch_last_height(on_error, handler);
  block_height_client_.wait();
}

void BitcoinInterface::SetBlockHeight(const size_t block_height) {
  block_height_ = block_height;
}

libbitcoin::chain::points_value
BitcoinInterface::GetUnspentOutputsForAccountIndex(
    const uint32_t account_index, UnspentList& unspent_list,
    libbitcoin::wallet::payment_address& change_address,
    std::vector<std::string>* excluded) {
  std::cout << "GetUnspentOutputsForAccountIndex called" << std::endl;
  auto assigned_change_address = false;
  libbitcoin::chain::points_value unspent{};

  const auto cur_height = block_height_;
  // internal == 0 indicates a receive address
  // internal == 1 indicates a change address
  for (size_t internal = 0; internal < 2; internal++) {
    for (size_t k = 0; k < gap_limit_; k++) {
      const auto key = GetKey(account_index, internal, k);
      const auto address = megabit::utils::bitcoin_address(
          libbitcoin::wallet::ec_public(key), payment_address_version_);

      // if this address has been excluded by user input, do
      // not consider it for return here
      if (excluded) {
        bool addr_excluded = false;
        for (const auto& addr : *excluded) {
          if (addr == address) {
            addr_excluded = true;
            break;
          }
        }

        if (addr_excluded) {
          std::cout << "Skipping utxos for address " << address
                    << " in mix depth " << account_index
                    << " due to user specified exclusion" << std::endl;
          continue;
        }
      }

      AddressHistory history{};
      if (!GetAddressHistory(key.secret(), history)) {
        break;
      }

      if (history.is_spent()) {
        continue;
      }

      if (internal && !assigned_change_address) {
        change_address = libbitcoin::wallet::payment_address(
            libbitcoin::wallet::ec_public(key), payment_address_version_);
        assigned_change_address = true;

        std::cout << "Sending change to "
                  << megabit::utils::bitcoin_address(
                         libbitcoin::wallet::ec_public(key),
                         payment_address_version_)
                  << std::endl;
      }

      for (const auto& transfer : history.transfers) {
        // FIXME: relax confirmed check here?
        if (transfer.confirmed(cur_height) && !transfer.is_spent()) {
          std::cout << "Adding unspent for account index " << account_index
                    << " with hash "
                    << libbitcoin::encode_base16(transfer.output.hash())
                    << " and value " << transfer.value << std::endl;

          unspent.points.push_back({transfer.output, transfer.value});
          unspent_list.push_back({key, transfer});
        }
      }
    }
  }
  return unspent;
}

bool BitcoinInterface::InitiateSendPayment(
    const uint32_t account_index, uint64_t& amount,
    const libbitcoin::wallet::payment_address destination_address,
    const uint64_t target_fee_per_kb, bool subtract_fee_from_amount) {
  uint64_t change_amount = 0;
  UnspentList selected_unspent_list;
  libbitcoin::wallet::payment_address change_address{};

  // NOTE: unless we're subtracting the fee from the amount this
  // retrieves utxos that satisfy the amount + a fee of
  // 0.5KB.  It may not be a good enough estimate for all cases, and
  // may be a terrible estimate in others (perhaps causing funds to be
  // arbitrarily trapped for no reason).  If you find yourself in this
  // situation, adjust this value accordingly.
  auto adjusted_amount =
      (subtract_fee_from_amount ? amount : amount + (target_fee_per_kb / 2));

  std::cout << "Send Payment called with amount " << amount
            << ", but searching for utxos to satisfy the amount of "
            << adjusted_amount << " to account for an estimated fee"
            << std::endl;

  auto ret = RetrieveUnspentAndChangeAddress(selected_unspent_list,
                                             change_address, change_amount,
                                             account_index, adjusted_amount);

  std::cout << "retrieve unspent and change address returned: " << ret
            << " and change address " << change_address.encoded() << std::endl;
  if (ret) {
    // FIXME: if the amount was adjusted, is this right in all cases?
    if (!subtract_fee_from_amount &&
        (adjusted_amount != amount + (target_fee_per_kb / 2))) {
      std::cout << "adjusted amount " << adjusted_amount
                << " != " << (amount + (target_fee_per_kb / 2)) << std::endl;
      amount = adjusted_amount - (target_fee_per_kb / 2);
      std::cout << "re adjusted amount to " << amount << std::endl;
    }

    auto tx = CreateSignedTransaction(
        selected_unspent_list, amount, destination_address, target_fee_per_kb,
        change_amount, change_address, subtract_fee_from_amount);

    pending_transaction_ = std::make_shared<PendingTransaction>(
        tx, selected_unspent_list, amount, destination_address,
        target_fee_per_kb, change_amount, change_address,
        subtract_fee_from_amount);
  }
  return ret;
}

bool BitcoinInterface::SendPendingTransaction() {
  return CreateAndBroadcastTransaction(
      pending_transaction_->selected_unspent_list, pending_transaction_->amount,
      pending_transaction_->destination_address,
      pending_transaction_->target_fee_per_kb,
      pending_transaction_->change_amount, pending_transaction_->change_address,
      pending_transaction_->subtract_fee_from_amount);
}

void BitcoinInterface::SignTransactionInputs(
    UnspentList unspent_list, libbitcoin::chain::transaction& output_tx) {
  std::cout << "SignTransactionInputs called" << std::endl;
  for (auto& cur_unspent : unspent_list) {
    const auto& key = cur_unspent.first;
    const auto& transfer = cur_unspent.second;
    if (transfer.is_spent()) {
      // FIXME: Handle without throwing
      throw std::runtime_error("Error: our own utxo has been spent already");
    }

    TxBlockInfo tx_block_info{};
    GetTransactionInfo(transfer.output.hash(), tx_block_info);
    const auto& previous_output_script =
        tx_block_info.tx.outputs()[transfer.output.index()].script();

    const auto address = megabit::utils::bitcoin_address(
        previous_output_script, payment_address_version_);

    // set our signed script on the input, but first find the
    // input's index in the tx we're building
    uint32_t input_index = std::numeric_limits<uint32_t>::max();
    for (uint32_t i = 0; i < output_tx.inputs().size(); i++) {
      if (output_tx.inputs()[i].previous_output() == transfer.output) {
        input_index = i;
        break;
      }
    }

    if (input_index == std::numeric_limits<uint32_t>::max()) {
      // FIXME: Handle without throwing
      throw std::runtime_error(
          "Cannot find our own input to sign in the transaction!");
    }

    // create endorsement for the input index we're about to
    // assign (all inputs must already be added to the tx for
    // this to work)
    libbitcoin::endorsement tx_endorse;
    if (!libbitcoin::chain::script::create_endorsement(
            tx_endorse, key, previous_output_script, output_tx, input_index,
            sighash_type)) {
      // FIXME: Handle without throwing
      throw std::runtime_error("Failed to create tx endorsement");
    }

    // create endorsement script
    const auto pub_key_data = megabit::utils::public_from_private(key);

    std::stringstream script_ss;
    script_ss << "[" << libbitcoin::encode_base16(tx_endorse) << "] [";
    script_ss << libbitcoin::encode_base16(pub_key_data) << "]";

    libbitcoin::chain::script endorsement_script;
    if (!endorsement_script.from_string(script_ss.str())) {
      // FIXME: Handle without throwing
      throw std::runtime_error("failed to create endorsement script");
    }

    // set signed script on the input
    output_tx.inputs()[input_index].set_script(std::move(endorsement_script));

    // validate input
    const auto ret = libbitcoin::chain::script::verify(
        output_tx, input_index, libbitcoin::machine::rule_fork::all_rules);
    if (ret != libbitcoin::error::success) {
      // FIXME: Handle without throwing
      throw std::runtime_error("Signature is invalid");
    }
  }
}

uint64_t BitcoinInterface::GetCalculatedFee(
    const libbitcoin::chain::transaction& tx,
    const uint64_t target_fee_per_kb) {
  const auto tx_size = static_cast<float>(tx.serialized_size());
  return target_fee_per_kb * static_cast<float>(tx_size / 1024);
}

libbitcoin::chain::transaction BitcoinInterface::CreateSignedTransaction(
    const UnspentList& unspent, uint64_t& amount,
    const libbitcoin::wallet::payment_address destination_address,
    const uint64_t target_fee_per_kb, const uint64_t change_amount,
    const libbitcoin::wallet::payment_address change_address,
    bool subtract_fee_from_amount) {
  std::stringstream error_msg;

  libbitcoin::chain::transaction tx;
  tx.set_locktime(locktime);
  tx.set_version(transaction_version);

  libbitcoin::chain::input::list inputs;
  libbitcoin::chain::output::list outputs;

  // add the destination output and change outputs (if any)
  libbitcoin::machine::operation::list amount_payment_ops =
      libbitcoin::chain::script::to_pay_key_hash_pattern(
          destination_address.hash());

  const libbitcoin::chain::output output{
      amount, libbitcoin::chain::script{amount_payment_ops}};

  outputs.push_back(output);

  // add our raw change amount
  if (change_amount) {
    std::cout << "Using change amount of " << change_amount
              << " back to our self" << std::endl;

    libbitcoin::machine::operation::list change_payment_ops =
        libbitcoin::chain::script::to_pay_key_hash_pattern(
            change_address.hash());

    const libbitcoin::chain::output change_output{
        change_amount, libbitcoin::chain::script{change_payment_ops}};

    outputs.push_back(change_output);
  }

  // add all inputs to the tx
  for (const auto& cur_unspent : unspent) {
    const auto& transfer = cur_unspent.second;
    MEGABIT_ASSERT(!transfer.is_spent());

    libbitcoin::chain::input input;
    input.set_sequence(libbitcoin::max_input_sequence);
    input.set_previous_output(
        {transfer.output.hash(), transfer.output.index()});

    inputs.push_back(input);
  }

  tx.set_inputs(inputs);
  tx.set_outputs(outputs);

  // sign all inputs to the tx
  SignTransactionInputs(unspent, tx);

  auto estimated_fee = GetCalculatedFee(tx, target_fee_per_kb);
  std::cout << "Estimated fee for tx of " << tx.serialized_size()
            << " bytes is " << estimated_fee << std::endl;

  auto fee_handled = false;

  // if we are expecting change back, pull the fee from there
  if (change_amount && (change_amount > estimated_fee)) {
    libbitcoin::machine::operation::list change_payment_ops =
        libbitcoin::chain::script::to_pay_key_hash_pattern(
            change_address.hash());

    const uint64_t adjusted_amount = change_amount - estimated_fee;
    const libbitcoin::chain::output change_output{
        adjusted_amount, libbitcoin::chain::script{change_payment_ops}};

    std::cout << "Using adjusted change amount of " << adjusted_amount
              << " back to our self";

    tx.outputs()[1] = change_output;

    fee_handled = true;
  }

  // if the fee hasn't been handled by pulling from the change,
  // if specified, we can pull the fee from the total amount to
  // send (assuming the amount is larger than the estimated fee)
  if (!fee_handled && subtract_fee_from_amount && (amount > estimated_fee)) {
    amount -= estimated_fee;

    std::cout << "Subtracting fee from amount. "
              << "Using adjusted destination amount of" << amount << std::endl;

    const libbitcoin::chain::output output{
        amount, libbitcoin::chain::script{amount_payment_ops}};

    // adjust the amount that was previously already set
    tx.outputs()[0] = output;

    fee_handled = true;
  }

  if (!fee_handled) {
    error_msg << "Not enough funds in this mix level for the amount "
                 "of "
              << amount << " in addition to the estimated fee of "
              << estimated_fee << std::endl;
    throw std::runtime_error(error_msg.str());
  }

  // re-sign all inputs to the tx after fee related modifications
  SignTransactionInputs(unspent, tx);

  return tx;
}

bool BitcoinInterface::CreateAndBroadcastTransaction(
    const UnspentList& unspent, uint64_t& amount,
    const libbitcoin::wallet::payment_address destination_address,
    const uint64_t target_fee_per_kb, const uint64_t change_amount,
    const libbitcoin::wallet::payment_address change_address,
    bool subtract_fee_from_amount) {
  std::stringstream error_msg;

  libbitcoin::chain::transaction tx = CreateSignedTransaction(
      unspent, amount, destination_address, target_fee_per_kb, change_amount,
      change_address, subtract_fee_from_amount);
  /* tx.set_locktime(locktime); */
  /* tx.set_version(transaction_version); */

  /* libbitcoin::chain::input::list inputs; */
  /* libbitcoin::chain::output::list outputs; */

  /* // add the destination output and change outputs (if any) */
  /* libbitcoin::machine::operation::list amount_payment_ops = */
  /*     libbitcoin::chain::script::to_pay_key_hash_pattern( */
  /*         destination_address.hash()); */

  /* const libbitcoin::chain::output output{ */
  /*     amount, libbitcoin::chain::script{amount_payment_ops}}; */

  /* outputs.push_back(output); */

  /* // add our raw change amount */
  /* if (change_amount) { */
  /*   std::cout << "Using change amount of " << change_amount << " back to our
   * self" */
  /*             << std::endl; */

  /*   libbitcoin::machine::operation::list change_payment_ops = */
  /*       libbitcoin::chain::script::to_pay_key_hash_pattern( */
  /*           change_address.hash()); */

  /*   const libbitcoin::chain::output change_output{ */
  /*       change_amount, libbitcoin::chain::script{change_payment_ops}}; */

  /*   outputs.push_back(change_output); */
  /* } */

  /* // add all inputs to the tx */
  /* for (const auto& cur_unspent : unspent) { */
  /*   /\* const auto& key = cur_unspent.first; *\/ */
  /*   const auto& transfer = cur_unspent.second; */
  /*   if (transfer.is_spent()) { */
  /*     return false; */
  /*   } */

  /*   libbitcoin::chain::input input; */
  /*   input.set_sequence(libbitcoin::max_input_sequence); */
  /*   input.set_previous_output( */
  /*       {transfer.output.hash(), transfer.output.index()}); */

  /*   inputs.push_back(input); */
  /* } */

  /* tx.set_inputs(inputs); */
  /* tx.set_outputs(outputs); */

  /* // sign all inputs to the tx */
  /* SignTransactionInputs(unspent, tx); */

  /* auto estimated_fee = GetCalculatedFee(tx, target_fee_per_kb); */
  /* std::cout << "Estimated fee for tx of " << tx_size << " bytes is " */
  /*           << estimated_fee << std::endl; */

  /* auto fee_handled = false; */

  /* // if we are expecting change back, pull the fee from there */
  /* if (change_amount && (change_amount > estimated_fee)) { */
  /*   libbitcoin::machine::operation::list change_payment_ops = */
  /*       libbitcoin::chain::script::to_pay_key_hash_pattern( */
  /*           change_address.hash()); */

  /*   const uint64_t adjusted_amount = change_amount - estimated_fee; */
  /*   const libbitcoin::chain::output change_output{ */
  /*       adjusted_amount, libbitcoin::chain::script{change_payment_ops}}; */

  /*   std::cout << "Using adjusted change amount of " << adjusted_amount */
  /*             << " back to our self"; */

  /*   tx.outputs()[1] = change_output; */

  /*   fee_handled = true; */
  /* } */

  /* // if the fee hasn't been handled by pulling from the change, */
  /* // if specified, we can pull the fee from the total amount to */
  /* // send (assuming the amount is larger than the estimated fee) */
  /* if (!fee_handled && subtract_fee_from_amount && (amount > estimated_fee)) {
   */
  /*   amount -= estimated_fee; */

  /*   std::cout << "Subtracting fee from amount. " */
  /*             << "Using adjusted destination amount of" << amount <<
   * std::endl; */

  /*   const libbitcoin::chain::output output{ */
  /*       amount, libbitcoin::chain::script{amount_payment_ops}}; */

  /*   // adjust the amount that was previously already set */
  /*   tx.outputs()[0] = output; */

  /*   fee_handled = true; */
  /* } */

  /* if (!fee_handled) { */
  /*   error_msg << "Not enough funds in this mix level for the amount " */
  /*                "of " */
  /*             << amount << " in addition to the estimated fee of " */
  /*             << estimated_fee << std::endl; */
  /*   throw std::runtime_error(error_msg.str()); */
  /* } */

  /* // re-sign all inputs to the tx after fee related modifications */
  /* SignTransactionInputs(unspent, tx); */

  return TransactionIsValid(tx) && SendTransaction(tx);
}

bool BitcoinInterface::RetrieveUnspentAndChangeAddress(
    UnspentList& selected_unspent,
    libbitcoin::wallet::payment_address& change_address,
    uint64_t& change_amount, const uint32_t account_index, uint64_t& amount,
    std::vector<std::string>* excluded) {
  std::cout << "Retrieve UnspentAndChangeAddress called with amount " << amount
            << std::endl;
  // retrieve all unspent for this account
  UnspentList unspent_list;
  auto unspent = GetUnspentOutputsForAccountIndex(account_index, unspent_list,
                                                  change_address, excluded);
  MEGABIT_ASSERT(unspent.points.size() == unspent_list.size());

  if (amount > 0) {
    libbitcoin::chain::points_value selected{};
    libbitcoin::wallet::select_outputs::select(selected, unspent, amount);
    change_amount = selected.value() - amount;

    if (!selected.points.size()) {
      std::cout << "Error: Retrieved " << selected.points.size()
                << " selected points from unspent list of size "
                << unspent.points.size() << " for amount " << amount
                << std::endl;
      return false;
    }

    std::cout << "Retrieved " << selected.points.size()
              << " selected points from "
                 "unspent list with change of "
              << change_amount << " BTC" << std::endl;

    // create new unspent_list to only contain the unspent objects
    // matching the selected outputs
    UnspentList selected_unspent_list;
    selected_unspent_list.reserve(selected.points.size());
    for (auto& selected_point : selected.points) {
      libbitcoin::chain::output_point output_point(std::move(selected_point));

      for (const auto& unspent : unspent_list) {
        if (output_point == unspent.second.output) {
          selected_unspent_list.push_back(unspent);
          break;
        }
      }
    }

    if (selected_unspent_list.size() != selected.points.size()) {
      throw std::runtime_error(
          "Failed to select a coherent utxo set (Unconfirmed "
          "transaction or Double spend involved?)");
    }

    // set change amount and swap newly created unspent list with
    // the outgoing parameter
    selected_unspent.swap(selected_unspent_list);

    return selected_unspent.size() == selected.points.size();
  }

  for (const auto& unspent : unspent_list) {
    amount += (!unspent.second.is_spent() ? unspent.second.value : 0);
  }
  selected_unspent.swap(unspent_list);

  std::cout << "Spendable total in mixdepth" << account_index << "is" << amount
            << "btc" << std::endl;

  return (selected_unspent.size() > 0);
}

bool BitcoinInterface::SendTransaction(
    const libbitcoin::chain::transaction& transaction) {
  bool ret = false;

  auto on_done = [&ret](const libbitcoin::code& error) {
    if (!error) {
      std::cout << "Payment has been sent" << std::endl;
      ret = true;
    }
  };

  auto on_error = [](const libbitcoin::code& error) {
    std::cout << "Failed to broadcast transaction " << error.message()
              << std::endl;
  };

  client_.transaction_pool_broadcast(on_error, on_done, transaction);
  client_.wait();

  return ret;
}
