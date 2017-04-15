#include "createwalletintroduction.hpp"

#include <bitcoin/bitcoin.hpp>
#include <iostream>

#include "createwalletwizard.hpp"

CreateWalletIntroduction::CreateWalletIntroduction(QWidget* parent)
    : QWizardPage(parent) {
  setTitle(tr("Create your Megabit Wallet"));
  setPixmap(QWizard::WatermarkPixmap, QPixmap(":/images/watermark.png"));

  const auto intro =
      "Welcome to Megabit!\n\nThis wizard will help you generate a new HD "
      "Bitcoin wallet.\n\nYou will need to enter a passphrase that will "
      "help secure your wallet and you will need to write down a set of words "
      "(called a mnemonic) in the selected language of your choosing.\n\n";

  main_label_ = new QLabel(tr(intro));
  main_label_->setWordWrap(true);

  language_label_ =
      new QLabel(tr("First select your preferred mnemonic language:"));
  language_en_ = new QRadioButton(tr("English"));
  language_es_ = new QRadioButton(tr("Spanish"));
  language_fr_ = new QRadioButton(tr("French"));
  language_it_ = new QRadioButton(tr("Italian"));
  language_ja_ = new QRadioButton(tr("Japanese"));
  language_cs_ = new QRadioButton(tr("Czech"));
  language_ru_ = new QRadioButton(tr("Russian"));
  language_uk_ = new QRadioButton(tr("Ukranian"));
  language_zh_Hans_ = new QRadioButton(tr("Simplified Chinese"));
  language_zh_Hant_ = new QRadioButton(tr("Traditional Chinese"));

  language_en_->setChecked(true);

  // create a non-visible label to register the preferred value
  language_value_label_ = new QLabel("en");
  registerField("createwallet.language", language_value_label_);

  label1_ = new QLabel(tr("Enter a passphrase:"));
  passphrase1_ = new QLineEdit();
  passphrase1_->setEchoMode(QLineEdit::Password);
  registerField("createwallet.passphrase1*", passphrase1_);

  label2_ = new QLabel(tr("Enter the same passphrase again:"));
  passphrase2_ = new QLineEdit();
  passphrase2_->setEchoMode(QLineEdit::Password);
  registerField("createwallet.passphrase2*", passphrase2_);

  layout_ = new QVBoxLayout();
  layout_->addWidget(main_label_);
  layout_->addWidget(language_label_);
  layout_->addWidget(language_en_);
  layout_->addWidget(language_es_);
  layout_->addWidget(language_fr_);
  layout_->addWidget(language_it_);
  layout_->addWidget(language_ja_);
  layout_->addWidget(language_cs_);
  layout_->addWidget(language_ru_);
  layout_->addWidget(language_uk_);
  layout_->addWidget(language_zh_Hans_);
  layout_->addWidget(language_zh_Hant_);
  layout_->addWidget(label1_);
  layout_->addWidget(passphrase1_);
  layout_->addWidget(label2_);
  layout_->addWidget(passphrase2_);
  setLayout(layout_);
}

CreateWalletIntroduction::~CreateWalletIntroduction() {
  delete label1_;
  delete label2_;
  delete layout_;
  delete main_label_;
  delete language_label_;
  delete language_value_label_;
  delete language_en_;
  delete language_es_;
  delete language_fr_;
  delete language_it_;
  delete language_ja_;
  delete language_cs_;
  delete language_ru_;
  delete language_uk_;
  delete language_zh_Hans_;
  delete language_zh_Hant_;
  delete passphrase1_;
  delete passphrase2_;
}

void CreateWalletIntroduction::cleanupPage() {
  passphrase1_->clear();
  passphrase2_->clear();
  passphrase1_->setText(tr(""));
  passphrase2_->setText(tr(""));
}

bool CreateWalletIntroduction::validatePage() {
  auto ret = false;
  if (field("createwallet.passphrase1").toString().toStdString().size()) {
    if (field("createwallet.passphrase1").toString() ==
        field("createwallet.passphrase2").toString()) {
      std::string language = "en";
      if (language_en_->isChecked()) language = "en";
      if (language_es_->isChecked()) language = "es";
      if (language_fr_->isChecked()) language = "fr";
      if (language_it_->isChecked()) language = "it";
      if (language_ja_->isChecked()) language = "ja";
      if (language_cs_->isChecked()) language = "cs";
      if (language_ru_->isChecked()) language = "ru";
      if (language_ru_->isChecked()) language = "uk";
      if (language_zh_Hans_->isChecked()) language = "zh_Hans";
      if (language_zh_Hant_->isChecked()) language = "zh_Hant";

      std::cout << "Setting language_value to be " << language << std::endl;
      auto language_str = QString::fromStdString(language);
      language_value_label_->setText(language_str);
      setField("createwallet.language", language_str);

      auto passphrase =
          field("createwallet.passphrase1").toString().toStdString();
      const auto start =
          reinterpret_cast<const unsigned char*>(passphrase.c_str());
      const auto end = start + passphrase.length();
      const auto passphrase_hash = libbitcoin::bitcoin_hash(
          libbitcoin::array_slice<uint8_t>(start, end));

      QString passphrase_hash_str =
          QString::fromStdString(libbitcoin::encode_base16(passphrase_hash));

      setField("createwallet.passphrase1", passphrase_hash_str);
      setField("createwallet.passphrase2", QString(tr("")));

      std::cout << "Using passphrase hash "
                << libbitcoin::encode_base16(passphrase_hash) << std::endl;
      std::cout << "Checked language is " << language << std::endl;

      ret = true;
    } else {
      QMessageBox::information(
          const_cast<decltype(this)>(this), tr("Passphrases do not match"),
          tr("Error: The passphrases entered do not match"));
    }
  }
  return ret;
}

int CreateWalletIntroduction::nextId() const {
  return CreateWalletWizard::PageIds::Generate;
}
