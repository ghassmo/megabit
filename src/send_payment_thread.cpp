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

#include "../include/megabit/send_payment_thread.hpp"

#include <QTimer>

#include "../include/megabit/bitcoin_interface.hpp"

void SendPaymentThread::SendPayment() {
  std::cout << "[THREAD] SendPayment called" << std::endl;

  // NOTE: this is a blocking call, which is why it's in a
  // separate thread
  if (bitcoin_interface_.InitiateSendPayment(
          account_index_, amount_, destination_address_, target_fee_per_kb_,
          subtract_fee_from_amount_)) {
    emit GetUserSendPaymentConfirmation();
  } else {
    emit SendPaymentError(
        tr("Failed to Contruct this Payment transaction.  "
           "Please check the logs for more details"));
  }
}

void SendPaymentThread::OnSendUserConfirmedPayment() {
  std::cout << "SEND PAYMENT THREAD: CALLED ON SEND USER CONFIRMED PAYMENT"
            << std::endl;
  if (!send_called_ && bitcoin_interface_.SendPendingTransaction()) {
    send_called_ = true;
    std::cout << "PAYMENT SENT -- EMITTING FINISHED" << std::endl;
    emit finished();
  } else {
    emit SendPaymentError(
        tr("Failed to Send Payment.  Please check the logs for more details"));
  }
}
