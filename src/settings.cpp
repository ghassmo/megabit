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

#include "../include/megabit/settings.hpp"

#include <QByteArray>
#include <QFont>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QSettings>
#include <QVBoxLayout>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <iostream>
#include <vector>

#include "../include/megabit/constants.hpp"
#include "../include/megabit/megabit.hpp"

void Settings::InitDialog() {
  auto network_label = new QLabel(tr("Choose Bitcoin Network:"));
  auto network_combo = new QComboBox();
  network_combo->addItem("Mainnet", "mainnet");
  network_combo->addItem("Testnet", "testnet");

  int index = network_combo->findData(config_.network);
  if (index == -1) {
    index = 0;
  }
  network_combo->setCurrentIndex(index);

  connect(network_combo, SIGNAL(currentIndexChanged(QString)), this,
          SLOT(NetworkIndexChanged(QString)));

  auto server_label = new QLabel(tr("Libbitcoin server url:"));
  server_line_edit_ = new QLineEdit();
  server_line_edit_->setText(config_.server_address);

  auto server_pk_label = new QLabel(tr("Libbitcoin server public key:"));
  server_pk_line_edit_ = new QLineEdit();
  server_pk_line_edit_->setText(config_.server_public_key);

  auto currency_label = new QLabel(tr("Conversion Currency:"));
  currency_combo_ = new QComboBox();

  static const std::vector<QString> currencies = {
      "USD", "AUD", "BRL", "CAD", "CHF", "CLP", "CNY", "DKK",
      "EUR", "GBP", "HKD", "INR", "ISK", "JPY", "KRW", "NZD",
      "PLN", "RUB", "SEK", "SGD", "THB", "TWD"};

  for (const auto currency : currencies) {
    currency_combo_->addItem(currency, currency);
  }

  index = currency_combo_->findData(config_.currency);
  if (index == -1) {
    index = 0;
  }
  currency_combo_->setCurrentIndex(index);

  connect(currency_combo_, SIGNAL(currentIndexChanged(QString)), this,
          SLOT(CurrencyIndexChanged(QString)));

  auto cancel_button = new QPushButton("Cancel");
  connect(cancel_button, SIGNAL(clicked()), this, SLOT(reject()));

  auto apply_button = new QPushButton("Apply");
  connect(apply_button, SIGNAL(clicked()), this, SLOT(ApplySettings()));

  auto button_layout = new QHBoxLayout();
  button_layout->addWidget(cancel_button);
  button_layout->addWidget(apply_button);

  auto settings_layout = new QVBoxLayout();
  settings_layout->addWidget(network_label);
  settings_layout->addWidget(network_combo);
  settings_layout->addWidget(server_label);
  settings_layout->addWidget(server_line_edit_);
  settings_layout->addWidget(server_pk_label);
  settings_layout->addWidget(server_pk_line_edit_);
  settings_layout->addWidget(currency_label);
  settings_layout->addWidget(currency_combo_);

  settings_layout->addLayout(button_layout);

  setLayout(settings_layout);
  setWindowTitle(tr("Megabit Preferences"));
  resize(450, 300);
}

Settings::~Settings() {
  safe_delete(server_line_edit_);
  safe_delete(server_pk_line_edit_);
}

void Settings::reject() { QDialog::reject(); }

void Settings::NetworkIndexChanged(QString network) {
  std::cout << "network index changed to " << network.toStdString()
            << std::endl;
  if (network == "Mainnet") {
    server_line_edit_->setText(
        QString::fromStdString(megabit::constants::libbitcoin_server_address));
    server_pk_line_edit_->setText(QString::fromStdString(
        megabit::constants::libbitcoin_server_public_key));
  } else {
    server_line_edit_->setText(QString::fromStdString(
        megabit::constants::libbitcoin_testnet_server_address));
    server_pk_line_edit_->setText(QString::fromStdString(
        megabit::constants::libbitcoin_testnet_server_public_key));
  }
}

void Settings::CurrencyIndexChanged(QString currency) {
  std::cout << "currency index changed to " << currency.toStdString()
            << std::endl;
}

void Settings::ApplySettings() {
  QSettings settings("TheCodeFactory", "Megabit");

  config_.currency = currency_combo_->currentText();
  config_.server_address = server_line_edit_->text();
  config_.server_public_key = server_pk_line_edit_->text();

  settings.setValue("global/currency", config_.currency);
  settings.setValue("global/libbitcoin_server_address", config_.server_address);
  settings.setValue("global/libbitcoin_server_public_key",
                    config_.server_public_key);

  QMessageBox::information(
      const_cast<decltype(this)>(this), tr("Megabit: Configuration Updated"),
      tr("The wallet configuration has been updated.\n\n"
         "Changes will not apply until after Megabit is restarted."));

  QDialog::accept();
}
