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

#ifndef __SETTINGS_HPP
#define __SETTINGS_HPP

#include <QComboBox>
#include <QDialog>
#include <QLineEdit>
#include <QMessageBox>

class Configuration;

class Settings : public QDialog {
  Q_OBJECT

 public:
  explicit Settings(Configuration& config) : config_(config) {}
  ~Settings();
  void InitDialog();

 public slots:
  void ApplySettings();
  void NetworkIndexChanged(QString network);
  void CurrencyIndexChanged(QString currency);

 private:
  void reject();

  QLineEdit* server_line_edit_;
  QLineEdit* server_pk_line_edit_;
  QComboBox* currency_combo_;
  Configuration& config_;
};

#endif  // __SETTINGS_HPP
