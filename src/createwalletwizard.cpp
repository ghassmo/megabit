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

#include "../include/megabit/createwalletwizard.hpp"

#include "../include/megabit/createwalletconfirm.hpp"
#include "../include/megabit/createwalletgenerate.hpp"
#include "../include/megabit/createwalletintroduction.hpp"

CreateWalletWizard::CreateWalletWizard(QWidget* parent) : QWizard(parent) {
  setPage(CreateWalletWizard::PageIds::Introduction,
          new CreateWalletIntroduction());
  setPage(CreateWalletWizard::PageIds::Generate, new CreateWalletGenerate());
  setPage(CreateWalletWizard::PageIds::Confirm, new CreateWalletConfirm());

  setStartId(CreateWalletWizard::PageIds::Introduction);

#ifndef Q_OS_MAC
  setWizardStyle(ModernStyle);
#endif

  setOption(HaveHelpButton, true);
  setPixmap(QWizard::LogoPixmap, QPixmap(":/images/logo.png"));

  connect(this, &QWizard::helpRequested, this, &CreateWalletWizard::showHelp);

  setWindowTitle(tr("Create your Megabit Wallet"));
}

void CreateWalletWizard::showHelp() {
  static QString lastHelpMessage;

  QString message;

  switch (currentId()) {
    case CreateWalletWizard::PageIds::Introduction:
      message = tr("Introducing the Megabit Bitcoin Wallet");
      break;
    case CreateWalletWizard::PageIds::Generate:
      message = tr("Let's Generate a new HD Bitcoin wallet");
      break;
    case CreateWalletWizard::PageIds::Confirm:
      message =
          tr("Confirm that this information about your "
             "new wallet is correct");
      break;
    default:
      message = tr("Unknown help information requested.");
      break;
  }

  if (lastHelpMessage == message) {
    message =
        tr("Unfortunately, this is all the help that is available. "
           "Please post an issue on the project's github, or contact "
           "the developers.");
  }

  QMessageBox::information(this, tr("Create Megabit Wallet Help"), message);

  lastHelpMessage = message;
}
