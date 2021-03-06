// Copyright (C) 2019 tribe29 GmbH - License: GNU General Public License v2
// This file is part of Checkmk (https://checkmk.com). It is subject to the
// terms and conditions defined in the file COPYING, which is part of this
// source code package.

#include "TableHostsByGroup.h"
#include "MonitoringCore.h"
#include "Query.h"
#include "Row.h"
#include "TableHostGroups.h"
#include "TableHosts.h"
#include "auth.h"
#include "nagios.h"

extern host *host_list;
extern hostgroup *hostgroup_list;

namespace {
struct hostbygroup {
    host hst;
    // cppcheck is too dumb to see usage in the DANGEROUS_OFFSETOF macro
    // cppcheck-suppress unusedStructMember
    hostgroup *host_group;
};
}  // namespace

TableHostsByGroup::TableHostsByGroup(MonitoringCore *mc) : Table(mc) {
    TableHosts::addColumns(this, "", -1, -1);
    TableHostGroups::addColumns(this, "hostgroup_",
                                DANGEROUS_OFFSETOF(hostbygroup, host_group));
}

std::string TableHostsByGroup::name() const { return "hostsbygroup"; }

std::string TableHostsByGroup::namePrefix() const { return "host_"; }

void TableHostsByGroup::answerQuery(Query *query) {
    bool requires_authcheck =
        query->authUser() != nullptr &&
        core()->groupAuthorization() == AuthorizationKind::strict;

    for (hostgroup *hg = hostgroup_list; hg != nullptr; hg = hg->next) {
        if (requires_authcheck &&
            !is_authorized_for_host_group(core(), hg, query->authUser())) {
            continue;
        }

        for (hostsmember *m = hg->members; m != nullptr; m = m->next) {
            hostbygroup hbg = {*m->host_ptr, hg};
            if (!query->processDataset(Row(&hbg))) {
                return;
            }
        }
    }
}

bool TableHostsByGroup::isAuthorized(Row row, const contact *ctc) const {
    return is_authorized_for(core(), ctc, &rowData<hostbygroup>(row)->hst,
                             nullptr);
}
