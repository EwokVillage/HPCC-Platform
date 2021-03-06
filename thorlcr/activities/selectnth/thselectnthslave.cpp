/*##############################################################################

    Copyright (C) 2011 HPCC Systems.

    All rights reserved. This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
############################################################################## */

#include "thselectnthslave.ipp"
#include "thactivityutil.ipp"
#include "thbufdef.hpp"

class CSelectNthSlaveActivity : public CSlaveActivity, public CThorDataLink, implements ISmartBufferNotify
{
    bool first, isLocal, seenNth;
    rowcount_t lookaheadN, N, startN;
    bool createDefaultIfFail;
    Owned<IThorDataLink> input;
    IHThorSelectNArg *helper;
    SpinLock spin;

    void initN()
    {
        // in n<0 before start of dataset (so output blank row)
        // n==0 means output nothing and return 0 (get returns eos)
        if (isLocal || (1 == container.queryJob().queryMyRank()))
        {
            N = (rowcount_t)helper->getRowToSelect();
            if (0==N)
                N=RCMAX;
        }
        else
        {
            CMessageBuffer msg;
            if (!receiveMsg(msg, container.queryJob().queryMyRank()-1, mpTag))
                return;
            msg.read(N);
            msg.read(seenNth);
            if (!seenNth && container.queryJob().queryMyRank() == container.queryJob().querySlaves())
                createDefaultIfFail = true;
        }
        startN = N;
        ActPrintLog("SELECTNTH: Selecting row %"I64F"d", N);
    }
    void sendN()
    {
        if (isLocal || container.queryJob().queryMyRank() == container.queryJob().querySlaves()) // don't send if local or last node
            return;
        CMessageBuffer msg;
        msg.append(N);
        msg.append(seenNth); // used by last node to trigger fail if not seen
        container.queryJob().queryJobComm().send(msg, container.queryJob().queryMyRank()+1, mpTag);
    }

public:
    IMPLEMENT_IINTERFACE_USING(CSimpleInterface);

    CSelectNthSlaveActivity(CGraphElementBase *_container, bool _isLocal) : CSlaveActivity(_container), CThorDataLink(this)
    {
        isLocal = _isLocal;
        createDefaultIfFail = isLocal || (container.queryJob().queryMyRank() == container.queryJob().querySlaves());
    }
    ~CSelectNthSlaveActivity()
    {
    }

// IThorSlaveActivity overloaded methods
    virtual void init(MemoryBuffer & data, MemoryBuffer &slaveData)
    {
        if (!container.queryLocalOrGrouped())
            mpTag = container.queryJob().deserializeMPTag(data);
        appendOutputLinked(this);
        helper = static_cast <IHThorSelectNArg *> (queryHelper());
    }
    virtual void start()
    {
        ActivityTimer s(totalCycles, timeActivities, NULL);

        lookaheadN = RCMAX;
        startN = 0; // set by initN()
        rowcount_t rowN = (rowcount_t)helper->getRowToSelect();
        if (!isLocal && rowN)
            input.setown(createDataLinkSmartBuffer(this, inputs.item(0), SELECTN_SMART_BUFFER_SIZE, isSmartBufferSpillNeeded(this), false, rowN, this, false, &container.queryJob().queryIDiskUsage()));
        else
            input.set(inputs.item(0));
        try
        {
            startInput(input);
        }
        catch (IException *e)
        {
            ActPrintLog(e, NULL);
            N=0;
            sendN();
            throw;
        }
        dataLinkStart("SELECTNTH", container.queryId());

        seenNth = false;
        if (0==helper->getRowToSelect())
        {
            ThorDataLinkMetaInfo info;
            inputs.item(0)->getMetaInfo(info);
            StringBuffer meta;
            meta.appendf("META(totalRowsMin=%"I64F"d,totalRowsMax=%"I64F"d,rowsOutput=%"RCPF"d,spilled=%"I64F"d,byteTotal=%"I64F"d)",
                info.totalRowsMin,info.totalRowsMax,info.rowsOutput,info.spilled,info.byteTotal);
#if 0                 
            Owned<IThorException> e = MakeActivityWarning(this, -1, "%s", meta.str());
            fireException(e);
#else
            ActPrintLog("%s",meta.str());
#endif
        }
        first = true;
    }
    virtual void stop()
    {
        stopInput(input);
        dataLinkStop();
    }
    virtual void abort()
    {
        CSlaveActivity::abort();
        if (1 != container.queryJob().queryMyRank())
            cancelReceiveMsg(RANK_ALL, mpTag);
    }
    CATCH_NEXTROW()
    {
        ActivityTimer t(totalCycles, timeActivities, NULL);
        OwnedConstThorRow ret;
        Owned<IException> exception;
        if (first) // only return 1!
        {
            try
            {
                first = false;
                initN();
                if (RCMAX==N) // indicates before start of dataset e.g. ds[0]
                {
                    RtlDynamicRowBuilder row(queryRowAllocator());
                    size32_t sz = helper->createDefault(row);
                    ret.setown(row.finalizeRowClear(sz));
                    N = 0; // return that processed all
                }
                else if (N)
                {
                    while (!abortSoon)
                    {
                        ret.setown(input->ungroupedNextRow());
                        if (!ret)
                            break;
                        N--;
                        {
                            SpinBlock block(spin);
                            if (lookaheadN<startN) // will not reach N==0, so don't bother continuing to read
                            {
                                N = startN-lookaheadN;
                                ret.clear();
                                break;
                            }
                        }
                        if (0==N)
                            break;
                    }
                    if ((N!=0)&&createDefaultIfFail)
                    {
                        N = 0; // return that processed all (i.e. none left)
                        RtlDynamicRowBuilder row(queryRowAllocator());
                        size32_t sz = helper->createDefault(row);
                        ret.setown(row.finalizeRowClear(sz));
                    }
                }
                if (startN && 0 == N)
                    seenNth = true;
            }
            catch (IException *e)
            {
                N=0;
                exception.setown(e);
            }
            sendN();
            if (exception.get())
                throw exception.getClear();
        }
        if (ret) 
            dataLinkIncrement();
        return ret.getClear();
    }
    virtual bool isGrouped() { return false; }
    void getMetaInfo(ThorDataLinkMetaInfo &info)
    {
        initMetaInfo(info);
        info.isSequential = true; 
        info.canReduceNumRows = true; // not sure what selectNth is doing
        calcMetaInfoSize(info,inputs.item(0));
    }
// ISmartBufferNotify methods used for global selectn only
    virtual void onInputStarted(IException *) { } // not needed
    virtual bool startAsync() { return false; }
    virtual void onInputFinished(rowcount_t count)
    {
        SpinBlock b(spin);
        lookaheadN = count;
    }
};


CActivityBase *createLocalSelectNthSlave(CGraphElementBase *container)
{
    return new CSelectNthSlaveActivity(container, true);
}

CActivityBase *createSelectNthSlave(CGraphElementBase *container)
{
    return new CSelectNthSlaveActivity(container, false);
}

