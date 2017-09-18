/*
 *
 *    Copyright (c) 2016-2017 Nest Labs, Inc.
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
 *      This file defines notification engine for Weave
 *      Data Management (WDM) profile.
 *
 */


#ifndef _WEAVE_DATA_MANAGEMENT_NOTIFICATION_ENGINE_CURRENT_H
#define _WEAVE_DATA_MANAGEMENT_NOTIFICATION_ENGINE_CURRENT_H

#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>
#include <Weave/Core/WeaveCore.h>
#include <Weave/Profiles/data-management/SubscriptionHandler.h>
#include <Weave/Profiles/data-management/TraitData.h>
#include <Weave/Profiles/data-management/TraitCatalog.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current) {

/*
 *  @class NotificationEngine
 *
 *  @brief The notification engine is responsible for generating notifies to subscriber. It is able to find the intersection between
 *         the path interest set of each subscriber with what has changed in the publisher data store and generate tailored notifies for
 *         each subscriber.
 *
 *         To achieve this, the engine tracks data-changes (i.e data dirtiness) at a couple of different levels:
 *              - Per subscriber, per trait instance dirtiness: Every subscriber tracks trait-changes at a per-instance granularity.
 *                Anytime a data source makes known that a property handle within has changed, the NE will iterate over every subscriber that has subscribed
 *                to that trait instance and mark the fact that that instance is now dirty.
 *
 *              - Granular per trait instance, per property handle dirtiness: If selected through compile-time options by the user, the engine will
 *                mark dirtiness down to the property handle. This allows it to generate compact notifies that convey as succinctly as possible the data
 *                that has changed. This will be described in more detail in the solvers section.
 *
 *         At its core, it iterates over every subscription, then every dirty instance within that subscription and tries to gather and pack as much relevant data
 *         as possible into a notify message before sending that to the subscriber. It continues to do so until it has no more work to do. This could be due to a couple of reasons:
 *              - Notifies are in flight to the subscriber(s)
 *              - We have exceeded the maximum number of notifies that can be flight across all subscribers.
 *              - We have no more space in the packet to stuff in more data.
 *              - We have no more dirty data to process for a particular set of subscriptions.
 *
 *         Once it surmises there is no more work to be done, it returns. If all work for a subscription has been completed, it will invoke a method in the SubscriptionHandler to finish
 *         processing that subscription (which might involve sending out subscription responses).
 *
 *         During subscription establishment, the NE works slightly differently than at other times - it will retrieve *all* the data for a particular trait. There-after, it will only retrieve
 *         new, changed data.
 *
 *         Some notable features:
 *              - Subscription fairness: The engine round-robins over all subscriptions and will always resume its work loop at the last subscription it was
 *              trying to process to ensure all subscriptions are handled with equal priority.
 *
 *              - Trait instance fairness: Within a subscription, the engine also rounds robins over all trait instances and will resume its work loop at the last trait instance that was being
 *              processed *for that subscription*. This ensures trait instances that have a high rate of change don't starve out others.
 *
 *              - Inter-trait chunking across multiple notifies: The engine supports splitting trait data over multiple notifies. It will however only do this split at the trait instance granularity.
 *              It cannot chunk up data within a trait.
 *
 *              - Graceful degradation due to resource shortages: If it runs out space in the dirty stores, the engine will degrade gracefully by generating sub-optimal notify messages that have more
 *              data in them while still being protocol correct.
 *
 */
class NotificationEngine
{
public:
    /**
     * Initializes the engine. Should only be called once.
     *
     * @retval #WEAVE_NO_ERROR On success.
     * @retval other           Was unable to retrieve data and write it into the writer.
     */
    WEAVE_ERROR Init(void);

    /**
     * Main work-horse function that executes the run-loop.
     *
     * @retval void
     */
    void Run(void);

    /**
     * Marks a handle associated with a data source as being dirty.
     *
     * @retval #WEAVE_NO_ERROR On success.
     * @retval other           Was unable to retrieve data and write it into the writer.
     */
    WEAVE_ERROR SetDirty(TraitDataSource *aDataSource, PropertyPathHandle aPropertyHandle);

    WEAVE_ERROR DeleteKey(TraitDataSource *aDataSource, PropertyPathHandle aPropertyHandle);

