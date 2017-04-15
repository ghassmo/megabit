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

#ifndef __CREATE_WALLET_INTRODUCTION_HPP
#define __CREATE_WALLET_INTRODUCTION_HPP

#include <QLabel>
#include <QLineEdit>
#include <QRadioButton>
#include <QVBoxLayout>
#include <QWidget>
#include <QWizardPage>

class CreateWalletIntroduction : public QWizardPage {
  Q_OBJECT

 public:
  CreateWalletIntroduction(QWidget* parent = Q_NULLPTR);
  ~CreateWalletIntroduction();

  void cleanupPage() override;
  bool validatePage() override;
  int nextId() const override;

 private:
  QLabel* main_label_;
  QLabel* language_label_;
  QLabel* language_value_label_;
  QLabel* label1_;
  QLabel* label2_;
  QLineEdit* passphrase1_;
  QLineEdit* passphrase2_;

  QRadioButton* language_en_;
  QRadioButton* language_es_;
  QRadioButton* language_fr_;
  QRadioButton* language_it_;
  QRadioButton* language_ja_;
  QRadioButton* language_cs_;
  QRadioButton* language_ru_;
  QRadioButton* language_uk_;
  QRadioButton* language_zh_Hans_;
  QRadioButton* language_zh_Hant_;

  QVBoxLayout* layout_;
};

#endif  // __CREATE_WALLET_INTRODUCTION_HPP
