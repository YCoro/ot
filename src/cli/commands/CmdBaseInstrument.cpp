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

#include "CmdBaseInstrument.hpp"

#include <opentxs/opentxs.hpp>

#include <stdint.h>
#include <ostream>
#include <string>
#include <vector>

using namespace opentxs;
using namespace std;

CmdBaseInstrument::CmdBaseInstrument() {}

CmdBaseInstrument::~CmdBaseInstrument() {}

int32_t CmdBaseInstrument::sendPayment(
    const string& cheque,
    string sender,
    const char* what) const
{
    string server = SwigWrap::Instrmnt_GetNotaryID(cheque);
    if ("" == server) {
        otOut << "Error: cannot get server.\n";
        return -1;
    }

    if ("" == sender) {
        sender = SwigWrap::Instrmnt_GetSenderNymID(cheque);
        if ("" == sender) {
            otOut << "Error: cannot get sender.\n";
            return -1;
        }
    }

    string recipient = SwigWrap::Instrmnt_GetRecipientNymID(cheque);
    if ("" == recipient) {
        otOut << "Error: cannot get recipient.\n";
        return -1;
    }

    auto payment = std::make_shared<const OTPayment>(String(cheque.c_str()));

    std::string response;
    {
        response = OT::App()
                          .API()
                          .ServerAction()
                          .SendPayment(
                              Identifier(sender),
                              Identifier(server),
                              Identifier(recipient),
                              payment)
                          ->Run();
    }
    return processResponse(response, what);
}

string CmdBaseInstrument::writeCheque(
    string myacct,
    string hisnym,
    string amount,
    string memo,
    string validfor,
    bool isInvoice) const
{
    if (!checkAccount("myacct", myacct)) {
        return "";
    }

    if ("" != hisnym && !checkNym("hisnym", hisnym)) {
        return "";
    }

    int64_t value = checkAmount("amount", amount, myacct);
    if (OT_ERROR_AMOUNT == value) {
        return "";
    }

    if ("" != validfor && !checkValue("validfor", validfor)) {
        return "";
    }

    string server = SwigWrap::GetAccountWallet_NotaryID(myacct);
    if ("" == server) {
        otOut << "Error: cannot determine server from myacct.\n";
        return "";
    }

    string mynym = SwigWrap::GetAccountWallet_NymID(myacct);
    if ("" == mynym) {
        otOut << "Error: cannot determine mynym from myacct.\n";
        return "";
    }

    {
        if (!OT::App().API().ServerAction().GetTransactionNumbers(
                Identifier(mynym), Identifier(server), 10)) {
            otOut << "Error: cannot reserve transaction numbers.\n";
            return "";
        }
    }

    int64_t oneMonth = OTTimeGetSecondsFromTime(OT_TIME_MONTH_IN_SECONDS);
    int64_t timeSpan = "" != validfor ? stoll(validfor) : oneMonth;
    time64_t from = SwigWrap::GetTime();
    time64_t until = OTTimeAddTimeInterval(from, timeSpan);

    string cheque = SwigWrap::WriteCheque(
        server,
        isInvoice ? -value : value,
        from,
        until,
        myacct,
        mynym,
        memo,
        hisnym);
    if ("" == cheque) {
        otOut << "Error: cannot write cheque.\n";
        return "";
    }

    // Record it in the records?
    // Update: We wouldn't record that here. Instead, SwigWrap::WriteCheque
    // should drop a notice into the payments outbox, the same as it does when
    // you "sendcheque" (after all, the same resolution would be expected once
    // it is cashed.)

    return cheque;
}
