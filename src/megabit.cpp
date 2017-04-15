/*
 * This file is part of Megabit, a BIP44 HD wallet built on
 * libbitcoin.
 *
 * Copyright (C) 2017-2019 Neill Miller (neillm@thecodefactory.org)
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

#include "../include/megabit/megabit.hpp"

#include <QByteArray>
#include <QClipboard>
#include <QFont>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QProgressDialog>
#include <QSettings>
#include <QTcpSocket>
#include <QTimer>
#include <QVBoxLayout>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <iostream>
#include <vector>

#include "../include/megabit/bitcoin_interface.hpp"
#include "../include/megabit/constants.hpp"
#include "../include/megabit/createwalletwizard.hpp"
#include "../include/megabit/utils.hpp"
#include "ui_megabit.h"

Megabit::Megabit(QWidget* parent)
    : QMainWindow(parent),
      ui(new Ui::Megabit),
      wizard_(nullptr),
      fee_reply_(nullptr),
      currency_reply_(nullptr),
      wallet_loader_dialog_(nullptr),
      bitcoin_interface_() {
  ui->setupUi(this);

  receive_combo_ = nullptr;
  receive_label_ = nullptr;
  receive_image_label_ = nullptr;
  receive_address_label_ = nullptr;
  receive_spacing_label_ = nullptr;
  receive_tab_layout_ = nullptr;
  receive_address_edit_ = nullptr;
  copy_receive_address_ = nullptr;

  address_monitor_map_lock_.unlock();

  refresh_block_height_ = 0;

  // start timers for external service related events
  QTimer::singleShot(10, this, SLOT(GetFeeData()));
  QTimer::singleShot(20, this, SLOT(GetCurrencyData()));

  connect(ui->refreshButton, SIGNAL(released()), this,
          SLOT(RefreshTransactions()));

  connect(ui->subtract_fee, SIGNAL(clicked()), this,
          SLOT(OnSubtractFeeFromAmountChecked()));

  connect(ui->add_account, SIGNAL(clicked()), this, SLOT(AddAccount()));

  connect(ui->send_transaction, SIGNAL(clicked()), this, SLOT(SendPayment()));

  connect(this, SIGNAL(ProgressUpdated(uint32_t, uint32_t, const TxInfo)), this,
          SLOT(OnProgressUpdated(uint32_t, uint32_t, const TxInfo)));

  connect(this, SIGNAL(WalletLoaded()), this, SLOT(OnWalletLoaded()));

  connect(ui->actionPreferences, SIGNAL(triggered()), this,
          SLOT(ShowSettings()));

  connect(ui->actionAbout_Megabit, SIGNAL(triggered()), this,
          SLOT(ShowAbout()));

  ShowSplashScreen();
  LoadConfiguration(config_);
}

Megabit::~Megabit() {
  {
    std::lock_guard<boost::detail::spinlock> lock(address_monitor_map_lock_);
    for (const auto& monitor_iter : address_monitor_map_) {
      std::cout << "canceling monitor for " << monitor_iter.first << std::endl;
      monitor_iter.second->address_monitor_worker->CancelMonitor();
    }
  }

  safe_delete(wizard_);
  safe_delete(wallet_loader_dialog_);

  safe_delete_later(fee_reply_);
  safe_delete_later(currency_reply_);

  for (auto file : qrcode_image_files_) {
    QFile::remove(file->fileName());
    delete file;
  }

  delete receive_combo_;
  delete receive_label_;
  delete receive_address_label_;
  delete receive_tab_layout_;
  delete receive_image_label_;
  delete receive_spacing_label_;

  delete ui;
}

void Megabit::ShowSplashScreen() {
  splash_screen_ =
      new QSplashScreen(this, QPixmap(":/megabit-placeholder.png"));
  splash_screen_->show();
  splash_screen_->showMessage("Megabit loading ...");
}

void Megabit::LoadConfiguration(Configuration& config) {
  if (splash_screen_) splash_screen_->showMessage("Loading Configuration ...");
  QSettings settings("TheCodeFactory", "Megabit");
  config.current_tab_index = 0;
  config.first_time = settings.value("global/first_time", 1).toInt();
  if (config.first_time) {
    if (splash_screen_) splash_screen_->finish(this);

    wizard_ = new CreateWalletWizard(this);
    wizard_->show();
  } else {
    config.server_address =
        settings
            .value("global/libbitcoin_server_address",
                   QString::fromStdString(
                       megabit::constants::libbitcoin_server_address))
            .toString();
    config.server_public_key =
        settings
            .value("global/libbitcoin_server_public_key",
                   QString::fromStdString(
                       megabit::constants::libbitcoin_server_public_key))
            .toString();
    config.num_accounts = settings.value("accounts/numAccounts", 1).toInt();
    for (size_t i = 0; i < config.num_accounts; i++) {
      const auto str_index = QString::number(i);
      const auto account_index = QString::number(i + 1);
      const auto account_name = settings
                                    .value("accounts/" + str_index + "/name",
                                           "Account " + account_index)
                                    .toString();
      config.account_names << account_name;
    }

    config.current_account_index =
        settings
            .value("global/default_account_index",
                   QString::number(megabit::constants::default_account_index))
            .toInt();
    config.currency = settings
                          .value("global/currency",
                                 megabit::constants::default_currency.c_str())
                          .toString();
    config.checksum = settings.value("global/checksum").toString();
    config.encrypted_seed = settings.value("global/encrypted_seed").toString();
    config.network = settings.value("global/network", "mainnet").toString();

    auto entered = false;
    QString passphrase_str =
        QInputDialog::getText(this, tr("Passphrase Required"),
                              tr("To access your Megabit wallet, your "
                                 "passphrase is required:"),
                              QLineEdit::Password, "", &entered);

    if (entered && !passphrase_str.isEmpty()) {
      libbitcoin::hash_digest passphrase_hash;
      megabit::utils::mem_lock_region(passphrase_hash);
      megabit::utils::get_passphrase_key(passphrase_hash,
                                         passphrase_str.toStdString());
      passphrase_str = "";

      libbitcoin::long_hash seed;
      megabit::utils::mem_lock_region(seed);
      libbitcoin::decode_base16(seed, config.encrypted_seed.toStdString());

      libbitcoin::data_chunk checksum;
      megabit::utils::mem_lock_region(checksum);
      libbitcoin::decode_base16(checksum, config.checksum.toStdString());

      if (!megabit::utils::get_user_wallet_seed(seed, checksum,
                                                passphrase_hash)) {
        if (splash_screen_) splash_screen_->finish(this);
        QMessageBox::information(
            const_cast<decltype(this)>(this), tr("Passphrase is incorrect"),
            tr("Error: The passphrase entered is incorrect. "
               "Please restart the application and make sure "
               "the passphrase has been properly entered."));

        exit(1);
      }

      bitcoin_interface_.SetNumAccounts(config.num_accounts);
      bitcoin_interface_.SetNetwork(config.network.toStdString());
      bitcoin_interface_.SetServerInfo(config.server_address.toStdString(),
                                       config.server_public_key.toStdString());

      bitcoin_interface_.InitializeFromSeed(seed);

      megabit::utils::mem_unlock_region(checksum);
      megabit::utils::mem_unlock_region(passphrase_hash);
      megabit::utils::mem_unlock_region(seed);

      if (!LoadAccountsTab(config_)) {
        if (splash_screen_) splash_screen_->finish(this);

        QMessageBox::information(
            const_cast<decltype(this)>(this), tr("Account Data Failure"),
            tr("Error: Could not load account information. "
               "Please restart the application and make sure "
               "that the server settings are correct."));

        exit(1);
      }

      LoadSendTab(config_);
      LoadReceiveTab(config_, megabit::constants::default_account_index);

      if (splash_screen_) splash_screen_->finish(this);
      emit WalletLoaded();
    } else {
      if (splash_screen_) splash_screen_->finish(this);
      exit(1);
    }
  }
}

void Megabit::SaveConfiguration(const Configuration& config) {
  std::cout << "Saving Configuration ... " << std::endl;
  QSettings settings("TheCodeFactory", "Megabit");
  settings.setValue("accounts/numAccounts/",
                    QString::number(config.num_accounts));

  for (size_t i = 0; i < config.num_accounts; i++) {
    const auto str_index = QString::number(i);
    settings.setValue("accounts/" + str_index + "/name",
                      config.account_names.at(i));
  }

  settings.setValue("global/default_account_index",
                    QString::number(config.current_account_index));
  settings.setValue("global/currency", config.currency);
  settings.setValue("global/libbitcoin_server_address", config.server_address);
  settings.setValue("global/libbitcoin_server_public_key",
                    config.server_public_key);
}

bool Megabit::LoadAccountsTab(Configuration& config_) {
  ui->accountsTable->setColumnCount(3);
  ui->accountsTable->setRowCount(config_.num_accounts);
  ui->accountsTable->setHorizontalHeaderLabels(
      {"Account Name", "Balance (BTC)", "Balance (" + config_.currency + ")"});

  for (int i = 0; i < ui->accountsTable->horizontalHeader()->count(); i++) {
    ui->accountsTable->horizontalHeader()->setSectionResizeMode(
        i, QHeaderView::Stretch);
  }

  ui->transactionTable->setColumnCount(5);
  ui->transactionTable->setRowCount(0);
  ui->transactionTable->verticalHeader()->setVisible(false);
  ui->transactionTable->setHorizontalHeaderLabels(
      {"Date", "Type", "Label", "Amount (BTC)",
       "Amount (" + config_.currency + ")"});

  for (int i = 0; i < ui->transactionTable->horizontalHeader()->count(); i++) {
    ui->transactionTable->horizontalHeader()->setSectionResizeMode(
        i, QHeaderView::Stretch);
  }

  wallet_loader_dialog_ = nullptr;
  for (size_t i = 0; i < config_.num_accounts; i++) {
    const auto account_name_str = config_.account_names.at(i);
    // NOTE: these table widget items are automatically
    // de-allocated by the parent container when they are no
    // longer needed (i.e. when the Refresh clears all items).
    // Trying to track them and manually delete them is a bug
    QTableWidgetItem* account_name = new QTableWidgetItem(account_name_str);

    connect(ui->accountsTable, SIGNAL(cellChanged(int, int)), this,
            SLOT(OnAccountNameEdited(int, int)));

    auto progress_updater = [this, &account_name_str, i](
                                uint32_t account_index, uint32_t value_complete,
                                uint32_t value_max, const TxInfo& tx_info) {
      if (!wallet_loader_dialog_) {
        ShowWalletLoader();
      }
      QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

      if (splash_screen_)
        splash_screen_->showMessage("Loading " + account_name_str + " ...");

      wallet_loader_dialog_->setLabelText("Loading " + account_name_str +
                                          " address information ...");
      wallet_loader_dialog_->setMaximum(value_max);
      wallet_loader_dialog_->setValue(value_complete);

      QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
      emit ProgressUpdated(account_index, value_complete, tx_info);
    };

    bool error = false;
    uint64_t balance =
        bitcoin_interface_.GetAccountBalance(error, i, progress_updater);
    if (error) return false;

    account_balance_map_[i] = balance;
    QString balance_str;
    balance_str.setNum(bitcoin_interface_.SatoshiToBtc(balance), 'f', 8);
    QTableWidgetItem* account_balance_btc = new QTableWidgetItem(balance_str);
    account_balance_btc->setTextAlignment(Qt::AlignVCenter);
    account_balance_btc->setFlags(account_balance_btc->flags() &
                                  ~Qt::ItemIsEditable);

    auto converted_balance = GetConvertedCurrencyAmount(
        config_, balance, config_.currency.toStdString());
    QString converted_balance_str;
    converted_balance_str.setNum(
        bitcoin_interface_.SatoshiToBtc(converted_balance), 'f', 2);
    QTableWidgetItem* account_balance_converted =
        new QTableWidgetItem(converted_balance_str);
    account_balance_converted->setFlags(account_balance_converted->flags() &
                                        ~Qt::ItemIsEditable);

    account_balance_converted->setTextAlignment(Qt::AlignVCenter);
    ui->accountsTable->setItem(i, 0, account_name);
    ui->accountsTable->setItem(i, 1, account_balance_btc);
    ui->accountsTable->setItem(i, 2, account_balance_converted);
  }

  ui->tab_main->setCurrentIndex(config_.current_tab_index);
  return true;
}

void Megabit::LoadSendTab(Configuration& config) {
  static QString estimate_str =
      "Estimate is based on a 1K transaction size and may adjust "
      "after the transaction is created";

  connect(ui->account_selector, SIGNAL(currentIndexChanged(int)), this,
          SLOT(OnSendAccountUpdated(int)));

  ui->account_selector->clear();
  ui->account_selector->addItems(config.account_names);
  ui->account_selector->setCurrentIndex(config.current_account_index);

  ui->amountLabel->setText("Amount (" + config.currency + ")");
  ui->totalSpentLabel->setText("Estimated Total Spent (" + config.currency +
                               ")");

  ui->totalSpentLabel->setToolTip(estimate_str);
  ui->totalSpentLineEdit->setToolTip(estimate_str);
  ui->totalSpentBTCLabel->setToolTip(estimate_str);
  ui->totalSpentBTCLineEdit->setToolTip(estimate_str);

  connect(ui->amountLineEdit, SIGNAL(textEdited(const QString&)), this,
          SLOT(OnAmountEdited(const QString&)));
  connect(ui->amountBTCLineEdit, SIGNAL(textEdited(const QString&)), this,
          SLOT(OnAmountBTCEdited(const QString&)));

  current_fee_ = config_.low_fee_per_kb;
  ui->fee_slider->setTickPosition(QSlider::TicksBelow);
  connect(ui->fee_slider, SIGNAL(sliderMoved(int)), this,
          SLOT(OnFeeAdjusted(int)));

  ui->send_transaction->setEnabled(false);

  connect(ui->recipientBitcoinAddressLineEdit,
          SIGNAL(textEdited(const QString&)), this,
          SLOT(OnRecipientAddressEdited(const QString&)));

  connect(ui->useAccountMaxButton, SIGNAL(clicked()), this,
          SLOT(OnUseAccountMax()));

  UpdateFeeLabel();
}

void Megabit::OnAmountEdited(const QString& text) {
  // amount is in the user set currency
  const auto amount = text.toDouble();
  const auto conversion_factor =
      GetConvertedCurrencyAmount(config_, 1, config_.currency.toStdString());
  const auto converted_balance = (amount / conversion_factor);

  auto fee = (ui->subtract_fee->isChecked() ? 0 : current_fee_);
  const auto converted_balance_with_fee =
      converted_balance + (fee / megabit::constants::satoshi_per_btc);

  QString converted_balance_str;
  converted_balance_str.setNum(converted_balance, 'f', 8);
  QString converted_balance_with_fee_str;
  converted_balance_with_fee_str.setNum(converted_balance_with_fee, 'f', 8);

  QString converted_balance_currency_with_fee_str;
  converted_balance_currency_with_fee_str.setNum(
      converted_balance_with_fee * conversion_factor, 'f', 2);

  ui->amountBTCLineEdit->setText(converted_balance_str);
  ui->totalSpentBTCLineEdit->setText(converted_balance_with_fee_str);
  ui->totalSpentLineEdit->setText(converted_balance_currency_with_fee_str);
}

void Megabit::OnAmountBTCEdited(const QString& text) {
  // amount is in BTC
  const auto amount = text.toDouble() * megabit::constants::satoshi_per_btc;
  const auto converted_balance = GetConvertedCurrencyAmount(
      config_, amount, config_.currency.toStdString());

  auto fee = (ui->subtract_fee->isChecked() ? 0 : current_fee_);
  const auto converted_balance_with_fee = GetConvertedCurrencyAmount(
      config_, amount + fee, config_.currency.toStdString());

  QString converted_balance_str;
  converted_balance_str.setNum(
      bitcoin_interface_.SatoshiToBtc(converted_balance), 'f', 2);
  QString converted_balance_with_fee_str;
  converted_balance_with_fee_str.setNum(
      bitcoin_interface_.SatoshiToBtc(converted_balance_with_fee), 'f', 2);

  QString converted_balance_currency_with_fee_str;
  converted_balance_currency_with_fee_str.setNum(
      ((amount + fee) / megabit::constants::satoshi_per_btc), 'f', 8);
  ui->amountLineEdit->setText(converted_balance_str);
  ui->totalSpentLineEdit->setText(converted_balance_with_fee_str);
  ui->totalSpentBTCLineEdit->setText(converted_balance_currency_with_fee_str);
}

void Megabit::OnFeeAdjusted(int value) {
  const auto fee_range = config_.high_fee_per_kb - config_.low_fee_per_kb;
  current_fee_ =
      config_.low_fee_per_kb +
      (fee_range * (static_cast<double>((value == 0) ? 0 : (value + 1)) / 100));

  if (!ui->subtract_fee->isChecked()) {
    const auto amount = ui->amountBTCLineEdit->text().toDouble() *
                        megabit::constants::satoshi_per_btc;
    const auto converted_balance_with_fee = GetConvertedCurrencyAmount(
        config_, amount + current_fee_, config_.currency.toStdString());

    QString converted_balance_with_fee_str;
    converted_balance_with_fee_str.setNum(
        bitcoin_interface_.SatoshiToBtc(converted_balance_with_fee), 'f', 2);

    QString converted_balance_currency_with_fee_str;
    converted_balance_currency_with_fee_str.setNum(
        ((amount + current_fee_) / megabit::constants::satoshi_per_btc), 'f',
        8);

    ui->totalSpentLineEdit->setText(converted_balance_with_fee_str);
    ui->totalSpentBTCLineEdit->setText(converted_balance_currency_with_fee_str);
  }
}

void Megabit::OnRecipientAddressEdited(const QString& address) {
  auto addr = bitcoin_interface_.GetPaymentAddress(address.toStdString());
  auto is_valid_address = addr.encoded() == address.toStdString();
  ui->send_transaction->setEnabled(is_valid_address);
}

void Megabit::OnSendAccountUpdated(int index) {
  const auto account_index = (index == -1 ? 0 : index);
  config_.current_account_index = index;
  uint64_t balance = account_balance_map_[account_index];
  QString balance_str;
  balance_str.setNum(bitcoin_interface_.SatoshiToBtc(balance), 'f', 8);
  ui->useAccountMaxButton->setText("Use Account Max (" + balance_str + " BTC)");
}

void Megabit::OnReceiveAccountUpdated(int index) {
  if (index != -1) {
    std::cout << "OnReceiveAccountUpdated to " << index << std::endl;
    LoadReceiveTab(config_, index, false);
  }
}

void Megabit::OnUseAccountMax() {
  // if account max was selected, auto-check the subtract fee from
  // amount option and auto-populate the BTC amount
  ui->subtract_fee->setChecked(true);
  uint64_t balance = account_balance_map_[config_.current_account_index];
  QString balance_str;
  balance_str.setNum(bitcoin_interface_.SatoshiToBtc(balance), 'f', 8);
  ui->amountBTCLineEdit->setText(balance_str);
  OnAmountBTCEdited(balance_str);
}

void Megabit::OnSubtractFeeFromAmountChecked() {
  OnAmountBTCEdited(ui->amountBTCLineEdit->text());
}

void Megabit::UpdateFeeLabel() {
  QString low_fee_str;
  low_fee_str.setNum(bitcoin_interface_.SatoshiToBtc(config_.low_fee_per_kb),
                     'f', 8);

  QString high_fee_str;
  high_fee_str.setNum(bitcoin_interface_.SatoshiToBtc(config_.high_fee_per_kb),
                      'f', 8);

  QString range_str = tr("(Recommended fee range per KB is\n") + low_fee_str +
                      " to " + high_fee_str + " BTC)";

  ui->transaction_fee_range->setText(range_str);
}

void Megabit::AddressSubscriptionHandler(
    const libbitcoin::code& error, uint16_t sequence, size_t height,
    const libbitcoin::hash_digest& tx_hash) {
  std::cout << "SUBSCRIPTION HANDLER GOT ERROR CODE: " << error
            << ", HEIGHT: " << height << ", SEQ: " << sequence
            << ", tx_hash: " << libbitcoin::encode_base16(tx_hash) << std::endl;

  // fetch the tx info and update unconfirmed balances
}

void Megabit::LoadReceiveTab(Configuration& config, uint32_t account_index,
                             bool set_index) {
  auto receive_address = bitcoin_interface_.GetNextAddressForAccount(
      account_index, megabit::constants::external_address_index);
  if (receive_address.empty()) {
    QMessageBox::information(
        const_cast<decltype(this)>(this),
        tr("Megabit: Cannot contact Bitcoin network"),
        tr("Error: The connection to the bitcoin network was unable to "
           "retrieve necessary information about this wallet.\n\n"
           "Please check that the configured server is reachable "
           "and try again."));

    exit(1);
  }

  if (!receive_combo_) {
    receive_combo_ = new QComboBox();
    connect(receive_combo_, SIGNAL(currentIndexChanged(int)), this,
            SLOT(OnReceiveAccountUpdated(int)));
  }

  if (set_index) {
    // the reason we do this conditionally is because when a user
    // selects a different receive account, we don't want to have it
    // infinitely recursively call this (which calls
    // OnReceiveAccountUpdated)
    receive_combo_->clear();
    receive_combo_->addItems(config.account_names);
    receive_combo_->setCurrentIndex(account_index);
  }

  safe_delete(receive_label_);
  receive_label_ =
      new QLabel("To receive Bitcoin into account \"" +
                 config.account_names.at(account_index) + "\", send to");
  receive_label_->setWordWrap(true);

  safe_delete(receive_address_edit_);
  receive_address_edit_ = new QLineEdit(this);
  receive_address_edit_->setReadOnly(true);
  receive_address_edit_->setText(QString::fromStdString(receive_address));

  if (!copy_receive_address_) {
    copy_receive_address_ = new QPushButton("Copy Address to Clipboard", this);
    connect(copy_receive_address_, SIGNAL(clicked()), this,
            SLOT(OnCopyReceiveAddress()));
  }

  QTimer::singleShot(100, this, [this, receive_address]() {
    MonitorAddress(receive_address);
  });

  safe_delete(receive_address_label_);
  receive_address_label_ = new QLabel(QString::fromStdString(receive_address));
  receive_address_label_->setFont(QFont("Courier", 12, QFont::Bold));
  receive_address_label_->setWordWrap(true);

  const auto png_data = bitcoin_interface_.GetAddressAsPngData(receive_address);

  QTemporaryFile* png_file = new QTemporaryFile();
  qrcode_image_files_.push_back(png_file);
  png_file->setAutoRemove(false);
  auto is_open = png_file->open();
  MEGABIT_ASSERT(is_open);
  const auto data = reinterpret_cast<const char*>(png_data.data());
  png_file->write(data, png_data.size());
  png_file->flush();
  png_file->close();

  safe_delete(receive_image_label_);
  receive_image_label_ = new QLabel();
  receive_image_label_->setPixmap(QPixmap(png_file->fileName()));

  QSizePolicy spacer(QSizePolicy::Preferred, QSizePolicy::Preferred);
  spacer.setVerticalStretch(2);

  safe_delete(receive_spacing_label_);
  receive_spacing_label_ = new QLabel();
  receive_spacing_label_->setSizePolicy(spacer);

  safe_delete(receive_tab_layout_);
  receive_tab_layout_ = new QVBoxLayout();
  receive_tab_layout_->addWidget(receive_combo_);
  receive_tab_layout_->addWidget(receive_label_);
  receive_tab_layout_->addWidget(receive_address_label_);
  receive_tab_layout_->addWidget(receive_image_label_);
  receive_tab_layout_->addWidget(receive_spacing_label_);
  receive_tab_layout_->addWidget(receive_address_edit_);
  receive_tab_layout_->addWidget(copy_receive_address_);
  ui->receive_tab->setLayout(receive_tab_layout_);
}

void Megabit::OnCopyReceiveAddress() {
  std::cout << "OnCopyReceiveAddress called ..." << std::endl;
  QClipboard* clipboard = QApplication::clipboard();
  MEGABIT_ASSERT(clipboard);
  MEGABIT_ASSERT(receive_address_edit_);

  auto address = receive_address_edit_->text();
  receive_address_edit_->selectAll();
  std::cout << "COPYING " << address.toStdString() << " TO CLIPBOARD"
            << std::endl;
  clipboard->setText(receive_address_edit_->text());
}

void Megabit::closeEvent(QCloseEvent* /* event */) {
  std::cout << "Closing Wallet and Saving Configuration" << std::endl;
  SaveConfiguration(config_);
}

