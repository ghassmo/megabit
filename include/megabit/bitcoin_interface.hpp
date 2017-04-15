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

#ifndef __BITCOIN_INTERFACE_HPP
#define __BITCOIN_INTERFACE_HPP

#include <QWidget>
#include <bitcoin/client/obelisk_client.hpp>
#include <functional>
#include <thread>

#include "../include/megabit/constants.hpp"
#include "../include/megabit/utils.hpp"

struct AddressHistory {
  struct Internal {
    Internal(const libbitcoin::chain::output_point& output_pt,
             const size_t o_height,
             const libbitcoin::chain::input_point& spend_pt,
             const size_t s_height, const uint64_t val)
        : output(output_pt),
          output_height(o_height),
          spend(spend_pt),
          spend_height(s_height),
          value(val) {}

    libbitcoin::chain::output_point output;
    size_t output_height;
    libbitcoin::chain::input_point spend;
    size_t spend_height;
    uint64_t value;

    bool is_spent() const { return (spend.hash() != libbitcoin::null_hash); }

    bool confirmed(size_t height) const {
      return (height >= output_height + megabit::constants::num_confirmations);
    }

    bool operator==(const Internal& other) const {
      return (output_height == other.output_height &&
              spend_height == other.spend_height && value == other.value &&
              output == other.output && spend == other.spend);
    }
  };

  using InternalList = std::vector<Internal>;

  uint32_t account_index;
  uint64_t total_value;
  InternalList transfers;

  bool is_spent() const {
    auto ret = false, has_unspent = false;
    for (const auto& transfer : transfers) {
      ret = true;
      has_unspent =
          (transfer.spend.index() == megabit::constants::unspent_index) &&
          (transfer.spend_height == megabit::constants::unspent_height);
      if (has_unspent) {
        ret = false;
        break;
      }
    }
    return ret;
  }

  friend std::ostream& operator<<(std::ostream& out, const AddressHistory& ai) {
    out << "AddressHistory[" /* << ai.account_index << ", " << ai.internal << ",
                                " */
           /* << ai.index << */ "] = "
        << ai.total_value << ", spent? " << (ai.is_spent() ? "true" : "false")
        << std::endl;
    return out;
  }

  bool operator==(const AddressHistory& other) const {
    auto ret = false;
    if (total_value == other.total_value &&
        transfers.size() == other.transfers.size()) {
      ret = true;

      const size_t len = transfers.size();
      for (size_t i = 0; i < len; i++) {
        if (!(transfers[i] == other.transfers[i])) {
          ret = false;
          break;
        }
      }
    }
    return ret;
  }
};

using Seed = libbitcoin::long_hash;
using HDKey = libbitcoin::wallet::hd_private;
using Mnemonic = libbitcoin::wallet::word_list;
using LibbitcoinClient = libbitcoin::client::obelisk_client;

using Unspent = std::pair<libbitcoin::ec_secret, AddressHistory::Internal>;
using UnspentList = std::vector<Unspent>;

using AddressHistoryMap = std::unordered_map<std::string, AddressHistory>;

using KeyList = std::vector<libbitcoin::wallet::hd_private>;
using KeyPair =
    std::pair<libbitcoin::wallet::hd_private, libbitcoin::wallet::hd_private>;
using KeyPairList = std::vector<KeyPair>;

using AddressActivityHandler =
    std::function<void(const libbitcoin::code& error, uint16_t sequence,
                       size_t height, const libbitcoin::hash_digest& tx_hash)>;

using UpdaterFunction =
    std::function<void(uint32_t value_complete, uint32_t value_max)>;

struct TxInfo {
  bool is_spend;
  uint64_t amount;
  size_t height;
  std::string address;
  libbitcoin::hash_digest hash;
  libbitcoin::chain::point point;
  libbitcoin::chain::header header;
  libbitcoin::chain::transaction output;
};

struct TxBlockInfo {
  size_t height;
  size_t index;
  libbitcoin::chain::header header;
  libbitcoin::chain::transaction tx;
};

struct PendingTransaction {
  PendingTransaction(
      const libbitcoin::chain::transaction& _tx,
      const UnspentList& _selected_unspent_list, uint64_t& _amount,
      const libbitcoin::wallet::payment_address _destination_address,
      const uint64_t _target_fee_per_kb, const uint64_t _change_amount,
      const libbitcoin::wallet::payment_address _change_address,
      bool _subtract_fee_from_amount)
      : tx(_tx),
        selected_unspent_list(_selected_unspent_list),
        amount(_amount),
        destination_address(_destination_address),
        target_fee_per_kb(_target_fee_per_kb),
        change_amount(_change_amount),
        change_address(_change_address),
        subtract_fee_from_amount(_subtract_fee_from_amount) {}

  const libbitcoin::chain::transaction tx;
  const UnspentList selected_unspent_list;
  uint64_t amount;
  const libbitcoin::wallet::payment_address destination_address;
  const uint64_t target_fee_per_kb;
  const uint64_t change_amount;
  const libbitcoin::wallet::payment_address change_address;
  bool subtract_fee_from_amount;
};

using TxUpdaterFunction =
    std::function<void(uint32_t account_index, uint32_t value_complete,
                       uint32_t value_max, const TxInfo& tx_info)>;

using ErrorHandler = std::function<void(const libbitcoin::code& error)>;
using BlockHeightHandler = std::function<void(size_t height)>;
using AddressSubscriptionHandler =
    std::function<void(const libbitcoin::code& code)>;

struct AddressBalance {
  uint64_t total_received;
  uint64_t confirmed;
  uint64_t unspent;
};

