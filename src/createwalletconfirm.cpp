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

#include "../include/megabit/createwalletconfirm.hpp"

#include <QSettings>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <chrono>
#include <iostream>
#include <sstream>

#include "../include/megabit/createwalletwizard.hpp"

CreateWalletConfirm::CreateWalletConfirm(QWidget* parent)
    : QWizardPage(parent) {
  setTitle(tr("Your Megabit Secret Mnemonic"));
  setPixmap(QWizard::WatermarkPixmap, QPixmap(":/images/watermark.png"));

  main_label_ =
      new QLabel(tr("To confirm that you have properly stored your "
                    "wallet's Mnemonic, please enter the following words:"));

  main_label_->setWordWrap(true);

  confirm_label1_ = new QLabel(tr(""));
  confirm_label2_ = new QLabel(tr(""));

  confirm1_ = new QLineEdit();
  registerField("createwallet.confirm_label1", confirm_label1_);
  registerField("createwallet.confirm1*", confirm1_);

  confirm2_ = new QLineEdit();
  registerField("createwallet.confirm_label2", confirm_label2_);
  registerField("createwallet.confirm2*", confirm2_);

  layout_ = new QVBoxLayout();
  layout_->addWidget(main_label_);
  layout_->addWidget(confirm_label1_);
  layout_->addWidget(confirm1_);
  layout_->addWidget(confirm_label2_);
  layout_->addWidget(confirm2_);
  setLayout(layout_);
}

CreateWalletConfirm::~CreateWalletConfirm() {
  delete confirm1_;
  delete confirm2_;
  delete confirm_label1_;
  delete confirm_label2_;
  delete main_label_;
  delete layout_;
}

void CreateWalletConfirm::initializePage() {
  auto mnemonic_flat = field("createwallet.mnemonic_flat").toString().trimmed();
  auto mnemonic = mnemonic_flat.toStdString();
  std::cout << "GOT MNEMONIC: " << mnemonic << std::endl;

  boost::split(word_list_, mnemonic, boost::is_any_of(" "));
  const auto num_words = word_list_.size();

  // pick 2 random words in the word list for verification
  const auto rand_seed =
      std::chrono::system_clock::now().time_since_epoch().count();
  boost::random::mt19937 generator(rand_seed);
  confirm_index1_ = (generator() % num_words) + 1;
  do {
    confirm_index2_ = (generator() % num_words) + 1;
  } while (confirm_index1_ == confirm_index2_);

  if (confirm_index1_ > confirm_index2_) {
    std::swap(confirm_index1_, confirm_index2_);
  }

  std::stringstream label1_ss;
  label1_ss << "Word " << confirm_index1_;

  std::stringstream label2_ss;
  label2_ss << "Word " << confirm_index2_;

  confirm_label1_->setText(QString::fromStdString(label1_ss.str()));
  confirm_label2_->setText(QString::fromStdString(label2_ss.str()));

  auto index = 0;
  for (auto& word : word_list_) {
    std::cout << "Indexed[" << ++index << "] = " << word << std::endl;
  }
}

bool CreateWalletConfirm::validatePage() {
  auto confirm1 = confirm1_->text().toStdString();
  auto confirm2 = confirm2_->text().toStdString();

  // the indices are 1 based (i.e. 1-24) and need to be 0 based
  // (i.e. 0-23) to index into the words vector
  const auto index1 = confirm_index1_ - 1;
  const auto index2 = confirm_index2_ - 1;

  std::cout << "Checking: " << word_list_[index1] << " == " << confirm1 << "?"
            << std::endl;
  std::cout << "Checking: " << word_list_[index2] << " == " << confirm2 << "?"
            << std::endl;
  auto ret =
      ((word_list_[index1] == confirm1) && (word_list_[index2] == confirm2));
  if (ret) {
    // persist required settings
    QSettings settings("TheCodeFactory", "Megabit");
    settings.setValue("global/first_time", 0);

    QMessageBox::information(
        const_cast<decltype(this)>(this), tr("Wallet Created!"),
        tr("Congratulations: The wallet was created successfully and this app "
           "will exit at this time.  Please restart to use the new wallet!"));

    exit(1);
  } else {
    QMessageBox::information(
        const_cast<decltype(this)>(this), tr("Mnemonic check failed"),
        tr("Error: The confirmation words entered do not match those provided. "
           "Please be sure that the mnemonic has been properly written down to "
           "ensure future wallet recovery, should it be needed."));
  }
  return ret;
}

int CreateWalletConfirm::nextId() const { return -1; }