    enum NotifyRequestBuilderState
    {
        kNotifyRequestBuilder_Idle = 0,  //< The request has not been opened or has been closed and finalized
        kNotifyRequestBuilder_Ready, //< The request has been initialized and is ready for any optional toplevel elements
        kNotifyRequestBuilder_BuildDataList, //< The request is building the DataList portion of the structure
        kNotifyRequestBuilder_BuildEventList //< The request is building the EventList portion of the structure
    };

    /**
     *  @class NotifyRequestBuilder
     *
     *  @brief This provides a helper class to compose notifies and abstract away the construction and structure of the message from its consumers. This is a more compact
     *         version of a similar class provided in MessageDef.cpp that aims to be sensitive to the flash and ram needs of the device.
     */
     class NotifyRequestBuilder
    {
    public:
        /**
         * Initializes the builder. Should only be called once.
         *
         * @retval #WEAVE_NO_ERROR On success.
         * @retval other           Was unable to initialize the builder.
         */
        WEAVE_ERROR Init(PacketBuffer**aBuf, TLV::TLVWriter *aWriter, SubscriptionHandler *aSubHandler);

        /**
         * Start the construction of the notify.
         *
         * @retval #WEAVE_NO_ERROR On success.
         * @retval #WEAVE_ERROR_INCORRECT_STATE If the request is not at the toplevel of the buffer.
         * @retval other           Unable to construct the end of the notify.
         */

        WEAVE_ERROR StartNotifyRequest();

        /**
         * End the construction of the notify.
         *
         * @retval #WEAVE_NO_ERROR On success.
         * @retval #WEAVE_ERROR_INCORRECT_STATE If the request is not at Notify container.
         * @retval other           Unable to construct the end of the notify.
         */
        WEAVE_ERROR EndNotifyRequest();

        /**
         * Starts the construction of the data list array.
         *
         * @retval #WEAVE_NO_ERROR On success.
         * @retval #WEAVE_ERROR_INCORRECT_STATE If the request is not at the Notify container.
         * @retval other           Unable to construct the beginning of the data list.
         */
        WEAVE_ERROR StartDataList(void);

        /**
         * End the construction of the data list array.
         *
         * @retval #WEAVE_NO_ERROR On success.
         * @retval #WEAVE_ERROR_INCORRECT_STATE If the request is not at the DataList container.
         * @retval other           Unable to construct the end of the data list.
         */
        WEAVE_ERROR EndDataList();

        /**
         * Starts the construction of the event list.
         *
         * @retval #WEAVE_NO_ERROR On success.
         * @retval #WEAVE_ERROR_INCORRECT_STATE If the request is not at the Notify container.
         * @retval other           Unable to construct the beginning of the data list.
         */
        WEAVE_ERROR StartEventList();

        /**
         * End the construction of the event list.
         *
         * @retval #WEAVE_NO_ERROR On success.
         * @retval #WEAVE_ERROR_INCORRECT_STATE If the request is not at the EventList container.
         * @retval other           Unable to construct the end of the data list.
         */
        WEAVE_ERROR EndEventList();

        /**
         * Given a trait path, write out the data element associated
         * with that path. The caller can also optionally pass in a
         * handle set allows for leveraging the merge operation with a
         * narrower set of immediate child nodes of the parent
         * property path handle.
         *
         * @retval #WEAVE_NO_ERROR On success.
         * @retval other           Unable to retrieve and write the data element.
         */
        WEAVE_ERROR WriteDataElement(TraitDataHandle aTraitDataHandle, PropertyPathHandle aPropertyPathHandle, SchemaVersion aSchemaVersion, PropertyPathHandle *aMergeDataHandleSet, uint32_t aNumMergeDataHandles, PropertyPathHandle *aDeleteHandleSet, uint32_t aNumDeleteHandles);

        /**
         * Checkpoint the request state into a TLVWriter
         *
         * @param aWriter[out] A writer to checkpoint the state of the TLV writer into.
         *
         * @retval #WEAVE_NO_ERROR On success.
         */
        WEAVE_ERROR Checkpoint(TLV::TLVWriter &aPoint);

