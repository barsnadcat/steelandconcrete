#ifndef SERVERAPP_H
#define SERVERAPP_H

#include <Typedefs.h>
#include <ServerGeodesicGrid.h>
#include <ClientConnection.h>
#include <ServerUnit.h>
#include <sockio.h>
#include <task.h>
#include <Request.pb.h>
#include <ServerBuilding.h>

class ServerGame
{
public:
    typedef std::map< UnitId, ServerUnit* > ServerUnits;
    typedef std::map< BuildingId, ServerBuilding* > ServerBuildings;
    ServerGame(int aSize);
    ~ServerGame();
    void MainLoop(Ogre::String aAddress, Ogre::String aPort);
    ServerGeodesicGrid& GetGrid();
    ServerUnit& CreateUnit(ServerTile& aTile);
    ServerBuilding& CreateBuilding(ServerTile* aTile);
    void Send(socket_t& aSocket);
    GameTime GetTime() const { return mTime; }
    void SignalClientEvent() { mClientEvent->signal(); }
    void LoadCommands(const RequestMsg& commands);
protected:
private:
    void UpdateGame();
    ServerGeodesicGrid* mGrid;
    int mUnitCount;
    int mBuildingCount;
    GameTime mTime;
    GameTime mTimeStep;
    ServerUnits mUnits;
    ServerBuildings mBuildings;
    event* mClientEvent;
    mutex* mGameMutex;
};

#endif // SERVERAPP_H