void Megabit::ShowAbout() {
  QMessageBox::about(this, "About Megabit",
                     "This is the about info\nThis is more about info");
}

void Megabit::ShowSettings() {
  Settings settings(config_);
  settings.InitDialog();
  settings.exec();
}

void Megabit::GetFeeData() {
  QUrl url("http://api.blockcypher.com/v1/btc/main");
  QNetworkRequest req(url);
  req.setRawHeader("User-Agent", "Megabit Bitcoin Wallet");
  std::cout << "about to get fee data" << std::endl;
  fee_reply_ = network_manager_.get(req);

  connect(fee_reply_, SIGNAL(finished()), this, SLOT(OnFeeDataRead()));
}

void Megabit::OnFeeDataRead() {
  std::cout << "fee data retrieved" << std::endl;
  QByteArray array = fee_reply_->readAll();
  const auto response = array.data();
  std::cout << "fee data resp: " << response << std::endl;
  if (!strlen(response)) {
    // try again in one minute
    QTimer::singleShot(60000, this, SLOT(GetFeeData()));
    return;
  }

  std::vector<std::string> lines;
  boost::split(lines, response, boost::is_any_of("\r\n"));

  for (const auto& line : lines) {
    if (line.find("low_fee_per_kb") != std::string::npos) {
      std::vector<std::string> chunks;
      boost::split(chunks, line, boost::is_any_of(":"));
      config_.low_fee_per_kb = strtoull(chunks[1].c_str(), NULL, 10);
    } else if (line.find("medium_fee_per_kb") != std::string::npos) {
      std::vector<std::string> chunks;
      boost::split(chunks, line, boost::is_any_of(":"));
      config_.medium_fee_per_kb = strtoull(chunks[1].c_str(), NULL, 10);
    } else if (line.find("high_fee_per_kb") != std::string::npos) {
      std::vector<std::string> chunks;
      boost::split(chunks, line, boost::is_any_of(":"));
      config_.high_fee_per_kb = strtoull(chunks[1].c_str(), NULL, 10);
    }
  }

  UpdateFeeLabel();

  // update fees every 5 minutes
  QTimer::singleShot(300000, this, SLOT(GetFeeData()));
}