        /**
         * Rollback the request state into the checkpointed TLVWriter
         *
         * @param aWriter[in] A writer to that captured the state at some point in the past
         *
         * @retval #WEAVE_NO_ERROR On success.
         */
        WEAVE_ERROR Rollback(TLV::TLVWriter &aPoint);


        TLV::TLVWriter *GetWriter(void) { return mWriter; }

        /**
         * The main state transition function. The function takes the
         * desired state (i.e., the phase of the notify request builder
         * that we would like to reach), and transitions the request
         * into that state. If the desired state is the same as the
         * current state, the function does nothing. Otherwise, an
         * PacketBuffer is allocated (if needed); the function first
         * transitions the request into the toplevel notify request
         * (either opening the notify request TLV structure, or
         * closing the current TLV data container as needed), and then
         * transitions the Notify request either by opening the
         * appropriate TLV data container or by closing the
         * overarching Notify request.
         *
         * @param aDesiredState  The desired state the request should transition into
         *
         * @retval #WEAVE_NO_ERROR On success.
         * @retval #WEAVE_ERROR_NO_MEMORY Could not transition into the state because of insufficient memory.
         * @retval #WEAVE_ERROR_INCORRECT_STATE Corruption of the internal state machine.
         * @retval other When the state machine could not record the state in its buffer, likely indicates a design flaw rather than a runtime issue.
         */
        WEAVE_ERROR MoveToState(NotifyRequestBuilderState aDesiredState);

    private:
        TLV::TLVWriter *mWriter;
        NotifyRequestBuilderState mState;
        PacketBuffer **mBuf;
        SubscriptionHandler *mSub;
    };

    /*
     * GRAPH SOLVERS
     *
     * To generate notifies, NE institutes a notion of a solver to help crunch the tree-based representation of the schema along with the dirty handle information.
     * This allows for consolidation of the logic to processing the schema for notify generation to a localized body of functions.
     *
     * To accommodate the varying capabilities of Weave-enabled devices, a number of different solvers have been provided that range in their capabilities and their
     * constraints.
     *
     * These solvers can be chosen at compile time to reduce flash usage.
     *
     * ---- To facilitate this, all the public methods in the solvers have to have the same signature! If you decide to add a new solver, please ensure this criteria is met! ---
     *
     */

    /*
     *  @class BasicGraphSolver
     *
     *  @brief This is a coarse, basic solver that will retrieve the entire contents of a trait instance from root. The solver trades of computational complexity and
     *         reduced storage requirements with inefficiency in the data transmitted over the wire. This is rarely useful for most applications given the sheer in-efficiency of data transmitted
     *         over the wire, especially for traits with lots of key/value pairs. It is however useful for bring-up or for debugging issues with the other solvers.
     *
     *         Constraints: It only supports subscriptions to root and nothing deeper.
     */
    class BasicGraphSolver
    {
    public:
        static bool IsPropertyPathSupported(PropertyPathHandle aHandle);
        WEAVE_ERROR RetrieveTraitInstanceData(NotifyRequestBuilder *aBuilder, TraitDataHandle aTraitDataHandle, SchemaVersion aSchemaVersion, bool aRetrieveAll);
        static WEAVE_ERROR SetDirty(TraitDataHandle aTraitDataHandle, PropertyPathHandle aPropertyHandle);
        WEAVE_ERROR ClearDirty(void);
    };

    /*
     *  @class IntermediateGraphSolver
     *
     *  @brief This solver is able to generate compact notifies that try to only contain the modified bits of data. This leverages a finitely sized, global dirty store
     *         that houses granular dirty information per property handle per trait instance. When a notify is to be generated, the solver attempts to find the LCA (lowest-common-ancestor)
     *         of all the dirty nodes in the tree and generates a data-element against that path. In addition, it exploits the merge semantics of WDM to only include child trees of that LCA
     *         that contain dirty elements. This is pretty efficient given the reasonably flat, shallow structure of our IDLs.
     *
     *         If it is unable to store anymore dirty items in the granular store, it will degrade to marking the entire trait instance as dirty.
     *         In addition, if it runs out of space in the merge handle set, it will degrade to including all child trees of the LCA'ed node.
     *
     */
    class IntermediateGraphSolver
    {
    public:
        static bool IsPropertyPathSupported(PropertyPathHandle aHandle);
        WEAVE_ERROR RetrieveTraitInstanceData(NotifyRequestBuilder *aBuilder, TraitDataHandle aTraitDataHandle, SchemaVersion aSchemaVersion, bool aRetrieveAll);
        WEAVE_ERROR SetDirty(TraitDataHandle aTraitDataHandle, PropertyPathHandle aPropertyHandle);

#if TDM_ENABLE_PUBLISHER_DICTIONARY_SUPPORT
        WEAVE_ERROR DeleteKey(TraitDataHandle aTraitDataHandle, PropertyPathHandle aPropertyHandle);
#endif

