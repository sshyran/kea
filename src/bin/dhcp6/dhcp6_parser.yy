/* Copyright (C) 2015-2016 Internet Systems Consortium, Inc. ("ISC")

   This Source Code Form is subject to the terms of the Mozilla Public
   License, v. 2.0. If a copy of the MPL was not distributed with this
   file, You can obtain one at http://mozilla.org/MPL/2.0/. */

%skeleton "lalr1.cc" /* -*- C++ -*- */
%require "3.0.0"
%defines
%define parser_class_name {Dhcp6Parser}
%define api.prefix {parser6_}
%define api.token.constructor
%define api.value.type variant
%define api.namespace {isc::dhcp}
%define parse.assert
%code requires
{
#include <string>
#include <cc/data.h>
#include <dhcp/option.h>
#include <boost/lexical_cast.hpp>
#include <dhcp6/parser_context_decl.h>

using namespace isc::dhcp;
using namespace isc::data;
using namespace std;
}
// The parsing context.
%param { isc::dhcp::Parser6Context& ctx }
%locations
%define parse.trace
%define parse.error verbose
%code
{
#include <dhcp6/parser_context.h>

}

%define api.token.prefix {TOKEN_}
// Tokens in an order which makes sense and related to the intented use.
%token
  END  0  "end of file"
  COMMA ","
  COLON ":"
  LSQUARE_BRACKET "["
  RSQUARE_BRACKET "]"
  LCURLY_BRACKET "{"
  RCURLY_BRACKET "}"
  NULL_TYPE "null"

  DHCP6 "Dhcp6"
  INTERFACES_CONFIG "interfaces-config"
  INTERFACES "interfaces"

  LEASE_DATABASE "lease-database"
  HOSTS_DATABASE "hosts-database"
  TYPE "type"
  USER "user"
  PASSWORD "password"
  HOST "host"
  PERSIST "persist"
  LFC_INTERVAL "lfc-interval"

  PREFERRED_LIFETIME "preferred-lifetime"
  VALID_LIFETIME "valid-lifetime"
  RENEW_TIMER "renew-timer"
  REBIND_TIMER "rebind-timer"
  SUBNET6 "subnet6"
  OPTION_DATA "option-data"
  NAME "name"
  DATA "data"
  CODE "code"
  SPACE "space"
  CSV_FORMAT "csv-format"

  POOLS "pools"
  POOL "pool"
  PD_POOLS "pd-pools"
  PREFIX "prefix"
  PREFIX_LEN "prefix-len"
  DELEGATED_LEN "delegated-len"

  SUBNET "subnet"
  INTERFACE "interface"
  ID "id"

  MAC_SOURCES "mac-sources"
  RELAY_SUPPLIED_OPTIONS "relay-supplied-options"
  HOST_RESERVATION_IDENTIFIERS "host-reservation-identifiers"

  CLIENT_CLASSES "client-classes"
  TEST "test"
  CLIENT_CLASS "client-class"

  RESERVATIONS "reservations"
  IP_ADDRESSES "ip-addresses"
  PREFIXES "prefixes"
  DUID "duid"
  HW_ADDRESS "hw-address"
  HOSTNAME "hostname"

  HOOKS_LIBRARIES "hooks-libraries"
  LIBRARY "library"

  EXPIRED_LEASES_PROCESSING "expired-leases-processing"

  SERVER_ID "server-id"
  IDENTIFIER "identifier"
  HTYPE "htype"
  TIME "time"
  ENTERPRISE_ID "enterprise-id"

  DHCP4O6_PORT "dhcp4o6-port"

  LOGGING "Logging"
  LOGGERS "loggers"
  OUTPUT_OPTIONS "output_options"
  OUTPUT "output"
  DEBUGLEVEL "debuglevel"
  SEVERITY "severity"

  DHCP_DDNS "dhcp-ddns"
  ENABLE_UPDATES "enable-updates"
  QUALIFYING_SUFFIX "qualifying-suffix"

 // Not real tokens, just a way to signal what the parser is expected to
 // parse.
  TOPLEVEL_DHCP6
  TOPLEVEL_GENERIC_JSON