void Megabit::GetBlockHeight() {
  auto block_height_thread = new QThread();
  auto block_height_worker =
      new BlockHeightThread(bitcoin_interface_, block_height_);

  MEGABIT_ASSERT(block_height_thread);
  MEGABIT_ASSERT(block_height_worker);

  block_height_worker->moveToThread(block_height_thread);

  connect(block_height_worker, SIGNAL(BlockHeightError(QString)), this,
          SLOT(OnBlockHeightError(QString)));
  connect(block_height_thread, SIGNAL(started()), block_height_worker,
          SLOT(GetBlockHeight()));
  connect(block_height_worker, SIGNAL(finished()), block_height_thread,
          SLOT(quit()));
  connect(block_height_worker, SIGNAL(finished()), this,
          SLOT(OnGetBlockHeight()));
  connect(block_height_worker, SIGNAL(finished()), block_height_worker,
          SLOT(deleteLater()));
  connect(block_height_thread, SIGNAL(finished()), block_height_thread,
          SLOT(deleteLater()));

  block_height_thread->start();
}

void Megabit::OnGetBlockHeight() {
  bitcoin_interface_.SetBlockHeight(block_height_);

  // disable/enable the refresh button
  if (refresh_block_height_ == 0) {
    refresh_block_height_ = block_height_;
  }

  std::cout << "refresh block height = " << refresh_block_height_
            << ", block height = " << block_height_ << std::endl;
  ui->refreshButton->setEnabled(refresh_block_height_ != block_height_);

  // update transaction table's confirmation numbers each time we
  // receive a block update
  for (auto i = 0; i < ui->transactionTable->rowCount(); i++) {
    auto block = ui->transactionTable->item(i, 0)
                     ->data(Qt::UserRole)
                     .toString()
                     .toLong();
    auto num_confirmations = block ? QString::number(block_height_ - block) : 0;
    auto confirmation_str =
        (num_confirmations.toLong()
             ? QString("Confirmed (") + num_confirmations +
                   QString(" confirmations)")
             : QString("This transaction is not confirmed"));

    // FIXME: GRAY OUT IF UNCONFIRMED??

    // sets all tx tooltips to contain the number of confirmations
    for (auto j = 0; j < ui->transactionTable->columnCount(); j++) {
      auto item = ui->transactionTable->item(i, j);
      item->setData(Qt::ToolTipRole, confirmation_str);
    }
  }

  // start new timer to update block height once per minute
  QTimer::singleShot(60000, this, SLOT(GetBlockHeight()));
}