        WEAVE_ERROR ClearDirty(void);

        struct Store
        {
        public:
            Store();
            bool AddItem(TraitPath aItem);
            void RemoveItem(TraitDataHandle aDataHandle);
            void RemoveItemAt(uint32_t aIndex);
            bool IsPresent(TraitPath aItem);
            bool IsFull() { return mNumItems >= WDM_PUBLISHER_MAX_ITEMS_IN_TRAIT_DIRTY_STORE; }
            uint32_t GetNumItems() { return mNumItems; }
            uint32_t GetStoreSize() { return WDM_PUBLISHER_MAX_ITEMS_IN_TRAIT_DIRTY_STORE; }
            void Clear();

            TraitPath mStore[WDM_PUBLISHER_MAX_ITEMS_IN_TRAIT_DIRTY_STORE];
            bool mValidFlags[WDM_PUBLISHER_MAX_ITEMS_IN_TRAIT_DIRTY_STORE];
            uint32_t mNumItems;
        };

    private:
        static void ClearTraitInstanceDirty(void *aDataSource, TraitDataHandle aDataHandle, void *aContext);
        PropertyPathHandle GetNextCandidateHandle(uint32_t &aChangeStoreCursor, TraitDataHandle aTargetDataHandle, bool &aCandidateHandleIsDelete);

        Store mDirtyStore;

#if TDM_ENABLE_PUBLISHER_DICTIONARY_SUPPORT
        Store mDeleteStore;
#endif
    };

private:
    friend class SubscriptionHandler;
    friend class TestTdm;
    friend class TestWdm;

    /**
     * Should be invoked when the device receives a NotifyConfirm, or when the Notify request times out.
     * This allows the engine to do some clean-up.
     *
     * @retval #WEAVE_NO_ERROR On success.
     * @retval other           Was unable to retrieve data and write it into the writer.
     */
    void OnNotifyConfirm(SubscriptionHandler *aSubHandler, bool aNotifyDelivered);

    WEAVE_ERROR BuildSingleNotifyRequestDataList(SubscriptionHandler *aSubHandler, NotifyRequestBuilder &aNotifyRequest, bool &isSubscriptionClean, bool &aNeWriteInProgress);
    WEAVE_ERROR BuildSingleNotifyRequestEventList(SubscriptionHandler *aSubHandler, NotifyRequestBuilder &aNotifyRequest, bool &isSubscriptionClean, bool &aNeWriteInProgress);
    WEAVE_ERROR BuildSingleNotifyRequest(SubscriptionHandler *aSubHandler, bool &aSubscriptionHandled, bool &isSubscriptionClean);

    WEAVE_ERROR RetrieveTraitInstanceData(SubscriptionHandler *aSubHandler, SubscriptionHandler::TraitInstanceInfo *aTraitInfo, NotifyRequestBuilder *aBuilder, bool *aPacketFull);
    WEAVE_ERROR SendNotify(PacketBuffer *aBuf, SubscriptionHandler *aSubHandler);

    WEAVE_ERROR SendNotifyRequest();

    uint32_t mCurSubscriptionHandlerIdx;
    uint32_t mCurTraitInstanceIdx;
    uint32_t mNumNotifiesInFlight;
    nl::Weave::TLV::TLVType mOuterContainerType;
    WEAVE_CONFIG_WDM_PUBLISHER_GRAPH_SOLVER mGraphSolver;
};

}; // WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current)
}; // Profiles
}; // Weave
}; // nl

#endif // _WEAVE_DATA_MANAGEMENT_NOTIFICATION_ENGINE_CURRENT_H