;

%token <std::string> STRING "constant string"
%token <int64_t> INTEGER "integer"
%token <double> FLOAT "floating point"
%token <bool> BOOLEAN "boolean"

%type <ElementPtr> value

%printer { yyoutput << $$; } <*>;

%%

// The whole grammar starts with a map, because the config file
// constists of Dhcp, Logger and DhcpDdns entries in one big { }.
// %start map - this will parse everything as generic JSON
// %start dhcp6_map - this will parse everything with Dhcp6 syntax checking
%start start;

start: TOPLEVEL_DHCP6 syntax_map
| TOPLEVEL_GENERIC_JSON map2;

// ---- generic JSON parser ---------------------------------

// Values rule
value : INTEGER { $$ = ElementPtr(new IntElement($1)); }
     | FLOAT { $$ = ElementPtr(new DoubleElement($1)); }
     | BOOLEAN { $$ = ElementPtr(new BoolElement($1)); }
     | STRING { $$ = ElementPtr(new StringElement($1)); }
     | NULL_TYPE { $$ = ElementPtr(new NullElement()); }
     | map2 { $$ = ctx.stack_.back(); ctx.stack_.pop_back(); }
     | list_generic { $$ = ctx.stack_.back(); ctx.stack_.pop_back(); }
    ;

map2: LCURLY_BRACKET {
    // This code is executed when we're about to start parsing
    // the content of the map
    ElementPtr m(new MapElement());
    ctx.stack_.push_back(m);
} map_content RCURLY_BRACKET {
    // map parsing completed. If we ever want to do any wrap up
    // (maybe some sanity checking), this would be the best place
    // for it.
};

// Assignments rule
map_content: %empty // empty map
    | STRING COLON value {
        // map containing a single entry
        ctx.stack_.back()->set($1, $3);
    }
    | map_content COMMA STRING COLON value {
        // map consisting of a shorter map followed by comma and string:value
        ctx.stack_.back()->set($3, $5);
    }
    ;

list_generic: LSQUARE_BRACKET {
    ElementPtr l(new ListElement());
    ctx.stack_.push_back(l);
 } list_content RSQUARE_BRACKET {

 }

// This one is used in syntax parser.
list2: LSQUARE_BRACKET {
    // List parsing about to start
} list_content RSQUARE_BRACKET {
    // list parsing complete. Put any sanity checking here
    //ctx.stack_.pop_back();
};

list_content: %empty // Empty list
    | value {
        // List consisting of a single element.
        ctx.stack_.back()->add($1);
    }
    | list_content COMMA value {
        // List ending with , and a value.
        ctx.stack_.back()->add($3);
    }
    ;

// ---- generic JSON parser ends here ----------------------------------

// ---- syntax checking parser starts here -----------------------------

// This defines the top-level { } that holds Dhcp6, Dhcp4, DhcpDdns or Logging
// objects.
syntax_map: LCURLY_BRACKET {
    // This code is executed when we're about to start parsing
    // the content of the map
    ElementPtr m(new MapElement());
    ctx.stack_.push_back(m);
} global_objects RCURLY_BRACKET {
    // map parsing completed. If we ever want to do any wrap up
    // (maybe some sanity checking), this would be the best place
    // for it.
};

// This represents a single top level entry, e.g. Dhcp6 or DhcpDdns.
global_object: dhcp6_object
| logging_object;

// This represents top-level entries: Dhcp6, Dhcp4, DhcpDdns, Logging
global_objects
: global_object
| global_objects COMMA global_object
;