void Megabit::ClearPaymentFields() {
  ui->amountLineEdit->clear();
  ui->amountBTCLineEdit->clear();
  ui->recipientBitcoinAddressLineEdit->clear();
  ui->subtract_fee->setChecked(false);
  /* ui->amountBTCLineEdit->setText(tr("")); // FIXME: is there a clear? */
  /* ui->recipientBitcoinAddressLineEdit->setText(tr("")); */
  /* ui->subtract_fee->setChecked(false); */
}

void Megabit::SendPayment() {
  // Get all data from send payment dialog fields
  uint32_t account_index = config_.current_account_index;
  uint64_t amount = ui->amountBTCLineEdit->text().toDouble() *
                    megabit::constants::satoshi_per_btc;
  QString dest_addr_str = ui->recipientBitcoinAddressLineEdit->text();
  auto destination_address =
      bitcoin_interface_.GetPaymentAddress(dest_addr_str.toStdString());

  uint32_t target_fee_per_kb = current_fee_;
  bool subtract_fee_from_amount = ui->subtract_fee->isChecked();

  std::cout << "send payment account_index = " << account_index << std::endl;
  std::cout << "send payment amount = " << amount << std::endl;
  std::cout << "send payment dest_addr = " << dest_addr_str.toStdString()
            << std::endl;
  std::cout << "send payment target_fee_per_kb = " << target_fee_per_kb
            << std::endl;
  std::cout << "send payment subtract_fee_from_amount = "
            << subtract_fee_from_amount << std::endl;

  if ((amount == 0) || dest_addr_str.isEmpty()) {
    QMessageBox::information(const_cast<decltype(this)>(this),
                             tr("Send Warning"),
                             tr("Both an amount to send and a destination "
                                "address are required in order to send."));
    return;
  } else if ((amount > account_balance_map_[account_index]) ||
             ((amount == account_balance_map_[account_index]) &&
              !subtract_fee_from_amount)) {
    QMessageBox::information(
        const_cast<decltype(this)>(this), tr("Send Warning"),
        tr("The amount specified cannot be satisfied by "
           "the current account.\n\nPlease enter a valid amount."));
    return;
  }

  auto send_payment_thread = new QThread();
  auto send_payment_worker = new SendPaymentThread(
      bitcoin_interface_, account_index, amount, destination_address,
      target_fee_per_kb, subtract_fee_from_amount);

  MEGABIT_ASSERT(send_payment_thread);
  MEGABIT_ASSERT(send_payment_worker);

  send_payment_worker->moveToThread(send_payment_thread);

  connect(send_payment_thread, SIGNAL(started()), send_payment_worker,
          SLOT(SendPayment()));
  connect(send_payment_worker, SIGNAL(finished()), send_payment_thread,
          SLOT(quit()));
  connect(send_payment_worker, SIGNAL(GetUserSendPaymentConfirmation()), this,
          SLOT(OnGetUserSendPaymentConfirmation()));
  connect(send_payment_worker, SIGNAL(SendPaymentError(QString)), this,
          SLOT(OnSendPaymentError(QString)));
  connect(this, SIGNAL(SendUserConfirmedPayment()), send_payment_worker,
          SLOT(OnSendUserConfirmedPayment()));
  connect(send_payment_worker, SIGNAL(finished()), this, SLOT(OnPaymentSent()));
  connect(send_payment_worker, SIGNAL(finished()), send_payment_worker,
          SLOT(deleteLater()));
  connect(send_payment_thread, SIGNAL(finished()), send_payment_thread,
          SLOT(deleteLater()));

  send_payment_thread->start();
}

