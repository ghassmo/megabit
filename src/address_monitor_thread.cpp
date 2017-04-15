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

#include "../include/megabit/address_monitor_thread.hpp"

#include <QTimer>

#include "../include/megabit/bitcoin_interface.hpp"

void AddressMonitorThread::ListenForAddressChanges() {
  std::cout << "[THREAD: " << this << "] ListenForAddressChanges called with "
            << address_ << std::endl;

  auto on_error = [this](const libbitcoin::code& code) {
    address_monitor_info_->code = code;
    address_monitor_info_->finished = true;
    if (code && (code != libbitcoin::error::oversubscribed)) {
      std::stringstream error_ss;
      error_ss << "Failed to subscribe to ";
      error_ss << address_ << ": " << code;
      std::cout << error_ss.str() << std::endl;
      emit AddressSubscriptionError(QString::fromUtf8(error_ss.str().c_str()));
    }
  };

  auto on_subscribed = [this](const libbitcoin::code& code) {
    address_monitor_info_->code = code;
    address_monitor_info_->finished = true;
    std::cout << "got subscription code: " << code << std::endl;
    if (code == libbitcoin::error::success) {
      std::cout << "[thread: " << this << "] calling monitor :-) " << std::endl;
      QMetaObject::invokeMethod(this, "Monitor", Qt::QueuedConnection);
    } else {
      std::stringstream error_ss;
      error_ss << "Error: Subscribe to ";
      error_ss << address_ << ": " << code;
      std::cout << error_ss.str() << std::endl;
      std::cout << "[thread: " << this << "] calling subscription error (error "
                << code << ") :-(" << std::endl;
      emit AddressSubscriptionError(QString::fromUtf8(error_ss.str().c_str()));
    }
  };

  address_monitor_client_.subscribe_address(
      on_error, on_subscribed, libbitcoin::wallet::payment_address(address_));
  address_monitor_client_.wait();
}

