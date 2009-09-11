#include <pch.h>
#include <ServerGame.h>

#include <Network.h>
#include <Unit.pb.h>
#include <ServerLog.h>
#include <ConnectionManager.h>
#include <ChangeList.h>

ServerGame::ServerGame(int aSize): mGrid(NULL), mUnitCount(0), mTime(30), mTimeStep(30)
{
    task::initialize(task::normal_stack);
    mClientEvent = new event();
    mGameMutex = new mutex();
    mGrid = new ServerGeodesicGrid(aSize);
    GetLog() << "Size " << aSize << " Tile count " << mGrid->GetTileCount();
    for (size_t i = 0; i < 15; ++i)
    {
        CreateUnit(mGrid->GetTile(rand() % mGrid->GetTileCount()));
    }
}

ServerGame::~ServerGame()
{
    delete mGrid;
    delete mClientEvent;
    delete mGameMutex;
}

ServerGeodesicGrid& ServerGame::GetGrid()
{
    return *mGrid;
}

void ServerGame::MainLoop(Ogre::String aAddress, Ogre::String aPort)
{
    Ogre::String connection = aAddress + ":" + aPort;
    GetLog() << "Connecting to " << connection;
    socket_t* gate = socket_t::create_global(connection.c_str());
    if (gate->is_ok())
    {
        ConnectionManager manager(*gate, *this);
        while (true)
        {
            mClientEvent->wait();
            mClientEvent->reset();
            if (manager.IsAllClientsReady())
            {
                UpdateGame();
            }
        }
    }
    GetLog() << "Game over";
}

ServerUnit& ServerGame::CreateUnit(ServerTile& aTile)
{
    ServerUnit* unit = new ServerUnit(aTile, ++mUnitCount);
    mUnits.insert(std::make_pair(unit->GetUnitId(), unit));
    return *unit;
}

void ServerGame::Send(socket_t& aSocket) const
{
    mGrid->Send(aSocket);

    UnitCountMsg count;
    count.set_count(mUnits.size());
    count.set_time(mTime);
    WriteMessage(aSocket, count);
    ServerUnits::const_iterator i = mUnits.begin();
    GetLog() << "Unit count send";

    for (;i != mUnits.end(); ++i)
    {
        UnitMsg unit;
        unit.set_tag(i->first);
        unit.set_tile(i->second->GetPosition().GetTileId());
        WriteMessage(aSocket, unit);
    }
    GetLog() << "All units send";
}

void ServerGame::UpdateGame()
{
    mGameMutex->enter();
    GetLog() << "Update Game!";
    ChangeList::Clear();
    ServerUnits::iterator i = mUnits.begin();
    mTime += mTimeStep;
    for (; i != mUnits.end(); ++i)
    {
        ServerUnit& unit = *i->second;
        unit.ExecuteCommand();
    }

    GetLog() << "Time: " << mTime;
    mGameMutex->leave();
}


void ServerGame::LoadCommands(const RequestMsg& commands)
{
    mGameMutex->enter();
    for (int i = 0; i < commands.commands_size(); ++i)
    {
        const CommandMsg& command = commands.commands(i);
        if (command.has_commandmove())
        {
            const CommandMoveMsg& move = command.commandmove();
            mUnits[move.unitid()]->SetCommand(mGrid->GetTile(move.position()));
        }
    }
    mGameMutex->leave();
}