void Megabit::OnGetUserSendPaymentConfirmation() {
  std::cout << "on get user send payment confirmation called" << std::endl;
  auto pending_tx = bitcoin_interface_.GetPendingTransaction();

  uint64_t total_amount = 0;
  for (const auto& selected : pending_tx->selected_unspent_list) {
    total_amount += selected.second.value;
  }

  uint64_t fee = total_amount - pending_tx->amount - pending_tx->change_amount;
  if (pending_tx->subtract_fee_from_amount && (fee == 0)) {
    fee = bitcoin_interface_.GetCalculatedFee(pending_tx->tx,
                                              pending_tx->target_fee_per_kb);
  }

  std::cout << "total amount: " << total_amount << ", tx amount: "
            << bitcoin_interface_.SatoshiToBtc(pending_tx->amount)
            << ", change: " << pending_tx->change_amount
            << ", fee: " << bitcoin_interface_.SatoshiToBtc(fee) << std::endl;

  QString amount_str;
  if (pending_tx->subtract_fee_from_amount) {
    amount_str.setNum(bitcoin_interface_.SatoshiToBtc(pending_tx->amount - fee),
                      'f', 8);
  } else {
    amount_str.setNum(bitcoin_interface_.SatoshiToBtc(pending_tx->amount), 'f',
                      8);
  }

  QString fee_str;
  fee_str.setNum(bitcoin_interface_.SatoshiToBtc(fee), 'f', 8);

  const auto conversion_factor =
      GetConvertedCurrencyAmount(config_, 1, config_.currency.toStdString());
  auto currency_amount =
      bitcoin_interface_.SatoshiToBtc(pending_tx->amount) * conversion_factor;
  auto currency_fee = bitcoin_interface_.SatoshiToBtc(fee) * conversion_factor;

  std::cout << "converted currency amount: " << currency_amount << std::endl;
  std::cout << "converted currency fee: " << currency_fee << std::endl;

  QString currency_amount_str;
  currency_amount_str.setNum(currency_amount, 'f', 2);
  currency_amount_str += " " + config_.currency;

  QString currency_fee_str;
  currency_fee_str.setNum(currency_fee, 'f', 2);
  currency_fee_str += " " + config_.currency;

  auto ret = QMessageBox::question(
      this, "Send Payment",
      "Are you sure that you want to send " + amount_str + " BTC (" +
          currency_amount_str + ") to " +
          QString::fromStdString(pending_tx->destination_address.encoded()) +
          " with an estimated fee of " + fee_str + " BTC (" + currency_fee_str +
          ")?",
      QMessageBox::Yes | QMessageBox::No);
  if (ret == QMessageBox::Yes) {
    std::cout << "emitting send user confirmed payment" << std::endl;
    emit SendUserConfirmedPayment();
  }
}