dhcp6_object: DHCP6 COLON LCURLY_BRACKET {
    // This code is executed when we're about to start parsing
    // the content of the map
    ElementPtr m(new MapElement());
    ctx.stack_.back()->set("Dhcp6", m);
    ctx.stack_.push_back(m);
} global_params RCURLY_BRACKET {
    // map parsing completed. If we ever want to do any wrap up
    // (maybe some sanity checking), this would be the best place
    // for it.
    ctx.stack_.pop_back();
};

global_params: global_param
| global_params COMMA global_param;

// These are the parameters that are allowed in the top-level for
// Dhcp6.
global_param
: preferred_lifetime
| valid_lifetime
| renew_timer
| rebind_timer
| subnet6_list
| interfaces_config
| lease_database
| hosts_database
| mac_sources
| relay_supplied_options
| host_reservation_identifiers
| client_classes
| option_data_list
| hooks_libraries
| expired_leases_processing
| server_id
| dhcp4o6_port
| dhcp_ddns
;

preferred_lifetime: PREFERRED_LIFETIME COLON INTEGER {
    ElementPtr prf(new IntElement($3));
    ctx.stack_.back()->set("preferred-lifetime", prf);
};

valid_lifetime: VALID_LIFETIME COLON INTEGER {
    ElementPtr prf(new IntElement($3));
    ctx.stack_.back()->set("valid-lifetime", prf);
};

renew_timer: RENEW_TIMER COLON INTEGER {
    ElementPtr prf(new IntElement($3));
    ctx.stack_.back()->set("renew-timer", prf);
};

rebind_timer: REBIND_TIMER COLON INTEGER {
    ElementPtr prf(new IntElement($3));
    ctx.stack_.back()->set("rebind-timer", prf);
};

interfaces_config: INTERFACES_CONFIG COLON {
    ElementPtr i(new MapElement());
    ctx.stack_.back()->set("interfaces-config", i);
    ctx.stack_.push_back(i);
 } LCURLY_BRACKET interface_config_map RCURLY_BRACKET {
    ctx.stack_.pop_back();
};

interface_config_map: INTERFACES {
    ElementPtr l(new ListElement());
    ctx.stack_.back()->set("interfaces", l);
    ctx.stack_.push_back(l);
 } COLON list2 {
     ctx.stack_.pop_back();
 }

lease_database: LEASE_DATABASE {
    ElementPtr i(new MapElement());
    ctx.stack_.back()->set("lease-database", i);
    ctx.stack_.push_back(i);
}
COLON LCURLY_BRACKET database_map_params {
     ctx.stack_.pop_back();
} RCURLY_BRACKET;

hosts_database: HOSTS_DATABASE {
    ElementPtr i(new MapElement());
    ctx.stack_.back()->set("hosts-database", i);
    ctx.stack_.push_back(i);
}
COLON LCURLY_BRACKET database_map_params {
     ctx.stack_.pop_back();
} RCURLY_BRACKET;

database_map_params: lease_database_map_param
| database_map_params COMMA lease_database_map_param;

lease_database_map_param: type
| user
| password
| host
| name
| persist
| lfc_interval;
;

type: TYPE COLON STRING {
    ElementPtr prf(new StringElement($3));
    ctx.stack_.back()->set("type", prf);
};

user: USER COLON STRING {
    ElementPtr user(new StringElement($3));
    ctx.stack_.back()->set("user", user);
};

password: PASSWORD COLON STRING {
    ElementPtr pwd(new StringElement($3));
    ctx.stack_.back()->set("password", pwd);
};

host: HOST COLON STRING {
    ElementPtr h(new StringElement($3));
    ctx.stack_.back()->set("host", h);
};

name: NAME COLON STRING {
    ElementPtr n(new StringElement($3));
    ctx.stack_.back()->set("name", n);
};

persist: PERSIST COLON BOOLEAN {
    ElementPtr n(new BoolElement($3));
    ctx.stack_.back()->set("persist", n);
};

lfc_interval: LFC_INTERVAL COLON INTEGER {
    ElementPtr n(new IntElement($3));
    ctx.stack_.back()->set("lfc-interval", n);
};

