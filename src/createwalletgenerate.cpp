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

#include "../include/megabit/createwalletgenerate.hpp"

#include <QSettings>
#include <bitcoin/bitcoin.hpp>
#include <iostream>
#include <string>

#include "../include/megabit/createwalletwizard.hpp"
#include "../include/megabit/utils.hpp"

CreateWalletGenerate::CreateWalletGenerate(QWidget* parent)
    : QWizardPage(parent) {
  setTitle(tr("Your Megabit Wallet Mnemonic"));
  setPixmap(QWizard::WatermarkPixmap, QPixmap(":/images/watermark.png"));

  main_label_ = new QLabel(
      tr("Here is your mnemonic phrase that can be used to restore your "
         "Bitcoin wallet for any reason.\n\nIf you would like a different "
         "set of words for your mnemonic, press back now.\n\n"));

  main_label_->setWordWrap(true);

  mnemonic_ = new QLabel(tr(""));

  // create non-visible widget to register the flat mnemonic values
  // (i.e. the word list as a string separated by spaces)
  mnemonic_flat_ = new QLineEdit(this);
  mnemonic_flat_->setVisible(false);
  registerField("createwallet.mnemonic_flat", mnemonic_flat_);

  layout_ = new QVBoxLayout();
  layout_->addWidget(main_label_);
  layout_->addWidget(mnemonic_);
  setLayout(layout_);
}

void CreateWalletGenerate::initializePage() {
  const auto language = field("createwallet.language").toString().toStdString();

  std::cout << "Got language: " << language << std::endl;

  auto generate_entropy = [](const size_t num_bytes) {
    libbitcoin::data_chunk entropy(num_bytes);
    libbitcoin::pseudo_random_fill(entropy);
    return libbitcoin::to_chunk(libbitcoin::bitcoin_hash(entropy));
  };

  auto get_mnemonic_language = [](std::string language) {
    auto ret = libbitcoin::wallet::language::en;
    if (language == "en") ret = libbitcoin::wallet::language::en;
    if (language == "es") ret = libbitcoin::wallet::language::es;
    if (language == "fr") ret = libbitcoin::wallet::language::fr;
    if (language == "it") ret = libbitcoin::wallet::language::it;
    if (language == "ja") ret = libbitcoin::wallet::language::ja;
    if (language == "cs") ret = libbitcoin::wallet::language::cs;
    if (language == "ru") ret = libbitcoin::wallet::language::ru;
    if (language == "uk") ret = libbitcoin::wallet::language::uk;
    if (language == "zh_Hans") ret = libbitcoin::wallet::language::zh_Hans;
    if (language == "zh_Hant") ret = libbitcoin::wallet::language::zh_Hant;
    return ret;
  };

  const auto lang = get_mnemonic_language(language);
  const auto entropy = generate_entropy(megabit::constants::entropy_length);
  word_list_ = libbitcoin::wallet::create_mnemonic(entropy, lang);
  MEGABIT_ASSERT(libbitcoin::wallet::validate_mnemonic(word_list_, lang));

  size_t index = 0;
  size_t num_words = word_list_.size();

  std::stringstream mnemonic_ss;
  std::stringstream mnemonic_flat_ss;
  for (const auto& word : word_list_) {
    mnemonic_ss << "[" << std::setw(2) << ++index << "] " << word << std::endl;
    mnemonic_flat_ss << word << ((index == num_words) ? "" : " ");
  }

  std::cout << "Setting mnemonic string to: " << mnemonic_ss.str() << std::endl;
  auto mnemonic = QString::fromStdString(mnemonic_ss.str());
  mnemonic_->setText(mnemonic);

  std::cout << "Setting mnemonic flat string to: " << mnemonic_flat_ss.str()
            << std::endl;
  auto mnemonic_flat = QString::fromStdString(mnemonic_flat_ss.str());
  mnemonic_flat_->setText(mnemonic_flat);
  setField("createwallet.mnemonic_flat", mnemonic_flat);
}

bool CreateWalletGenerate::validatePage() {
  const auto passphrase_hash_str =
      field("createwallet.passphrase1").toString().toStdString();

  libbitcoin::hash_digest passphrase_hash;
  libbitcoin::decode_base16(passphrase_hash, passphrase_hash_str);

  const auto& hash_start = reinterpret_cast<const char*>(&passphrase_hash);
  const auto passphrase_key =
      std::string(hash_start, hash_start + sizeof(passphrase_hash));

  auto seed = libbitcoin::wallet::decode_mnemonic(word_list_, passphrase_key);
  std::cout << "GENERATED RAW SEED: " << libbitcoin::encode_base16(seed)
            << std::endl;

  megabit::utils::encrypt_data<decltype(seed)>(passphrase_hash, seed);
  auto encrypted_seed_str = libbitcoin::encode_base16(seed);

  // generate a checksum using encrypted seed and passphrase hash
  libbitcoin::data_chunk extended(libbitcoin::to_chunk(passphrase_hash));
  libbitcoin::extend_data(extended, libbitcoin::to_chunk(seed));
  const auto checksum = megabit::utils::get_checksum(extended);
  const auto checksum_str = libbitcoin::encode_base16(checksum);

  std::cout << "VALIDATE Got passphrase hash: "
            << libbitcoin::encode_base16(passphrase_hash) << std::endl;
  std::cout << "VALIDATE Got encrypted seed: "
            << libbitcoin::encode_base16(seed) << std::endl;
  std::cout << "VALIDATE Got checksum: " << libbitcoin::encode_base16(checksum)
            << std::endl;

  // persist required settings
  QSettings settings("TheCodeFactory", "Megabit");
  settings.setValue("global/network", tr("mainnet"));
  settings.setValue(
      "global/creation_time",
      QString::fromStdString(megabit::utils::get_current_date_string()));
  settings.setValue("global/checksum", QString::fromStdString(checksum_str));
  settings.setValue("global/encrypted_seed",
                    QString::fromStdString(encrypted_seed_str));

  return true;
}

CreateWalletGenerate::~CreateWalletGenerate() {
  delete mnemonic_;
  delete mnemonic_flat_;
  delete main_label_;
  delete layout_;
}

int CreateWalletGenerate::nextId() const {
  return CreateWalletWizard::PageIds::Confirm;
}
