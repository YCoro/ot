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

#include "CmdShowProfile.hpp"

#include <opentxs/opentxs.hpp>

namespace opentxs
{
CmdShowProfile::CmdShowProfile()
{
    command = "showprofile";
    args[0] = "--mynym <id>";
    category = catNyms;
    help = "Display profile data";
}

void CmdShowProfile::display_groups(const ui::ProfileSection& section) const
{
    auto group = section.First();

    if (false == group->Valid()) {

        return;
    }

    auto lastGroup = group->Last();
    otOut << "** " << group->Name("en") <<  " (" << group->Type()
          << ")\n";
    display_items(group);

    while (false == lastGroup) {
        group = section.Next();
        lastGroup = group->Last();
        otOut << "** " << group->Name("en") <<  " (" << group->Type()
              << ")\n";
        display_items(group);
    }
}

void CmdShowProfile::display_items(const ui::ProfileSubsection& group) const
{
    auto item = group.First();

    if (false == item->Valid()) {

        return;
    }

    auto lastItem = item->Last();
    otOut << "  * ID: " << item->ClaimID() << " Value: " << item->Value()
          <<  " Primary: " << item->IsPrimary() << " Active: "
          << item->IsActive() << "\n";

    while (false == lastItem) {
        item = group.Next();
        lastItem = item->Last();
        otOut << "  * ID: " << item->ClaimID() << " Value: " << item->Value()
              <<  " Primary: " << item->IsPrimary() << " Active: "
              << item->IsActive() << "\n";
    }
}

std::int32_t CmdShowProfile::runWithOptions()
{
    return run(getOption("mynym"));
}

std::int32_t CmdShowProfile::run(std::string mynym)
{
    if (!checkNym("mynym", mynym)) {
        return -1;
    }

    const Identifier nymID{mynym};
    auto& profile = OT::App().UI().Profile(nymID);
    otOut << profile.DisplayName() << ": (" << profile.ID()
          << ")\nPayment Code: " << profile.PaymentCode() << "\n\n";
    dashLine();
    auto section = profile.First();

    if (false == section->Valid()) {

        return 1;
    }

    auto lastSection = section->Last();
    otOut << "* " << section->Name("en") <<  " (" << section->Type()
          << ")\n";
          display_groups(section);

    while (false == lastSection) {
        section = profile.Next();
        lastSection = section->Last();
        otOut << "* " << section->Name("en") <<  " (" << section->Type()
              << ")\n";
        display_groups(section);
    }

    otOut << std::endl;

    return 1;
}
}  // namespace opentxs
