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

#include "CmdExportCash.hpp"

#include <opentxs/opentxs.hpp>

#include <stdint.h>
#include <iostream>
#include <string>
#include <vector>

using namespace opentxs;
using namespace std;

CmdExportCash::CmdExportCash()
{
    command = "exportcash";
    args[0] = "--server <server>";
    args[1] = "--mynym <nym>";
    args[2] = "--mypurse <purse>";
    args[3] = "[--hisnym <nym>]";
    args[4] = "[--indices <indices|all>]";
    args[5] = "[--password <true|false>]";
    category = catInstruments;
    help = "Export a cash purse.";
    usage = "When password-protected, --hisnym is ignored.\n"
            "Otherwise cash is exported to hisnym instead of mynym.";
}

CmdExportCash::~CmdExportCash() {}

int32_t CmdExportCash::runWithOptions()
{
    return run(
        getOption("server"),
        getOption("mynym"),
        getOption("mypurse"),
        getOption("hisnym"),
        getOption("indices"),
        getOption("password"));
}

int32_t CmdExportCash::run(
    string server,
    string mynym,
    string mypurse,
    string hisnym,
    string indices,
    string password)
{
    if (!checkServer("server", server)) {
        return -1;
    }

    if (!checkNym("mynym", mynym)) {
        return -1;
    }

    if (!checkPurse("mypurse", mypurse)) {
        return -1;
    }

    if ("" != password && !checkFlag("password", password)) {
        return -1;
    }

    // if password is true, hisnym becomes optional
    if (password != "true" && "" != hisnym && !checkNym("hisnym", hisnym)) {
        return -1;
    }

    if ("" != indices && !checkIndices("indices", indices)) {
        return -1;
    }

    if (password != "true" && "" == hisnym) {
        hisnym = mynym;
    }

    string retainedCopy = "";
    string purse = exportCash(
        server,
        mynym,
        mypurse,
        hisnym,
        indices,
        password == "true",
        retainedCopy);
    if (purse.empty()) {
        return -1;
    }

    cout << purse << "\n";

    return 1;
}

// MyPurse and HisPurse will call the script even when not found, giving the
// script the opportunity to download the appropriate asset contract from the
// server, if necessary.
//
// Similarly, HisAcct works without being expected to be found in the wallet
// (since maybe it's HIS account and thus it's not IN your wallet...)
//
// Similarly, HisNym will call the script even when not found, giving the script
// the opportunity to download the appropriate pubkey ("check_nym" aka
// "checknym") and continue operating.
//
// All of the above, plus Server, ALREADY attempt a partial match search.
// Therefore, it's not necessary to perform ANOTHER partial match, when the
// value comes from --server, --mynym, --hisnym, --mypurse, --hispurse,
// --myacct, or --hisacct.
//
// You only need to do partial matches when you get values from ELSEWHERE, such
// as from custom arguments, or user-pasted input.
//
// Therefore, add a function for downloading a Nym's pubkey if not already in
// the wallet, and one that downloads an asset contract if not already in the
// wallet.

// The first three arguments are for loading up the purse we're exporting FROM.
//
// Then hisnym is for the Nym we're exporting TO (if hasPassword is false.)
//
// Then indices contains the indices for the tokens to export.
//
// Then hasPassword tells us whether to export to hisnym, or to create a
// password-protected purse and export to that instead. (In which case, the
// purse itself becomes the "owner" and can be passed wherever we would
// normally pass a NymID as a purse owner.)
//
// if hasPassword is false, then hisnym needs to contain the recipient Nym.
// This will contain MyNym by default, if HisNym wasn't provided.

string CmdExportCash::exportCash(
    const string& server,
    const string& mynym,
    const string& assetType,
    string& hisnym,
    const string& indices,
    bool hasPassword,
    string& retainedCopy) const
{
#if OT_CASH
    std::string strContract = SwigWrap::GetAssetType_Contract(assetType);

    if (!VerifyStringVal(strContract)) {
        std::string response;
        {
            response = OT::App()
                                      .API()
                                      .ServerAction()
                                      .DownloadContract(
                                          Identifier(mynym),
                                          Identifier(server),
                                          Identifier(assetType))
                                      ->Run();
        }

        if (1 == VerifyMessageSuccess(response)) {
            strContract = SwigWrap::GetAssetType_Contract(assetType);
        }
    }
    if (strContract.empty()) {
        otOut << "CmdExportCash::exportCash: Error: cannot load asset "
                 "contract.\n";
        return {};
    }
    // -------------------------------------------------------------------
    if (!hasPassword) {
        // If no recipient, then recipient == Nym.
        //
        if (hisnym.empty()) {
            otOut << "CmdExportCash::exportCash: recipientNym empty--using "
                     "mynym for recipient instead: "
                  << mynym << "\n";
            hisnym = mynym;
        }

        if (!(hisnym == mynym)) {
            // Even though we don't use this variable after this point,
            // we've still done something important: loaded and possibly
            // downloaded the recipient Nym, so that later in this function
            // we can reference that hisnym in other calls and we know
            // it will work.
            //
            // This function handles partial IDs for recipient.
            //
            std::string recipientPubKey =
                load_or_retrieve_encrypt_key(server, mynym, hisnym);

            if (!VerifyStringVal(recipientPubKey)) {
                otOut << "CmdExportCash::exportCash: recipientPubKey is null\n";
                return {};
            }
        }
    }
    // By this point, we have verified that we can load the public key
    // for the recipient.
    // (IF the exported purse isn't meant to be password-protected.)
    //
    return OT::App().API().Cash().export_cash(
        server, mynym, assetType, hisnym, indices, hasPassword, retainedCopy);
#else
    return {};
#endif  // OT_CASH
}

// load_or_retrieve_pubkey()
//
// Load targetNymID from local storage.
// If not there, then retrieve targetNymID from server,
// using nymID to send check_nym request. Then re-load
// and return. (Might still return null.)
//
std::string CmdExportCash::load_or_retrieve_encrypt_key(
    const std::string& notaryID,
    const std::string& nymID,
    const std::string& targetNymID) const
{
    std::string strPubkey = load_public_encryption_key(targetNymID);

    if (!VerifyStringVal(strPubkey)) {
        std::string strResponse = check_nym(notaryID, nymID, targetNymID);

        if (1 == VerifyMessageSuccess(strResponse)) {
            strPubkey = load_public_encryption_key(targetNymID);
        }
    }

    return strPubkey;  // might be null.
}

// load_public_key():
//
// Load a public key from local storage, and return it (or null).
//
// TODO: Need to fix ugly error messages by passing a bChecking in here
// so the calling function can try to load the pubkey just to see if it's there,
// without causing ugly error logs when there's no error.
std::string CmdExportCash::load_public_encryption_key(
    const std::string& nymID) const
{
    otOut << "\nload_public_encryption_key: Trying to load public "
             "key, assuming Nym isn't in the local wallet...\n";
    std::string strPubkey = SwigWrap::LoadPubkey_Encryption(
        nymID);  // This version is for "other people";

    if (!VerifyStringVal(strPubkey)) {
        otOut << "\nload_public_encryption_key: Didn't find the Nym (" << nymID
              << ") as an 'other' user, so next, checking to see if there's "
                 "a pubkey available for one of the local private Nyms...\n";
        strPubkey = SwigWrap::LoadUserPubkey_Encryption(
            nymID);  // This version is for "the user sitting at the machine.";

        if (!VerifyStringVal(strPubkey)) {
            otOut << "\nload_public_encryption_key: Didn't find "
                     "him as a local Nym either... returning nullptr.\n";
        }
    }

    return strPubkey;  // might be null.;
}
