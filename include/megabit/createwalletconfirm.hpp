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

#ifndef __CREATE_WALLET_CONFIRM_HPP
#define __CREATE_WALLET_CONFIRM_HPP

#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QWidget>
#include <QWizardPage>
#include <vector>

class CreateWalletConfirm : public QWizardPage {
  Q_OBJECT

 public:
  CreateWalletConfirm(QWidget* parent = Q_NULLPTR);
  ~CreateWalletConfirm();

  void initializePage() override;
  bool validatePage() override;
  int nextId() const override;

 private:
  int confirm_index1_;
  int confirm_index2_;
  QLabel* main_label_;
  QLineEdit* confirm1_;
  QLineEdit* confirm2_;
  QLabel* confirm_label1_;
  QLabel* confirm_label2_;
  QLayout* layout_;
  std::vector<std::string> word_list_;
};

#endif  // __CREATE_WALLET_CONFIRM_HPP
