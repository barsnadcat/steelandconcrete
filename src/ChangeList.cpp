#include <pch.h>
#include <ChangeList.h>

#include <ServerLog.h>

ChangeList::UpdateBlockList ChangeList::mChangeList;
ChangeList::ResponseList ChangeList::mCurrentChanges;
boost::shared_mutex ChangeList::mChangeListRWL;

ChangeMsg& ChangeList::AddChangeMsg()
{
    ChangeMsg* change = NULL;
    if (!mCurrentChanges.empty() && (mCurrentChanges.back()->changes_size() < 250))
    {
        change = mCurrentChanges.back()->add_changes();
    }
    else
    {
        ResponseMsg* msg = new ResponseMsg();
        msg->set_type(RESPONSE_PART);
        change = msg->add_changes();
        mCurrentChanges.push_back(msg);
    }
    assert(change);
    return *change;
}

void ChangeList::AddMove(UnitId aUnit, TileId aPosition)
{
    ChangeMsg& change = AddChangeMsg();
    UnitMoveMsg* move = change.mutable_unitmove();
    move->set_unitid(aUnit);
    move->set_position(aPosition);
}

void ChangeList::Clear()
{
    for (UpdateBlockList::iterator i = mChangeList.begin(); i != mChangeList.end(); ++i)
    {
        ResponseList responseList = i->second;
        for (ResponseList::iterator j = responseList.begin(); j != responseList.end(); ++j)
        {
            delete *j;
        }
    }
    mChangeList.clear();
}

void ChangeList::Commit(GameTime aTime)
{
    mChangeListRWL.lock();
    mChangeList.push_back(std::make_pair(aTime, mCurrentChanges));
    mCurrentChanges.resize(0);

    if (mChangeList.size() > 200)
    {
        ResponseList& changeList = mChangeList.front().second;
        for (ResponseList::iterator i = changeList.begin(); i != changeList.end(); ++i)
        {
            delete *i;
        }
        mChangeList.pop_front();
    }
    mChangeListRWL.unlock();
}

bool ChangeBlockCmp(ChangeList::UpdateBlock i, ChangeList::UpdateBlock j)
{
    return i.first < j.first;
}

void ChangeList::Write(INetwork& aNetwork, GameTime aClientTime, int32 aUpdateLength)
{
    mChangeListRWL.lock_shared();

    GameTime maxTime = 0;
    GameTime minTime = 0;
    if (!mChangeList.empty())
    {
        maxTime = mChangeList.back().first;
        minTime = mChangeList.front().first;
    }

    UpdateBlockList::iterator i =
        std::upper_bound(mChangeList.begin(), mChangeList.end(),
                         std::make_pair(aClientTime, ResponseList()), ChangeBlockCmp);

    while (i != mChangeList.end())
    {
        ResponseList& changeList = i->second;
        maxTime = i->first;
        if (!changeList.empty())
        {
            ResponseList::const_iterator j = changeList.begin();
            while (j != changeList.end())
            {
                aNetwork.WriteMessage(**j);
                ++j;
            }
        }
        ++i;
    }

    mChangeListRWL.unlock_shared();

    ResponseMsg emptyMsg;
    emptyMsg.set_type(RESPONSE_OK);
    emptyMsg.set_time(maxTime);
	emptyMsg.set_update_length(aUpdateLength);
    aNetwork.WriteMessage(emptyMsg);

}

void ChangeList::AddCommandDone(UnitId aUnit)
{
    ChangeMsg& change = AddChangeMsg();
    CommandDoneMsg* command = change.mutable_commanddone();
    command->set_unitid(aUnit);
}

void ChangeList::AddRemove(UnitId aUnit)
{
    ChangeMsg& change = AddChangeMsg();
    RemoveMsg* command = change.mutable_remove();
    command->set_unitid(aUnit);
}