mac_sources: MAC_SOURCES {
    ElementPtr l(new ListElement());
    ctx.stack_.back()->set("mac-sources", l);
    ctx.stack_.push_back(l);
} COLON LSQUARE_BRACKET mac_sources_list RSQUARE_BRACKET {
    ctx.stack_.pop_back();
};

mac_sources_list: mac_sources_value
| mac_sources_list COMMA mac_sources_value;

mac_sources_value: DUID {
    ElementPtr duid(new StringElement("duid")); ctx.stack_.back()->add(duid);
}| STRING {
    ElementPtr duid(new StringElement($1)); ctx.stack_.back()->add(duid);
};

host_reservation_identifiers: HOST_RESERVATION_IDENTIFIERS COLON LSQUARE_BRACKET {
    ElementPtr l(new ListElement());
    ctx.stack_.back()->set("host-reservation-identifiers", l);
    ctx.stack_.push_back(l);
} host_reservation_identifiers_list RSQUARE_BRACKET {
    ctx.stack_.pop_back();
};

host_reservation_identifiers_list: host_reservation_identifier
| host_reservation_identifiers_list COMMA host_reservation_identifier;

host_reservation_identifier: DUID {
    ElementPtr duid(new StringElement("duid")); ctx.stack_.back()->add(duid);
}
| HW_ADDRESS {
    ElementPtr hwaddr(new StringElement("hw-address")); ctx.stack_.back()->add(hwaddr);
}

relay_supplied_options: RELAY_SUPPLIED_OPTIONS {
    ElementPtr l(new ListElement());
    ctx.stack_.back()->set("relay-supplied-options", l);
    ctx.stack_.push_back(l);
} COLON list2 {
    ctx.stack_.pop_back();
};

hooks_libraries: HOOKS_LIBRARIES COLON {
    ElementPtr l(new ListElement());
    ctx.stack_.back()->set("hooks-libraries", l);
    ctx.stack_.push_back(l);
} LSQUARE_BRACKET hooks_libraries_list RSQUARE_BRACKET {
    ctx.stack_.pop_back();
};

hooks_libraries_list: %empty
| hooks_library
| hooks_libraries_list COMMA hooks_library;

hooks_library: LCURLY_BRACKET {
    ElementPtr m(new MapElement());
    ctx.stack_.back()->add(m);
    ctx.stack_.push_back(m);
} hooks_params RCURLY_BRACKET {
    ctx.stack_.pop_back();
};

hooks_params: hooks_param
| hooks_params hooks_param;

hooks_param: LIBRARY COLON STRING {
    ElementPtr lib(new StringElement($3)); ctx.stack_.back()->set("library", lib);
};

// --- expired-leases-processing ------------------------
expired_leases_processing: EXPIRED_LEASES_PROCESSING COLON LCURLY_BRACKET {
    ElementPtr m(new MapElement());
    ctx.stack_.back()->set("expired-leases-processing", m);
    ctx.stack_.push_back(m);
} expired_leases_params RCURLY_BRACKET {
    ctx.stack_.pop_back();
};

expired_leases_params: expired_leases_param
| expired_leases_params COMMA expired_leases_param;

// This is a bit of a simplification. But it can also serve as an example.
// Instead of explicitly listing all allowed expired leases parameters, we
// simply say that all of them as integers.
expired_leases_param: STRING COLON INTEGER {
    ElementPtr value(new IntElement($3)); ctx.stack_.back()->set($1, value);
}

// --- subnet6 ------------------------------------------
// This defines subnet6 as a list of maps.
// "subnet6": [ ... ]
subnet6_list: SUBNET6 COLON LSQUARE_BRACKET {
    ElementPtr l(new ListElement());
    ctx.stack_.back()->set("subnet6", l);
    ctx.stack_.push_back(l);
} subnet6_list_content RSQUARE_BRACKET {
    ctx.stack_.pop_back();
};

