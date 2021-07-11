/*
 *
 *    Copyright (c) 2018 Google LLC.
 *    Copyright (c) 2013-2017 Nest Labs, Inc.
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

/**
 *    @file
 *      This file provides a more logical container representation of a WDM dictionary
 *      that permits both indexing against the dictionary key as well as a more logical primary
 *      key
 */

#ifndef _WEAVE_DATA_MANAGEMENT_WDM_DICTIONARY_H
#define _WEAVE_DATA_MANAGEMENT_WDM_DICTIONARY_H

#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>

#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveMessageLayer.h>
#include <Weave/Profiles/ProfileCommon.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Profiles/data-management/MessageDef.h>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/sequenced_index.hpp>

using namespace boost::multi_index;

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current) {

//
// This templated class provides a more user-friendly type when interacting with WDL maps.
// WDL maps are constrained in their keys to having 16-bit numerical keys. As such, these maps
// cannot be thought of as traditional maps that have the key being the logical/primary key used to 
// index the data in the collection. Rather, the key is merely a unique number assigned to each item,
// lacking any semantic or logical meaning. The actual logical key is embedded in the item itself, and
// be any WDL type. In this sense, WDL maps can be thought of as spare arrays.
//
// Applications will typically want to interact with the collection locally on devices more naturally as a keyed collection, 
// with the data index using the logical key. This requires them to maintain a two-map solution: 
// one to store the original data in the trait, and another to translate from the logical key to the map key.
//
// This templated class utilizes boost's multi_index_container, which permits indexing the data against multiple keys
// while still retaining efficient storage and look-up of the data. The class wraps the provided boost type and
// provides helper methods to interact with each key's sub-table. 
//
// Specializing the template requires passing in both the logical key type as well as the value type as well.
//
// Requirements on the types include:
//      Logical Key Type: Has both '<' and '==' operators implemented.
//      Value Type: Has the '==' operator implemented.
//
// The class also provides methods to compare against another instance of the collection to easily figure out the
// set of items added, removed or modified against the logical key. This allows for comparisons
//
template <class KeyT, class ValueT>
class WdmDictionary {
public:
    //
    // This is the Item that is used in the collection, housing both the 16-bit dictionary
    // key as well as the logical KeyT typed key.
    // It also contains the value as well.
    //
    struct Item {
        Item(uint16_t dictKey) { _dict_key = dictKey; _logical_key = 0; }
        Item() { _dict_key = 0; _logical_key = 0; }

        bool operator<(const Item& i1) const {
            return (_logical_key < i1._logical_key);
        }

        bool operator ==(const Item& i) const {
            return (_logical_key == i._logical_key && _dict_key == i._dict_key && _data == i._data);
        }

        ValueT _data;
        uint16_t _dict_key;
        KeyT _logical_key;
    };

    struct map_key{};
    struct logical_key{};

    typedef multi_index_container<
        Item,
        indexed_by<
            // Both indexes are of type 'ordered_unique' since it's expected there will and should
            // not be collisions in either of the key-spaces.
            ordered_unique<tag<map_key>, member<Item, uint16_t, &Item::_dict_key> >,
            ordered_unique<tag<logical_key>, member<Item, KeyT, &Item::_logical_key> >
        >
    > Container;

    typedef typename Container::template index<map_key>::type DictKeyTableType;
    typedef typename Container::template index<map_key>::type::iterator DictKeyTableTypeIterator;
    typedef typename Container::template index<logical_key>::type LogicalKeyTableType;
    typedef typename Container::template index<logical_key>::type::iterator LogicalKeyTableTypeIterator;

    DictKeyTableType& GetDictKeyTable() { return _container.template get<map_key>(); }
    LogicalKeyTableType& GetLogicalKeyTable() {  return _container.template get<logical_key>(); }

    // 
    // This is a convenience method that automatically creates an element in the collection with a particular
    // dictionary key, or accesses an existing element that already exists, and allows the caller to mutate a member
    // through a provided closure.
    //
    void ModifyItem(uint16_t dictKey, std::function<void(Item&)> func);
   
    //
    // The following methods compare against a provided container and enumerates items that have been added, deleted or modified.
    // This is specifically done against the logical key table and *not* against the dictionary keys. This is necessary since certain
    // implementations of dictionaries (like the Nest cloud service) do not provide key stability for the dictionary keys, occasionally
    // re-key'ing them while retaining the logical key + value content.
    //
    void ItemsAdded(WdmDictionary& stagedContainer, std::function<void(LogicalKeyTableTypeIterator&)> func, bool updateStore=false);
    void ItemsRemoved(WdmDictionary& stagedContainer, std::function<void(LogicalKeyTableTypeIterator&)> func, bool updateStore=false);
    void ItemsModified(WdmDictionary& stagedContainer, std::function<void(LogicalKeyTableTypeIterator&, LogicalKeyTableTypeIterator&)> func, bool updateStore=false);
    bool IsEqual(WdmDictionary &dictionary);

private: 
    Container _container;
};

template <typename KeyT, typename ValueT>
bool WdmDictionary<KeyT, ValueT>::IsEqual(WdmDictionary &dictionary)
{
    return (dictionary._container.size() == _container.size()) 
        && std::equal(_container.begin(), _container.end(), dictionary._container.begin());
}

template <typename KeyT, typename ValueT>
void WdmDictionary<KeyT, ValueT>::ModifyItem(uint16_t dictKey, std::function<void(Item &)> func)
{
    Item item = Item(dictKey);
    DictKeyTableType& mapkey_tbl = _container.template get<map_key>();
    mapkey_tbl.modify(mapkey_tbl.insert(item).first, [&func](Item &d) {
        func(d);
    });
}

template <typename KeyT, typename ValueT>
void WdmDictionary<KeyT, ValueT>::ItemsAdded(WdmDictionary& stagedContainer, std::function<void(LogicalKeyTableTypeIterator&)> func, bool updateStore)
{
    Container addedItems;
    LogicalKeyTableType &addedItemsLogicalTbl = addedItems.template get<logical_key>();

    std::set_difference(stagedContainer.GetLogicalKeyTable().begin(), stagedContainer.GetLogicalKeyTable().end(), 
                        GetLogicalKeyTable().begin(), GetLogicalKeyTable().end(),
                        std::inserter(addedItemsLogicalTbl, addedItemsLogicalTbl.begin()));

    for (auto it = addedItemsLogicalTbl.begin(); it != addedItemsLogicalTbl.end(); it++) {
        func(it);

        if (updateStore) {
            GetLogicalKeyTable().insert(*it);
        }
    }
}

template <typename KeyT, typename ValueT>
void WdmDictionary<KeyT, ValueT>::ItemsRemoved(WdmDictionary& stagedContainer, std::function<void(LogicalKeyTableTypeIterator&)> func, bool updateStore)
{
    Container removedItems;
    LogicalKeyTableType &removedItemsLogicalTbl = removedItems.template get<logical_key>();

    std::set_difference(GetLogicalKeyTable().begin(), GetLogicalKeyTable().end(),
                        stagedContainer.GetLogicalKeyTable().begin(), stagedContainer.GetLogicalKeyTable().end(), 
                        std::inserter(removedItemsLogicalTbl, removedItemsLogicalTbl.begin()));

    for (auto it = removedItemsLogicalTbl.begin(); it != removedItemsLogicalTbl.end(); it++) {
        func(it);

        if (updateStore) {
            GetLogicalKeyTable().erase(it->_logical_key);
        }
    }
}

template <typename KeyT, typename ValueT>
void WdmDictionary<KeyT, ValueT>::ItemsModified(WdmDictionary& stagedContainer, std::function<void(LogicalKeyTableTypeIterator&, LogicalKeyTableTypeIterator&)> func, bool updateStore)
{
    Container modifiedItems;
    LogicalKeyTableType &modifiedItemsTbl = modifiedItems.template get<logical_key>();

    std::set_intersection(stagedContainer.GetLogicalKeyTable().begin(), stagedContainer.GetLogicalKeyTable().end(), 
                        GetLogicalKeyTable().begin(), GetLogicalKeyTable().end(),
                        std::inserter(modifiedItemsTbl, modifiedItemsTbl.begin()));

    for (auto it = modifiedItemsTbl.begin(); it != modifiedItemsTbl.end(); it++) {
        auto it1 = GetLogicalKeyTable().find(it->_logical_key);
        auto it2 = stagedContainer.GetLogicalKeyTable().find(it->_logical_key);

        if (!(it1->_data == it2->_data)) {
            func(it1, it2);
        }

        if (updateStore) {
            GetLogicalKeyTable().modify(it1, [it2](auto &d) {
                d = *it2;
            });
        }
    }
}

}; // namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current)
}; // namespace Profiles
}; // namespace Weave
}; // namespace nl

#endif
