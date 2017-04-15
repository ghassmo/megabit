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

#ifndef __MEGABIT_HPP
#define __MEGABIT_HPP

#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QNetworkReply>
#include <QPixmap>
#include <QProgressDialog>
#include <QPushButton>
#include <QSplashScreen>
#include <QTableWidgetItem>
#include <QTemporaryFile>
#include <boost/smart_ptr/detail/spinlock.hpp>
#include <unordered_map>

#include "address_monitor_thread.hpp"
#include "bitcoin_interface.hpp"
#include "block_height_thread.hpp"
#include "send_payment_thread.hpp"
#include "settings.hpp"

#define safe_delete(x) \
  if (x) delete x
#define safe_delete_later(x) \
  if (x) x->deleteLater()

class CreateWalletWizard;

namespace Ui {
class Megabit;
}

struct Configuration {
  bool first_time;
  size_t num_accounts;
  QStringList account_names;
  QString network;
  QString checksum;
  QString currency;
  QString creation_time;
  QString encrypted_seed;
  QString server_address;
  QString server_public_key;
  uint64_t low_fee_per_kb;
  uint64_t medium_fee_per_kb;
  uint64_t high_fee_per_kb;
  size_t current_tab_index;
  size_t current_account_index;
  std::unordered_map<std::string, double> currency_value_map;
};

struct AddressMonitorObj {
  QThread* address_monitor_thread;
  AddressMonitorThread* address_monitor_worker;
  std::shared_ptr<AddressMonitorInfo> address_monitor_info;
};

class Megabit : public QMainWindow {
  Q_OBJECT

 public:
  explicit Megabit(QWidget* parent = Q_NULLPTR);
  ~Megabit();

  void closeEvent(QCloseEvent* event);

  void RefreshTransactions(bool skip_refresh);

 private slots:
  void ShowAbout();
  void ShowSettings();

  void GetFeeData();
  void OnFeeDataRead();

  void GetCurrencyData();
  void OnCurrencyDataRead();

  void ShowWalletLoader();
  void OnWalletLoaded();

  void OnAccountNameEdited(int row, int column);

  void GetBlockHeight();
  void OnGetBlockHeight();
  void OnBlockHeightError(QString error);

  void SendPayment();
  void OnPaymentSent();

  void AddAccount();

  void MonitorAddress(std::string address);
  void OnMonitorAddressComplete();
  void OnAddressSubscriptionError(QString error);

  void RefreshTransactions();

  void OnProgressUpdated(uint32_t account_index, uint32_t row,
                         const TxInfo tx_info);

  void OnAmountEdited(const QString& text);
  void OnAmountBTCEdited(const QString& text);

  void OnFeeAdjusted(int value);

  void OnRecipientAddressEdited(const QString& address);

  void OnSendAccountUpdated(int index);

  void OnReceiveAccountUpdated(int index);

  void OnUseAccountMax();

  void OnSubtractFeeFromAmountChecked();

  void OnCopyReceiveAddress();

  void OnGetUserSendPaymentConfirmation();
  void OnSendPaymentError(QString error);

 signals:
  void finished(std::string address);
  void WalletLoaded();
  void ProgressUpdated(uint32_t account_index, uint32_t row,
                       const TxInfo tx_info);
  void SendUserConfirmedPayment();

 private:
  void ShowSplashScreen();
  void LoadConfiguration(Configuration& config);
  void SaveConfiguration(const Configuration& config);
  double GetConvertedCurrencyAmount(const Configuration& config,
                                    uint64_t btc_amount, std::string currency);

  bool LoadAccountsTab(Configuration& config);
  void LoadSendTab(Configuration& config);
  void LoadReceiveTab(Configuration& config, uint32_t account_index,
                      bool set_index = true);

  void UpdateFeeLabel();

  void ClearPaymentFields();

  void AddressSubscriptionHandler(const libbitcoin::code& error,
                                  uint16_t sequence, size_t height,
                                  const libbitcoin::hash_digest& tx_hash);

  void AddUnconfirmedTransaction(const std::string address,
                                 const libbitcoin::chain::transaction& tx);

  QLabel* status;
  Ui::Megabit* ui;
  CreateWalletWizard* wizard_;
  QNetworkReply* fee_reply_;
  QNetworkReply* currency_reply_;
  QProgressDialog* wallet_loader_dialog_;
  QSplashScreen* splash_screen_;

  // receive tab related members
  QLabel* receive_label_;
  QComboBox* receive_combo_;
  QLayout* receive_tab_layout_;
  QLabel* receive_image_label_;
  QLabel* receive_address_label_;
  QLabel* receive_spacing_label_;
  QLineEdit* receive_address_edit_;
  QPushButton* copy_receive_address_;

  double current_fee_;
  size_t block_height_;
  size_t refresh_block_height_;
  bool payment_status_;
  Configuration config_;
  BitcoinInterface bitcoin_interface_;
  QNetworkAccessManager network_manager_;

  std::vector<QTemporaryFile*> qrcode_image_files_;
  std::unordered_map<uint32_t, uint64_t> account_balance_map_;

  boost::detail::spinlock address_monitor_map_lock_;
  std::unordered_map<std::string, std::shared_ptr<AddressMonitorObj>>
      address_monitor_map_;
};

#endif  // __MEGABIT_HPP