// This defines the ... in "subnet6": [ ... ]
// It can either be empty (no subnets defined), have one subnet
// or have multiple subnets separate by comma.
subnet6_list_content: %empty
| subnet6
| subnet6_list_content COMMA subnet6
;

// --- Subnet definitions -------------------------------

// This defines a single subnet, i.e. a single map with
// subnet6 array.
subnet6: LCURLY_BRACKET {
    ElementPtr m(new MapElement());
    ctx.stack_.back()->add(m);
    ctx.stack_.push_back(m);
} subnet6_params {
    // Once we reached this place, the subnet parsing is now complete.
    // If we want to, we can implement default values here.
    // In particular we can do things like this:
    // if (!ctx.stack_.back()->get("interface")) {
    //     ctx.stack_.back()->set("interface", StringElement("loopback"));
    // }
    //
    // We can also stack up one level (Dhcp6) and copy over whatever
    // global parameters we want to:
    // if (!ctx.stack_.back()->get("renew-timer")) {
    //     ElementPtr renew = ctx_stack_[...].get("renew-timer");
    //     if (renew) {
    //         ctx.stack_.back()->set("renew-timer", renew);
    //     }
    // }
    ctx.stack_.pop_back();
} RCURLY_BRACKET;

// This defines that subnet can have one or more parameters.
subnet6_params: subnet6_param
| subnet6_params COMMA subnet6_param;

// This defines a list of allowed parameters for each subnet.
subnet6_param: option_data_list
| pools_list
| pd_pools_list
| subnet
| interface
| id
| client_class
| reservations
;

subnet: SUBNET COLON STRING {
    ElementPtr subnet(new StringElement($3)); ctx.stack_.back()->set("subnet", subnet);
};

interface: INTERFACE COLON STRING {
    ElementPtr iface(new StringElement($3)); ctx.stack_.back()->set("interface", iface);
};

subnet: CLIENT_CLASS COLON STRING {
    ElementPtr cls(new StringElement($3)); ctx.stack_.back()->set("client-class", cls);
};

id: ID COLON INTEGER {
    ElementPtr id(new IntElement($3)); ctx.stack_.back()->set("id", id);

};

// ---- option-data --------------------------

// This defines the "option-data": [ ... ] entry that may appear
// in several places, but most notably in subnet6 entries.
option_data_list: OPTION_DATA {
    ElementPtr l(new ListElement());
    ctx.stack_.back()->set("option-data", l);
    ctx.stack_.push_back(l);
} COLON LSQUARE_BRACKET option_data_list_content RSQUARE_BRACKET {
    ctx.stack_.pop_back();
};

// This defines the content of option-data. It may be empty,
// have one entry or multiple entries separated by comma.
option_data_list_content: %empty
| option_data_entry
| option_data_list_content COMMA option_data_entry;

// This defines th content of a single entry { ... } within
// option-data list.
option_data_entry: LCURLY_BRACKET {
    ElementPtr m(new MapElement());
    ctx.stack_.back()->add(m);
    ctx.stack_.push_back(m);
} option_data_params RCURLY_BRACKET {
    ctx.stack_.pop_back();
};

// This defines parameters specified inside the map that itself
// is an entry in option-data list.
option_data_params: option_data_param
| option_data_params COMMA option_data_param;

option_data_param: %empty
| option_data_name
| option_data_data
| option_data_code
| option_data_space
| option_data_csv_format
;


option_data_name: NAME COLON STRING {
    ElementPtr name(new StringElement($3)); ctx.stack_.back()->set("name", name);
};

option_data_data: DATA COLON STRING {
    ElementPtr data(new StringElement($3)); ctx.stack_.back()->set("data", data);
};

option_data_code: CODE COLON INTEGER {
    ElementPtr code(new IntElement($3)); ctx.stack_.back()->set("code", code);
};

option_data_space: SPACE COLON STRING {
    ElementPtr space(new StringElement($3)); ctx.stack_.back()->set("space", space);
};

