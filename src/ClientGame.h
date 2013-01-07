#ifndef CLIENTGAME_H
#define CLIENTGAME_H

#include <ClientUnit.h>
#include <ClientGeodesicGrid.h>
#include <Payload.pb.h>
#include <Typedefs.h>
#include <ServerProxy.h>
#include <SyncTimer.h>
#include <CEGUI.h>
#include <BirdCamera.h>

class ClientApp;

class ClientGame
{
public:
    typedef std::map< UnitId, ClientUnit* > ClientUnits;
    ClientGame(ServerProxyPtr aServerProxy, TileId aLandingTileId, int32 aGridSize);
    virtual ~ClientGame(); // Для QuicGUI
    void UpdateTileUnderCursor(Ogre::Ray aRay);
    void Update(unsigned long aFrameTime, const Ogre::RenderTarget::FrameStats& aStats);

    void mouseMoved(const OIS::MouseEvent& arg);
    void mousePressed(const OIS::MouseEvent& arg, OIS::MouseButtonID id);
    void mouseReleased(const OIS::MouseEvent& arg, OIS::MouseButtonID id);
    void keyPressed(const OIS::KeyEvent& arg);
    void keyReleased(const OIS::KeyEvent& arg);
    void SubscribeToGUI();

    static ClientUnit* GetUnit(UnitId aUnitId);
    static void EraseUnitId(UnitId aUnitId);
private:
    bool OnExit(const CEGUI::EventArgs& args);
    void OnEscape();
    void OnAct();
    void OnPayloadMsg(ConstPayloadPtr aPayloadMsg);
    void RequestUpdate();
    void LoadEvents(ConstPayloadPtr aPayloadMsg);
private:
    static ClientUnits mUnits;
    BirdCamera* mBirdCamera;
    ClientGeodesicGrid::Tiles mTiles;
    ClientGridNode* mTileUnderCursor;
    Ogre::SceneNode* mSelectionMarker;
    Ogre::SceneNode* mTargetMarker;
    GameTime mTime;
    SyncTimer mSyncTimer;
    int32 mServerUpdateLength;
    ServerProxyPtr mServerProxy;
};

#endif // CLIENTGAME_H
