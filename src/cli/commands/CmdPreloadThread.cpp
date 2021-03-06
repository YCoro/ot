/************************************************************
 *
 *                 OPEN TRANSACTIONS
 *
 *       Financial Cryptography and Digital Cash
 *       Library, Protocol, API, Server, CLI, GUI
 *
 *       -- Anonymous Numbered Accounts.
 *       -- Untraceable Digital Cash.
 *       -- Triple-Signed Receipts.
 *       -- Cheques, Vouchers, Transfers, Inboxes.
 *       -- Basket Currencies, Markets, Payment Plans.
 *       -- Signed, XML, Ricardian-style Contracts.
 *       -- Scripted smart contracts.
 *
 *  EMAIL:
 *  fellowtraveler@opentransactions.org
 *
 *  WEBSITE:
 *  http://www.opentransactions.org/
 *
 *  -----------------------------------------------------
 *
 *   LICENSE:
 *   This Source Code Form is subject to the terms of the
 *   Mozilla Public License, v. 2.0. If a copy of the MPL
 *   was not distributed with this file, You can obtain one
 *   at http://mozilla.org/MPL/2.0/.
 *
 *   DISCLAIMER:
 *   This program is distributed in the hope that it will
 *   be useful, but WITHOUT ANY WARRANTY; without even the
 *   implied warranty of MERCHANTABILITY or FITNESS FOR A
 *   PARTICULAR PURPOSE.  See the Mozilla Public License
 *   for more details.
 *
 ************************************************************/

#include "CmdPreloadThread.hpp"

#include <opentxs/opentxs.hpp>

namespace opentxs
{
CmdPreloadThread::CmdPreloadThread()
{
    command = "preloadthread";
    args[0] = "--mynym <nym ID>";
    args[1] = "--thread <thread ID>";
    args[2] = "--start <count>";
    args[3] = "--items <count>";
    category = catOtherUsers;
    help = "Cache message in an activity thread.";
}

std::int32_t CmdPreloadThread::runWithOptions()
{
    return run(
        getOption("mynym"),
        getOption("thread"),
        getOption("start"),
        getOption("items"));
}

std::int32_t CmdPreloadThread::run(
    std::string mynym,
    const std::string& thread,
    const std::string& start,
    const std::string& items)
{
    if (!checkNym("mynym", mynym)) {
        return -1;
    }

    std::size_t begin{0};
    std::size_t count{0};

    try {
        begin = std::stoul(start);
    } catch (const std::out_of_range&) {
        begin = 0;
    } catch (const std::invalid_argument&) {
        begin = 0;
    }

    try {
        count = std::stoul(items);
    } catch (const std::out_of_range&) {
        count = 1;
    } catch (const std::invalid_argument&) {
        count = 1;
    }

    OT::App().Activity().PreloadThread(
        Identifier(mynym), Identifier(thread), begin, count);

    return 0;
}
}  // namespace opentxs