option_data_csv_format: CSV_FORMAT COLON BOOLEAN {
    ElementPtr space(new BoolElement($3)); ctx.stack_.back()->set("csv-format", space);
};

// ---- pools ------------------------------------

// This defines the "pools": [ ... ] entry that may appear in subnet6.
pools_list: POOLS COLON {
    ElementPtr l(new ListElement());
    ctx.stack_.back()->set("pools", l);
    ctx.stack_.push_back(l);
} LSQUARE_BRACKET pools_list_content RSQUARE_BRACKET {
    ctx.stack_.pop_back();
};

// Pools may be empty, contain a single pool entry or multiple entries
// separate by commas.
pools_list_content: %empty
| pool_entry
| pools_list_content COMMA pool_entry;

pool_entry: LCURLY_BRACKET {
    ElementPtr m(new MapElement());
    ctx.stack_.back()->add(m);
    ctx.stack_.push_back(m);
} pool_params RCURLY_BRACKET {
    ctx.stack_.pop_back();
};

pool_params: pool_param
| pool_params COMMA pool_param;

pool_param: POOL COLON STRING {
    ElementPtr pool(new StringElement($3)); ctx.stack_.back()->set("pool", pool);
}
| option_data_list;
// --- end of pools definition -------------------------------

// --- pd-pools ----------------------------------------------
pd_pools_list: PD_POOLS COLON {
    ElementPtr l(new ListElement());
    ctx.stack_.back()->set("pd-pools", l);
    ctx.stack_.push_back(l);
} LSQUARE_BRACKET pd_pools_list_content RSQUARE_BRACKET {
    ctx.stack_.pop_back();
};

// Pools may be empty, contain a single pool entry or multiple entries
// separate by commas.
pd_pools_list_content: %empty
| pd_pool_entry
| pd_pools_list_content COMMA pd_pool_entry;

pd_pool_entry: LCURLY_BRACKET {
    ElementPtr m(new MapElement());
    ctx.stack_.back()->add(m);
    ctx.stack_.push_back(m);
} pd_pool_params RCURLY_BRACKET {
    ctx.stack_.pop_back();
};

pd_pool_params: pd_pool_param
| pd_pool_params COMMA pd_pool_param;

pd_pool_param: pd_prefix
| pd_prefix_len
| pd_delegated_len
| option_data_list
;

pd_prefix: PREFIX COLON STRING {
    ElementPtr prf(new StringElement($3)); ctx.stack_.back()->set("prefix", prf);
}

pd_prefix_len: PREFIX_LEN COLON INTEGER {
    ElementPtr prf(new IntElement($3)); ctx.stack_.back()->set("prefix-len", prf);
}

pd_delegated_len: DELEGATED_LEN COLON INTEGER {
    ElementPtr deleg(new IntElement($3)); ctx.stack_.back()->set("delegated-len", deleg);
}



// --- end of pd-pools ---------------------------------------

// --- reservations ------------------------------------------
reservations: RESERVATIONS COLON LSQUARE_BRACKET {
    ElementPtr l(new ListElement());
    ctx.stack_.back()->set("reservations", l);
    ctx.stack_.push_back(l);
} reservations_list {
    ctx.stack_.pop_back();
} RSQUARE_BRACKET;

reservations_list: %empty
| reservation
| reservations_list COMMA reservation;

reservation: LCURLY_BRACKET {
    ElementPtr m(new MapElement());
    ctx.stack_.back()->add(m);
    ctx.stack_.push_back(m);
} reservation_params RCURLY_BRACKET {
    ctx.stack_.pop_back();
};

reservation_params: reservation_param
| reservation_params COMMA reservation_param;

// @todo probably need to add mac-address as well here
reservation_param: %empty
| duid
| reservation_client_classes
| ip_addresses
| prefixes
| hw_address
| hostname
| option_data_list
;