void Megabit::OnPaymentSent() {
  // even though unconfirmed, refresh transactions
  // FIXME: how to indicate what is/isn't confirmed?!
  std::cout << "payment sent properly" << std::endl;
  ClearPaymentFields();
  // RefreshTransactions();
  QMessageBox::information(
      const_cast<decltype(this)>(this), tr("Send Success"),
      tr("Your payment has been broadcasted, however it may not appear "
         "in your transactions until it is confirmed."));
}

void Megabit::AddAccount() {
  std::cout << "AddAccount clicked" << std::endl;
  auto account_index = QString::number(++config_.num_accounts);
  config_.account_names.append(tr("Account ") + account_index);
  RefreshTransactions(false);
}

void Megabit::MonitorAddress(std::string address) {
  std::cout << "*** monitor address called with " << address << std::endl;

  {
    std::lock_guard<boost::detail::spinlock> lock(address_monitor_map_lock_);
    // first check if we already have a monitor running for this
    // address.  If so, and it's still running, do nothing.
    auto monitor_iter = address_monitor_map_.find(address);
    if (monitor_iter != address_monitor_map_.end()) {
      if (monitor_iter->second->address_monitor_thread->isRunning()) {
        std::cout << "Doing nothing because a monitor for " << address
                  << " is already running" << std::endl;
        return;
      }

      // if it was found and it's no longer running, let's remove this
      // from the map and initialize as if it was a new registration
      std::cout << "Cleaning up stale monitor for " << address << std::endl;
      monitor_iter->second->address_monitor_worker->CancelMonitor();

      address_monitor_map_.erase(monitor_iter);
    }
  }

  auto monitor = std::make_shared<AddressMonitorObj>();
  monitor->address_monitor_info = std::make_shared<AddressMonitorInfo>();
  monitor->address_monitor_thread = new QThread();
  monitor->address_monitor_worker = new AddressMonitorThread(
      address, config_.server_address.toStdString(),
      config_.server_public_key.toStdString(), monitor->address_monitor_info);

  std::cout << "*** Allocated new monitor " << monitor
            << " and listener thread " << monitor->address_monitor_thread
            << " for new worker " << monitor->address_monitor_worker
            << std::endl;

  MEGABIT_ASSERT(monitor->address_monitor_thread);
  MEGABIT_ASSERT(monitor->address_monitor_worker);

  std::cout << "worker has parent: "
            << monitor->address_monitor_worker->parent() << std::endl;
  std::cout << "[before] worker in thread: "
            << monitor->address_monitor_worker->thread() << std::endl;
  monitor->address_monitor_worker->moveToThread(
      monitor->address_monitor_thread);
  std::cout << "[after] worker in thread: "
            << monitor->address_monitor_worker->thread() << std::endl;

  connect(monitor->address_monitor_worker,
          SIGNAL(AddressSubscriptionError(QString)), this,
          SLOT(OnAddressSubscriptionError(QString)));
  connect(monitor->address_monitor_thread, SIGNAL(started()),
          monitor->address_monitor_worker, SLOT(ListenForAddressChanges()));
  connect(monitor->address_monitor_worker, SIGNAL(finished()), this,
          SLOT(OnMonitorAddressComplete()));
  connect(monitor->address_monitor_worker, SIGNAL(finished()),
          monitor->address_monitor_thread, SLOT(quit()));
  connect(monitor->address_monitor_worker, SIGNAL(finished()),
          monitor->address_monitor_worker, SLOT(deleteLater()));
  connect(monitor->address_monitor_thread, SIGNAL(finished()),
          monitor->address_monitor_thread, SLOT(deleteLater()));

  {
    std::lock_guard<boost::detail::spinlock> lock(address_monitor_map_lock_);
    address_monitor_map_[address] = monitor;
  }

  monitor->address_monitor_thread->start();
}

void Megabit::OnBlockHeightError(QString error) {
  // retry in 10 seconds after failure
  //
  // keep a counter and if it errors N number of times in a
  // row, show a message box and exit??
  QMessageBox::information(const_cast<decltype(this)>(this),
                           tr("Could not retrieve Block Height"),
                           tr("Error: ") + error);
  QTimer::singleShot(10000, this, SLOT(GetBlockHeight()));
}