void AddressMonitorThread::Monitor() {
  std::cout << "address monitor thread :: monitor called" << std::endl;
  auto subscription_update_handler = [this](const libbitcoin::code& code,
                                            uint16_t sequence, size_t height,
                                            const libbitcoin::hash_digest&
                                                tx_hash) {
    std::cout << "Monitor::subscription_update_handler called with " << code
              << std::endl;
    std::cout << "[0] subscription handler got code: " << code
              << ", height: " << height << ", seq: " << sequence
              << ", tx_hash: " << libbitcoin::encode_base16(tx_hash)
              << std::endl;

    /* if (code == libbitcoin::error::success) { */
    /*   if (height == 0) { */
    /*     std::cout << "[SEQ 1] GOT A FAKE CONFIRM, DOING NOTHING" <<
     * std::endl; */
    /*     // FIXME: This is real -- just need to bitcoin reverse the hash */
    /*     // e.g.
     * 167223e9291404f029336fb61a6c6a1ac34837163a1c0aeebae72cd2f2ab5d66 */
    /*     //   ->
     * 665dabf2d22ce7baee0a1c3a163748c31a6a6c1ab66f3329f0041429e9237216 */

    /*     std::cout << "GOT HASH: " << libbitcoin::encode_base16(tx_hash); */
    /*     auto reversed_tx_hash = tx_hash; */
    /*     std::reverse(reversed_tx_hash.begin(), reversed_tx_hash.end()); */
    /*     std::cout << "REVERSED HASH: " <<
     * libbitcoin::encode_base16(reversed_tx_hash); */

    /*     // Notify user that an unconfirmed tx was detected and an */
    /*     // update will occur when a confirmation is received */
    /*     // emit DetectedUnconfirmed(); */
    /*   } else { */
    /*     std::cout << "GOT A REAL CONFIRM, STORING INFO" << std::endl; */
    /*     if (height != address_monitor_info_->height) { */
    /*       address_monitor_info_->code = code; */
    /*       address_monitor_info_->sequence = sequence; */
    /*       address_monitor_info_->height = height; */
    /*       address_monitor_info_->tx_hash = tx_hash; */
    /*       address_monitor_info_->finished = true; */

    /*       std::cout << "GOT A REAL CONFIRM, CALLING CANCEL MONITOR" <<
     * std::endl; */
    /*       CancelMonitor(); */
    /*       if (address_monitor_client_.empty()) { */
    /*           std::cout << "MONITOR IS COMPLETE AND NOTHING IS WAITING --
     * EMITTING FINISHED" << std::endl; */
    /*           QTimer::singleShot(100, this, [this]() { emit finished(); });
     */
    /*       } else { */
    /*           std::cout << "MONITOR IS NOT COMPLETE -- WAITING FOR CANCEL TO
     * COMPLETE" << std::endl; */
    /*       } */
    /*     } else if (code == libbitcoin::error::oversubscribed) { */
    /*       std::cout << "MONITOR IS REALLY COMPLETE -- EMITTING FINISHED" <<
     * std::endl; */
    /*       QTimer::singleShot(100, this, [this]() { emit finished(); }); */
    /*     } */
    /*   } */
    /* } else if ((code == libbitcoin::error::channel_timeout) && */
    /*            subscription_monitor_active_) { */
    /*     // re-subscribe and start this monitoring  process over */
    /*     std::cout << "RE-SUBSCRIBING AFTER CHANNEL TIMEOUT" << std::endl; */
    /*     CancelMonitor(); */
    /*     QTimer::singleShot(100, this, SLOT(ListenForAddressChanges())); */
    /* } */

    if (code == libbitcoin::error::success) {
      if (height == 0) {
        address_monitor_info_->code = code;
        address_monitor_info_->sequence = sequence;
        address_monitor_info_->height = height;
        address_monitor_info_->tx_hash = tx_hash;
        address_monitor_info_->finished = true;

        /* std::cout << "ABOUT TO DO IMMEDIATE EXEC" << std::endl; */
        /* auto on_error = [](const libbitcoin::code& error) { */
        /*     std::cerr << "INNER TX ERROR: " << error.message() << std::endl;
         */
        /* }; */

        /* auto on_done = [](const libbitcoin::chain::transaction& /\* tx *\/) {
         */
        /*     std::cout << "TX DONE CALLED AND GOT TX INFO" << std::endl; */
        /* }; */

        /* LibbitcoinClient client{ */
        /*     megabit::constants::timeout_seconds, */
        /*         megabit::constants::num_retries}; */

        /* client.connect( */
        /*     libbitcoin_server_address_, {}, */
        /*     libbitcoin::config::sodium(libbitcoin_server_public_key_),{}); */

        /* client.transaction_pool_fetch_transaction(on_error, on_done,
         * tx_hash); */
        /* client.wait(); */
        /* std::cout << "DONE DOING IMMEDIATE EXEC" << std::endl; */

        std::cout << "trying the fake confirm, calling cancel monitor"
                  << std::endl;
        CancelMonitor();
        if (address_monitor_client_.empty()) {
          // FIXME: ??? TX NOT SHOWING? CALL DIRECTLY?
          /* // NOTE: We wait 10 seconds because libbitcoin doesn't seem */
          /* // to index information about this tx hash immediately, so */
          /* // if we try to get the transaction info right away, it */
          /* // failed with "object does not exist" error. */
          std::cout << "monitor is complete and nothing is waiting -- emitting "
                       "finished"
                    << std::endl;
          QTimer::singleShot(5000, this, [this]() { emit finished(); });
        } else {
          std::cout
              << "monitor is not complete -- waiting for cancel to complete"
              << std::endl;
        }

      } else {
        std::cout << "got a real confirm, storing info" << std::endl;
        if (height != address_monitor_info_->height) {
          address_monitor_info_->code = code;
          address_monitor_info_->sequence = sequence;
          address_monitor_info_->height = height;
          address_monitor_info_->tx_hash = tx_hash;
          address_monitor_info_->finished = true;

          std::cout << "got a real confirm, calling cancel monitor"
                    << std::endl;
          CancelMonitor();
          if (address_monitor_client_.empty()) {
            std::cout << "monitor is complete and nothing is waiting -- "
                         "emitting finished"
                      << std::endl;
            QTimer::singleShot(100, this, [this]() { emit finished(); });
          } else {
            std::cout
                << "monitor is not complete -- waiting for cancel to complete"
                << std::endl;
          }
        } else if (code == libbitcoin::error::oversubscribed) {
          std::cout << "monitor is really complete -- emitting finished"
                    << std::endl;
          QTimer::singleShot(100, this, [this]() { emit finished(); });
        }
      }
    } else if ((code == libbitcoin::error::channel_timeout) &&
               subscription_monitor_active_) {
      // re-subscribe and start this monitoring  process over
      std::cout << "re-subscribing after channel timeout" << std::endl;
      CancelMonitor();
      QTimer::singleShot(100, this, SLOT(ListenForAddressChanges()));
    }
  };

  std::cout << "AddressMonitorThread::Monitor -- Monitoring NOW..."
            << std::endl;

  // NOTE: this is a blocking call, which is why it's in a
  // separate thread
  MEGABIT_ASSERT(!subscription_monitor_active_);
  address_monitor_client_.set_on_update(subscription_update_handler);
  subscription_monitor_active_ = true;

  while (subscription_monitor_active_) {
    std::cout << "[" << this << "] Monitoring address subscription ["
              << address_ << "]" << std::endl;
    address_monitor_client_.monitor(60);
  }

  if (!address_monitor_client_.empty()) {
    address_monitor_client_.clear(libbitcoin::error::make_error_code(
        libbitcoin::error::error_code_t::oversubscribed));
  }

  std::cout << "about to notify one" << std::endl;
  subscription_monitor_.notify_one();

  std::cout << "Done Monitoring address subscription ..." << std::endl;
}

void AddressMonitorThread::CancelMonitor() {
  std::cout << "Trying to cancel monitor" << std::endl;
  subscription_monitor_active_ = false;
  std::unique_lock<std::mutex> lock(subscription_lock_);
  subscription_monitor_.wait(
      lock, [this]() { return !subscription_monitor_active_; });
  std::cout << "Cancel monitor -- DONE" << std::endl;
}
