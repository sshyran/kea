// Copyright (C) 2010  Internet Systems Consortium, Inc. ("ISC")
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
// REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
// AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
// LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
// OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#include <config.h>

// Workaround for a problem with boost and sunstudio 5.10
// There is a version check in there that appears wrong,
// which makes including boost/thread.hpp fail
// This will probably be fixed in a future version of boost,
// in which case this part can be removed then
#ifdef NEED_SUNPRO_WORKAROUND
#if defined(__SUNPRO_CC) && __SUNPRO_CC == 0x5100
#undef __SUNPRO_CC
#define __SUNPRO_CC 0x5090
#endif
#endif // NEED_SUNPRO_WORKAROUND


#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/foreach.hpp>
#include <boost/bind.hpp>

#include <config.h>
#include <dns/rdataclass.h>

#include "hash_table.h"
#include "lru_list.h"
#include "hash_deleter.h"
#include "nsas_entry_compare.h"
#include "nameserver_entry.h"
#include "nameserver_address_store.h"
#include "zone_entry.h"
#include "address_request_callback.h"

using namespace isc::dns;
using namespace std;

namespace isc {
namespace nsas {

// Constructor.
//
// The LRU lists are set equal to three times the size of the respective
// hash table, on the assumption that three elements is the longest linear
// search we want to do when looking up names in the hash table.
NameserverAddressStore::NameserverAddressStore(
    boost::shared_ptr<ResolverInterface> resolver, uint32_t zonehashsize,
    uint32_t nshashsize) :
    zone_hash_(new HashTable<ZoneEntry>(new NsasEntryCompare<ZoneEntry>,
        zonehashsize)),
    nameserver_hash_(new HashTable<NameserverEntry>(
        new NsasEntryCompare<NameserverEntry>, nshashsize)),
    zone_lru_(new LruList<ZoneEntry>((3 * zonehashsize),
        new HashDeleter<ZoneEntry>(*zone_hash_))),
    nameserver_lru_(new LruList<NameserverEntry>((3 * nshashsize),
        new HashDeleter<NameserverEntry>(*nameserver_hash_))),
    resolver_(resolver)
{ }

namespace {

/*
 * We use pointers here so there's no call to any copy constructor.
 * It is easier for the compiler to inline it and prove that there's
 * no need to copy anything. In that case, the bind should not be
 * called at all to create the object, just call the function.
 */
boost::shared_ptr<ZoneEntry>
newZone(const boost::shared_ptr<ResolverInterface>* resolver, const string* zone,
    const RRClass* class_code,
    const boost::shared_ptr<HashTable<NameserverEntry> >* ns_hash,
    const boost::shared_ptr<LruList<NameserverEntry> >* ns_lru)
{
    boost::shared_ptr<ZoneEntry> result(new ZoneEntry(*resolver, *zone, *class_code,
        *ns_hash, *ns_lru));
    return (result);
}

}

void
NameserverAddressStore::lookup(const string& zone, const RRClass& class_code,
    boost::shared_ptr<AddressRequestCallback> callback, AddressFamily family)
{
    pair<bool, boost::shared_ptr<ZoneEntry> > zone_obj(zone_hash_->getOrAdd(HashKey(
        zone, class_code), boost::bind(newZone, &resolver_, &zone, &class_code,
        &nameserver_hash_, &nameserver_lru_)));
    if (zone_obj.first) {
        zone_lru_->add(zone_obj.second);
    } else {
        zone_lru_->touch(zone_obj.second);
    }
    zone_obj.second->addCallback(callback, family);
}

} // namespace nsas
} // namespace isc