ip_addresses: IP_ADDRESSES COLON {
    ElementPtr l(new ListElement());
    ctx.stack_.back()->set("ip-addresses", l);
    ctx.stack_.push_back(l);
} list2 {
    ctx.stack_.pop_back();
};

prefixes: PREFIXES COLON  {
    ElementPtr l(new ListElement());
    ctx.stack_.back()->set("prefixes", l);
    ctx.stack_.push_back(l);
} list2 {
    ctx.stack_.pop_back();
};

duid: DUID COLON STRING {
    ElementPtr d(new StringElement($3)); ctx.stack_.back()->set("duid", d);
};

hw_address: HW_ADDRESS COLON STRING {
    ElementPtr hw(new StringElement($3)); ctx.stack_.back()->set("hw-address", hw);
};

hostname: HOSTNAME COLON STRING {
    ElementPtr host(new StringElement($3)); ctx.stack_.back()->set("hostname", host);
}

reservation_client_classes: CLIENT_CLASSES COLON {
    ElementPtr c(new ListElement());
    ctx.stack_.back()->set("client-classes", c);
    ctx.stack_.push_back(c);
} list2 {
    ctx.stack_.pop_back();
  };

// --- end of reservations definitions -----------------------

// --- client classes ----------------------------------------
client_classes: CLIENT_CLASSES COLON LSQUARE_BRACKET {
    ElementPtr l(new ListElement());
    ctx.stack_.back()->set("client-classes", l);
    ctx.stack_.push_back(l);
} client_classes_list RSQUARE_BRACKET {
    ctx.stack_.pop_back();
};

client_classes_list: client_class
| client_classes_list COMMA client_class;

client_class: LCURLY_BRACKET {
    ElementPtr m(new MapElement());
    ctx.stack_.back()->add(m);
    ctx.stack_.push_back(m);
} client_class_params RCURLY_BRACKET {
    ctx.stack_.pop_back();
};

client_class_params: client_class_param
| client_class_params COMMA client_class_param;

client_class_param: %empty
| client_class_name
| client_class_test
| option_data_list
;

client_class_name: NAME COLON STRING {
    ElementPtr name(new StringElement($3));
    ctx.stack_.back()->set("name", name);
};

client_class_test: TEST COLON STRING {
    ElementPtr test(new StringElement($3));
    ctx.stack_.back()->set("test", test);
}


// --- end of client classes ---------------------------------

// --- server-id ---------------------------------------------
server_id: SERVER_ID COLON LCURLY_BRACKET {
    ElementPtr m(new MapElement());
    ctx.stack_.back()->set("server-id", m);
    ctx.stack_.push_back(m);
} server_id_params RCURLY_BRACKET {
    ctx.stack_.pop_back();
};

server_id_params: server_id_param
| server_id_params COMMA server_id_param;

server_id_param: type
| identifier
| time
| htype
| enterprise_id
| persist;

htype: HTYPE COLON INTEGER {
    ElementPtr htype(new IntElement($3));
    ctx.stack_.back()->set("htype", htype);
};

identifier: IDENTIFIER COLON STRING {
    ElementPtr id(new StringElement($3));
    ctx.stack_.back()->set("identifier", id);
};

time: TIME COLON INTEGER {
    ElementPtr time(new IntElement($3));
    ctx.stack_.back()->set("time", time);
};

enterprise_id: ENTERPRISE_ID COLON INTEGER {
    ElementPtr time(new IntElement($3));
    ctx.stack_.back()->set("enterprise-id", time);
};

// --- end of server-id --------------------------------------

dhcp4o6_port: DHCP4O6_PORT COLON INTEGER {
    ElementPtr time(new IntElement($3));
    ctx.stack_.back()->set("dhcp4o6-port", time);
};

// --- logging entry -----------------------------------------