static constexpr uint32_t locktime = 0;
static constexpr uint32_t script_version = 5;
static constexpr uint32_t transaction_version = 1;
static constexpr auto sighash_type =
    libbitcoin::machine::sighash_algorithm::all;

class BitcoinInterface : public QObject {
  Q_OBJECT

 public:
  BitcoinInterface();

  void SetNetwork(const std::string& network);
  void SetNumAccounts(const size_t num_accounts);
  void SetServerInfo(const std::string& libbitcoin_server_address,
                     const std::string& libbitcoin_server_public_key);

  bool InitializeFromSeed(const Seed& seed);
  /* bool InitializeFromMnemonic(const Mnemonic& mnemonic, */
  /*                             const std::string& passphrase); */

  libbitcoin::wallet::payment_address GetPaymentAddress(
      const std::string address);

  const std::string GetNextAddressForAccount(uint32_t account_index,
                                             uint32_t internal);

  const libbitcoin::data_chunk GetAddressAsPngData(const std::string& address,
                                                   bool prefix = true);

  void GetBlockHeight(ErrorHandler on_error, BlockHeightHandler handler);

  void SetBlockHeight(const size_t block_height);

  const uint64_t GetAccountBalance(bool& error, uint32_t account_index,
                                   TxUpdaterFunction update_fn);

  template <class T>
  const double SatoshiToBtc(T satoshi_amount) {
    return static_cast<double>(static_cast<double>(satoshi_amount) /
                               megabit::constants::satoshi_per_btc);
  }

  // constructs a payment from the utxos in the specified account
  // index
  //
  // if the amount specified is 0, all available funds will be sent
  // (this is used for sweeping everything in the specified
  // account index) and the amount will be returned in amount
  bool InitiateSendPayment(
      const uint32_t account_index, uint64_t& amount,
      const libbitcoin::wallet::payment_address destination_address,
      const uint64_t target_fee_per_kb, bool subtract_fee_from_amount);

  // creates a transaction from the selected unpent outputs
  libbitcoin::chain::transaction CreateSignedTransaction(
      const UnspentList& unspent, uint64_t& amount,
      const libbitcoin::wallet::payment_address destination_address,
      const uint64_t target_fee_per_kb, const uint64_t change_amount,
      const libbitcoin::wallet::payment_address change_address,
      bool subtract_fee_from_amount);

  // creates a transaction from the selected unpent outputs, then
  // signs and broadcasts the transaction to the bitcoin network.
  bool CreateAndBroadcastTransaction(
      const UnspentList& unspent, uint64_t& amount,
      const libbitcoin::wallet::payment_address destination_address,
      const uint64_t target_fee_per_kb, const uint64_t change_amount,
      const libbitcoin::wallet::payment_address change_address,
      bool subtract_fee_from_amount);

  bool SendPendingTransaction();

  bool RetrieveUnspentAndChangeAddress(
      UnspentList& selected_unspent,
      libbitcoin::wallet::payment_address& change_address,
      uint64_t& change_amount, const uint32_t account_index, uint64_t& amount,
      std::vector<std::string>* excluded = nullptr);

  // if unconfirmed is true, the mempool is searched, otherwise the tx
  // store is searched
  bool GetTransactionInfo(const libbitcoin::hash_digest& tx_hash,
                          TxBlockInfo& tx_block_info, bool unconfirmed = false);

  std::shared_ptr<PendingTransaction> GetPendingTransaction() {
    return pending_transaction_;
  }

  uint64_t GetUnconfirmedTransactionAmount(
      const libbitcoin::chain::transaction& tx);

  uint64_t GetCalculatedFee(const libbitcoin::chain::transaction& tx,
                            const uint64_t target_fee_per_kb);

 private:
  const libbitcoin::wallet::hd_private GetKey(uint32_t account,
                                              uint32_t internal,
                                              uint32_t index) const;

  bool GetAddressHistory(const libbitcoin::ec_secret& key,
                         AddressHistory& history);

  bool GetAddressHistory(const std::string& address, AddressHistory& history);

  libbitcoin::chain::points_value GetUnspentOutputsForAccountIndex(
      const uint32_t account_index, UnspentList& unspent_list,
      libbitcoin::wallet::payment_address& change_address,
      std::vector<std::string>* excluded);

  bool GetTransactionInfo(const libbitcoin::hash_digest& tx_hash,
                          libbitcoin::chain::transaction& output_tx_type);

  void SignTransactionInputs(UnspentList unspent_list,
                             libbitcoin::chain::transaction& output_tx);

  bool TransactionIsValid(const libbitcoin::chain::transaction& transaction);

  bool SendTransaction(const libbitcoin::chain::transaction& transaction);

  size_t GetCurrentBlockHeight();

  bool initialized_;
  size_t gap_limit_;
  size_t num_accounts_;
  uint64_t prefixes_;
  uint32_t public_prefix_;
  uint32_t private_prefix_;
  HDKey bip32_root_private_key_;
  LibbitcoinClient client_{megabit::constants::timeout_seconds,
                           megabit::constants::num_retries};
  LibbitcoinClient block_height_client_{megabit::constants::timeout_seconds,
                                        megabit::constants::num_retries};
  std::string libbitcoin_server_address_;
  std::string libbitcoin_server_public_key_;
  size_t block_height_;
  uint8_t payment_address_version_;
  uint32_t bip44_coin_type_;
  std::shared_ptr<PendingTransaction> pending_transaction_;
  std::unordered_set<std::string> external_address_cache_;
  std::unordered_set<std::string> internal_address_cache_;
};

#endif  // __BITCOIN_INTERFACE_HPP
