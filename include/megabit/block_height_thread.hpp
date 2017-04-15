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

#ifndef __BLOCK_HEIGHT_THREAD_HPP
#define __BLOCK_HEIGHT_THREAD_HPP

#include <QThread>

#include "bitcoin_interface.hpp"

class BlockHeightThread : public QObject {
  Q_OBJECT

 public:
  explicit BlockHeightThread(BitcoinInterface& bitcoin_interface,
                             size_t& block_height)
      : bitcoin_interface_(bitcoin_interface), block_height_(block_height) {}

  ~BlockHeightThread() {}

 public slots:
  void GetBlockHeight();

 signals:
  void finished();
  void BlockHeightError(QString error);

 private:
  BitcoinInterface& bitcoin_interface_;
  size_t& block_height_;
};

#endif  // __BLOCK_HEIGHT_THREAD_HPP
