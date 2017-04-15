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

#include "../include/megabit/block_height_thread.hpp"

#include <QTimer>

#include "../include/megabit/bitcoin_interface.hpp"

void BlockHeightThread::GetBlockHeight() {
  std::cout << "[THREAD] GetBlockHeight called" << std::endl;

  auto on_error = [this](const libbitcoin::code& error) {
    std::stringstream error_ss;
    error_ss << "Failed to retrieve block height: ";
    error_ss << error;
    std::cout << error_ss.str() << std::endl;
    emit BlockHeightError(QString::fromUtf8(error_ss.str().c_str()));
  };

  auto handler = [this](size_t height) {
    block_height_ = height;
    std::cout << "Set block height to " << block_height_ << std::endl;
    emit finished();
  };

  // NOTE: this is a blocking call, which is why it's in a
  // separate thread
  bitcoin_interface_.GetBlockHeight(on_error, handler);
}