// This defines the top level "Logging" object. It parses
// the following "Logging": { ... }. The ... is defined
// by logging_params
logging_object: LOGGING COLON LCURLY_BRACKET {
    ElementPtr m(new MapElement());
    ctx.stack_.back()->set("Logging", m);
    ctx.stack_.push_back(m);
} logging_params RCURLY_BRACKET {
    ctx.stack_.pop_back();
};

// This defines the list of allowed parameters that may appear
// in the top-level Logging object. It can either be a single
// parameter or several parameters separated by commas.
logging_params: logging_param
| logging_params COMMA logging_param;

// There's currently only one parameter defined, which is "loggers".
logging_param: loggers;

// "loggers", the only parameter currently defined in "Logging" object,
// is "Loggers": [ ... ].
loggers: LOGGERS COLON {
    ElementPtr l(new ListElement());
    ctx.stack_.back()->set("loggers", l);
    ctx.stack_.push_back(l);
} LSQUARE_BRACKET loggers_entries RSQUARE_BRACKET {
    ctx.stack_.pop_back();
};

// These are the parameters allowed in loggers: either one logger
// entry or multiple entries separate by commas.
loggers_entries: logger_entry
| loggers_entries COMMA logger_entry;

// This defines a single entry defined in loggers in Logging.
logger_entry: LCURLY_BRACKET {
    ElementPtr l(new MapElement());
    ctx.stack_.back()->add(l);
    ctx.stack_.push_back(l);
} logger_params RCURLY_BRACKET {
    ctx.stack_.pop_back();
};

logger_params: logger_param
| logger_params COMMA logger_param;

logger_param: logger_name
| output_options_list
| debuglevel
| severity
;

logger_name: NAME COLON STRING {
    ElementPtr name(new StringElement($3)); ctx.stack_.back()->set("name", name);
};

debuglevel: DEBUGLEVEL COLON INTEGER {
    ElementPtr dl(new IntElement($3)); ctx.stack_.back()->set("debuglevel", dl);
};
severity: SEVERITY COLON STRING {
    ElementPtr sev(new StringElement($3)); ctx.stack_.back()->set("severity", sev);
};

output_options_list: OUTPUT_OPTIONS COLON {
    ElementPtr l(new ListElement());
    ctx.stack_.back()->set("output_options", l);
    ctx.stack_.push_back(l);
} LSQUARE_BRACKET output_options_list_content RSQUARE_BRACKET {
    ctx.stack_.pop_back();
};

output_options_list_content: output_entry
| output_options_list_content COMMA output_entry;

output_entry: LCURLY_BRACKET {
    ElementPtr m(new MapElement());
    ctx.stack_.back()->add(m);
    ctx.stack_.push_back(m);
} output_params RCURLY_BRACKET {
    ctx.stack_.pop_back();
};

output_params: output_param
| output_params COMMA output_param;

output_param: OUTPUT COLON STRING {
    ElementPtr sev(new StringElement($3)); ctx.stack_.back()->set("output", sev);
};

dhcp_ddns: DHCP_DDNS COLON LCURLY_BRACKET {
    ElementPtr m(new MapElement());
    ctx.stack_.back()->set("dhcp-ddns", m);
    ctx.stack_.push_back(m);
} dhcp_ddns_params RCURLY_BRACKET {
    ctx.stack_.pop_back();
};

dhcp_ddns_params: dhcp_ddns_param
| dhcp_ddns_params COMMA dhcp_ddns_param
;

dhcp_ddns_param: enable_updates
| qualifying_suffix
;

enable_updates: ENABLE_UPDATES COLON BOOLEAN {
    ElementPtr b(new BoolElement($3));
    ctx.stack_.back()->set("enable-updates", b);
};

qualifying_suffix: QUALIFYING_SUFFIX COLON STRING {
    ElementPtr qs(new StringElement($3));
    ctx.stack_.back()->set("qualifying-suffix", qs);
};

%%

void
isc::dhcp::Dhcp6Parser::error(const location_type& loc,
                              const std::string& what)
{
    ctx.error(loc, what);
}