void Megabit::OnAddressSubscriptionError(QString address) {
  // keep a counter and if it errors N number of times in a
  // row, show a message box and exit??
  QMessageBox::information(
      const_cast<decltype(this)>(this), tr("Address Subscription Error"),
      tr("Error: Cannot listen for transactions on ") + address);
  QTimer::singleShot(10000, this, SLOT(MonitorAddress(address.toStdString())));
}

void Megabit::OnSendPaymentError(QString error) {
  QMessageBox::information(const_cast<decltype(this)>(this),
                           tr("Payment did not send correctly"),
                           tr("Error: ") + error);
}

void Megabit::RefreshTransactions() {
  RefreshTransactions(refresh_block_height_ == block_height_);
}

void Megabit::RefreshTransactions(bool skip_refresh) {
  // skip_refresh is primarily used for UI clicks, vs times we know we
  // want to refresh
  if (skip_refresh) {
    return;
  }
  refresh_block_height_ = block_height_;

  // clear accounts tab
  ui->accountsTable->setRowCount(0);
  // clear receive tab
  ui->transactionTable->setRowCount(0);

  LoadAccountsTab(config_);
  LoadReceiveTab(config_, config_.current_account_index);

  emit WalletLoaded();
}

void Megabit::OnMonitorAddressComplete() {
  address_monitor_map_lock_.lock();

  std::shared_ptr<AddressMonitorInfo> ret = nullptr;
  for (auto monitor_entry : address_monitor_map_) {
    ret = monitor_entry.second->address_monitor_info;
    if (ret->finished) {
      break;
    }
  }

  MEGABIT_ASSERT(ret && ret->finished);
  auto address = ret->address;

  std::cout << "OnMonitorAddressComplete searching for " << address
            << std::endl;
  auto monitor_iter = address_monitor_map_.find(address);
  MEGABIT_ASSERT(monitor_iter != address_monitor_map_.end());

  std::cout << "OnMonitorAddressComplete called for " << address << std::endl;
  std::cout << "OnMonitorAddressComplete got return code " << ret->code
            << std::endl;
  std::cout << "Monitored information for " << ret->address << ", sequence "
            << (uint32_t)ret->sequence << ", height " << ret->height
            << ", tx_hash " << libbitcoin::encode_base16(ret->tx_hash)
            << std::endl;

  address_monitor_map_.erase(monitor_iter);
  address_monitor_map_lock_.unlock();

  // if we got a notification without details, or the subscription has
  // timed out (default server side config is 10 minutes),
  // re-subscribe and continue waiting
  if (ret->code == libbitcoin::error::channel_timeout) {
    QTimer::singleShot(100, this, SLOT(RefreshTransactions()));
    return;
  } else if (ret->code == libbitcoin::error::success) {
    TxBlockInfo tx_block_info{};
    if (bitcoin_interface_.GetTransactionInfo(ret->tx_hash, tx_block_info,
                                              true)) {
      // directly insert the transaction and alert the user
      AddUnconfirmedTransaction(address, tx_block_info.tx);
    } else {
      QTimer::singleShot(100, this,
                         [this, address]() { MonitorAddress(address); });
    }
  }
}

void Megabit::GetCurrencyData() {
  QUrl url("https://blockchain.info/ticker");
  QNetworkRequest req(url);
  req.setRawHeader("User-Agent", "Megabit Bitcoin Wallet");
  std::cout << "about to get currency data" << std::endl;
  currency_reply_ = network_manager_.get(req);

  connect(currency_reply_, SIGNAL(finished()), this,
          SLOT(OnCurrencyDataRead()));
}

void Megabit::OnCurrencyDataRead() {
  std::cout << "currency data retrieved" << std::endl;
  QByteArray array = currency_reply_->readAll();
  const auto response = array.data();
  std::cout << "currency data resp: " << response << std::endl;
  if (!strlen(response)) {
    return;
  }

  double conversion_rate = 0.0f;
  std::vector<std::string> lines;
  boost::split(lines, response, boost::is_any_of("\r\n"));

  for (const auto& line : lines) {
    std::cout << "currency reply: " << line << std::endl;
    if (line.find("  \"") != std::string::npos) {
      auto currency = line.substr(3, 3);
      auto pos = line.find("\"last\" : ");
      if (pos != std::string::npos) {
        auto end_pos = line.find(",", pos + 8);
        if (end_pos != std::string::npos) {
          const auto val = strtof(line.substr(pos + 8, end_pos).c_str(), NULL);
          config_.currency_value_map[currency] = val;

          if (QString::fromStdString(currency) == config_.currency) {
            conversion_rate = val;
          }
        }
      }
    }
  }

  // update all account balances after getting the latest currency
  // conversion rate
  for (int i = 0; i < ui->accountsTable->rowCount(); i++) {
    const auto btc = ui->accountsTable->model()
                         ->data(ui->accountsTable->model()->index(i, 1))
                         .toDouble();
    const auto currency = btc * conversion_rate;
    QString currency_str;
    currency_str.setNum(currency, 'f', 2);
    ui->accountsTable->model()->setData(ui->accountsTable->model()->index(i, 2),
                                        currency_str);
  }

  // update all transaction amounts after getting the latest
  // currency conversion rate
  for (auto i = 0; i < ui->transactionTable->rowCount(); i++) {
    auto btc = ui->transactionTable->item(i, 3)->text().toDouble();
    const auto currency = btc * conversion_rate;
    std::cout << "Updating currency now: " << btc << " -> " << currency
              << std::endl;
    QString currency_str;
    currency_str.setNum(currency, 'f', 2);
    ui->transactionTable->model()->setData(
        ui->transactionTable->model()->index(i, 4), currency_str);
  }

  // update currency conversion data every 5 minutes
  QTimer::singleShot(300000, this, SLOT(GetCurrencyData()));
}

void Megabit::ShowWalletLoader() {
  wallet_loader_dialog_ = new QProgressDialog(this);
  wallet_loader_dialog_->setLabelText("Loading account information ...");
  wallet_loader_dialog_->setCancelButton(0);
  wallet_loader_dialog_->setMaximum(100);
  wallet_loader_dialog_->setValue(0);
  wallet_loader_dialog_->setMinimumDuration(0);
  wallet_loader_dialog_->show();
}

void Megabit::OnWalletLoaded() {
  if (wallet_loader_dialog_) {
    wallet_loader_dialog_->cancel();
    delete wallet_loader_dialog_;
    wallet_loader_dialog_ = nullptr;
  }
  QTimer::singleShot(10, this, SLOT(GetBlockHeight()));
}

