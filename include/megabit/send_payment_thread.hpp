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

#ifndef __SEND_PAYMENT_THREAD_HPP
#define __SEND_PAYMENT_THREAD_HPP

#include <QThread>

#include "bitcoin_interface.hpp"

class SendPaymentThread : public QObject {
  Q_OBJECT

 public:
  explicit SendPaymentThread(
      BitcoinInterface& bitcoin_interface, const uint32_t account_index,
      uint64_t& amount,
      const libbitcoin::wallet::payment_address destination_address,
      const uint64_t target_fee_per_kb, bool subtract_fee_from_amount)
      : bitcoin_interface_(bitcoin_interface),
        account_index_(account_index),
        amount_(amount),
        destination_address_(destination_address),
        target_fee_per_kb_(target_fee_per_kb),
        subtract_fee_from_amount_(subtract_fee_from_amount),
        send_called_(false) {}

  ~SendPaymentThread() {}

 public slots:
  void SendPayment();
  void OnSendUserConfirmedPayment();

 signals:
  void GetUserSendPaymentConfirmation();
  void finished();
  void SendPaymentError(QString error);

 private:
  BitcoinInterface& bitcoin_interface_;
  uint32_t account_index_;
  uint64_t& amount_;
  libbitcoin::wallet::payment_address destination_address_;
  uint64_t target_fee_per_kb_;
  bool subtract_fee_from_amount_;
  bool send_called_;
};

#endif  // __SEND_PAYMENT_THREAD_HPP
