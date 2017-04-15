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

#ifndef __ADDRESS_MONITOR_THREAD_HPP
#define __ADDRESS_MONITOR_THREAD_HPP

#include <QThread>

#include "bitcoin_interface.hpp"

struct AddressMonitorInfo {
  std::string address;
  libbitcoin::code code;
  uint16_t sequence;
  size_t height;
  libbitcoin::hash_digest tx_hash;
  bool finished;
};

class AddressMonitorThread : public QObject {
  Q_OBJECT

 public:
  explicit AddressMonitorThread(
      const std::string address, const std::string libbitcoin_server_address,
      const std::string libbitcoin_server_public_key,
      std::shared_ptr<AddressMonitorInfo> address_monitor_info)
      : address_(address),
        libbitcoin_server_address_(libbitcoin_server_address),
        libbitcoin_server_public_key_(libbitcoin_server_public_key),
        address_monitor_info_(address_monitor_info),
        subscription_monitor_active_(false) {
    connect(this, SIGNAL(StartMonitor()), this, SLOT(Monitor()));

    address_monitor_info_->code = libbitcoin::error::oversubscribed;
    address_monitor_info_->address = address;
    address_monitor_info_->finished = false;

    bool ret = false;
    if (libbitcoin_server_public_key_.empty()) {
      ret = address_monitor_client_.connect(libbitcoin_server_address_);
    } else {
      ret = address_monitor_client_.connect(
          libbitcoin_server_address_, {},
          libbitcoin::config::sodium(libbitcoin_server_public_key_), {});
    }
    MEGABIT_ASSERT(ret);
  }

  ~AddressMonitorThread() {}

  void MonitorAddressSubscriptions(AddressActivityHandler handler);

 public slots:
  void ListenForAddressChanges();
  void Monitor();
  void CancelMonitor();

 signals:
  void finished();
  void AddressSubscriptionError(QString error);
  void StartMonitor();

 private:
  const std::string address_;
  const std::string libbitcoin_server_address_;
  const std::string libbitcoin_server_public_key_;
  std::shared_ptr<AddressMonitorInfo> address_monitor_info_;
  bool subscription_monitor_active_;
  std::mutex subscription_lock_;
  std::condition_variable subscription_monitor_;
  LibbitcoinClient address_monitor_client_{megabit::constants::timeout_seconds,
                                           megabit::constants::num_retries};
};

#endif  // __ADDRESS_MONITOR_THREAD_HPP