void Megabit::OnAccountNameEdited(int row, int column) {
  if (column == 0) {
    const auto account_name = ui->accountsTable->item(row, column)->text();
    if (account_name != "") {
      config_.account_names.replace(row, account_name);

      LoadSendTab(config_);
      LoadReceiveTab(config_, config_.current_account_index);
    }
  }
}

// adds entries to the transactionTable
void Megabit::OnProgressUpdated(uint32_t account_index, uint32_t row,
                                const TxInfo tx_info) {
  QTableWidgetItem* type_item = nullptr;
  if (tx_info.is_spend) {
    type_item = new QTableWidgetItem(tr("Sent from \"") +
                                     config_.account_names.at(account_index) +
                                     tr("\""));
  } else {
    type_item = new QTableWidgetItem(tr("Received with \"") +
                                     config_.account_names.at(account_index) +
                                     tr("\""));
  }

  ui->transactionTable->insertRow(row);

  type_item->setTextAlignment(Qt::AlignVCenter);
  type_item->setFlags(type_item->flags() & ~Qt::ItemIsEditable);

  const auto date_stamp =
      megabit::utils::get_date_string(tx_info.header.timestamp());

  QTableWidgetItem* date_item =
      new QTableWidgetItem(QString::fromStdString(date_stamp));
  date_item->setTextAlignment(Qt::AlignVCenter);
  date_item->setFlags(date_item->flags() & ~Qt::ItemIsEditable);
  date_item->setData(Qt::UserRole, QString::number(tx_info.height));

  QTableWidgetItem* label_item =
      new QTableWidgetItem(QString::fromStdString(tx_info.address));
  label_item->setTextAlignment(Qt::AlignVCenter);
  // this label is editable, but it means we have to store it in the
  // config or somewhere else (?)

  QString amount_str;
  amount_str.setNum(bitcoin_interface_.SatoshiToBtc(tx_info.amount), 'f', 8);
  QTableWidgetItem* amount_item = new QTableWidgetItem(amount_str);

  amount_item->setTextAlignment(Qt::AlignVCenter);
  amount_item->setFlags(amount_item->flags() & ~Qt::ItemIsEditable);

  auto converted_balance = GetConvertedCurrencyAmount(
      config_, tx_info.amount, config_.currency.toStdString());
  QString converted_balance_str;
  converted_balance_str.setNum(
      bitcoin_interface_.SatoshiToBtc(converted_balance), 'f', 2);
  QTableWidgetItem* converted_amount_item =
      new QTableWidgetItem(converted_balance_str);
  converted_amount_item->setFlags(converted_amount_item->flags() &
                                  ~Qt::ItemIsEditable);

  ui->transactionTable->setItem(row, 0, date_item);
  ui->transactionTable->setItem(row, 1, type_item);
  ui->transactionTable->setItem(row, 2, label_item);
  ui->transactionTable->setItem(row, 3, amount_item);
  ui->transactionTable->setItem(row, 4, converted_amount_item);

  ui->transactionTable->verticalHeader()->setVisible(false);

  // set the default column sorting to be by ascending date
  ui->transactionTable->sortItems(0, Qt::AscendingOrder);
}

void Megabit::AddUnconfirmedTransaction(
    const std::string address, const libbitcoin::chain::transaction& tx) {
  QTableWidgetItem* type_item = nullptr;
  type_item = new QTableWidgetItem(tr("Received Unconfimed Transaction"));

  auto row = 0;
  ui->transactionTable->insertRow(row);

  type_item->setTextAlignment(Qt::AlignVCenter);
  type_item->setFlags(type_item->flags() & ~Qt::ItemIsEditable);

  QTableWidgetItem* date_item = new QTableWidgetItem(tr("Pending ..."));
  date_item->setTextAlignment(Qt::AlignVCenter);
  date_item->setFlags(date_item->flags() & ~Qt::ItemIsEditable);
  date_item->setData(Qt::UserRole, QString::number(0));

  QTableWidgetItem* label_item =
      new QTableWidgetItem(QString::fromStdString(address));
  label_item->setTextAlignment(Qt::AlignVCenter);
  // this label is editable, but it means we have to store it in the
  // config or somewhere else (?)

  uint64_t amount = bitcoin_interface_.GetUnconfirmedTransactionAmount(tx);

  QString amount_str;
  amount_str.setNum(bitcoin_interface_.SatoshiToBtc(amount), 'f', 8);
  QTableWidgetItem* amount_item = new QTableWidgetItem(amount_str);

  amount_item->setTextAlignment(Qt::AlignVCenter);
  amount_item->setFlags(amount_item->flags() & ~Qt::ItemIsEditable);

  auto converted_balance = GetConvertedCurrencyAmount(
      config_, amount, config_.currency.toStdString());
  QString converted_balance_str;
  converted_balance_str.setNum(
      bitcoin_interface_.SatoshiToBtc(converted_balance), 'f', 2);
  QTableWidgetItem* converted_amount_item =
      new QTableWidgetItem(converted_balance_str);
  converted_amount_item->setFlags(converted_amount_item->flags() &
                                  ~Qt::ItemIsEditable);

  ui->transactionTable->setItem(row, 0, date_item);
  ui->transactionTable->setItem(row, 1, type_item);
  ui->transactionTable->setItem(row, 2, label_item);
  ui->transactionTable->setItem(row, 3, amount_item);
  ui->transactionTable->setItem(row, 4, converted_amount_item);

  ui->transactionTable->verticalHeader()->setVisible(false);

  // set the default column sorting to be by ascending date
  ui->transactionTable->sortItems(0, Qt::AscendingOrder);

  // TODO: Roll over QRcode displayed on Receive Tab
  //
  // NOTE: The reason this isn't done yet is probably because the tx
  // is unconfirmed and GetNextAddress is probably returning the same
  // address because we didn't bump the index (I think, not sure).  If
  // we do bump it, we run the risk of having large gaps at times
  // between addresses.  This could be addressed, by increasing the
  // gap length, but that may be ok (so long as it's user settable).

  QMessageBox::information(
      const_cast<decltype(this)>(this), tr("Bitcoin Received!"),
      tr("You have received ") + amount_str + " BTC at " +
          QString::fromStdString(address) + tr("!\n\nThis transaction is ") +
          tr("currently unconfirmed and will show up in your transaction ") +
          tr("history permanently after it is fully confirmed."));
}

double Megabit::GetConvertedCurrencyAmount(const Configuration& config,
                                           uint64_t btc_amount,
                                           std::string currency) {
  const auto currency_iter = config.currency_value_map.find(currency);
  return ((currency_iter != config.currency_value_map.end())
              ? currency_iter->second * btc_amount
              : 0.0f);
}
