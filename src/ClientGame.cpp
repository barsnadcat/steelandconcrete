#include <pch.h>
#include <ClientGame.h>
#include <ClientApp.h>
#include <Unit.pb.h>
#include <Network.h>
#include <ClientLog.h>
#include <Request.pb.h>
#include <Response.pb.h>
#include <ChangeList.pb.h>
#include <ClientApp.h>

ClientGame::ClientGame(Network* aNetwork):
    mGrid(NULL),
    mTileUnderCursor(NULL),
    mSelectedUnit(NULL),
    mViewPortWidget(
        mIngameSheet.GetSelectedWidth(),
        mIngameSheet.GetSelectedHeight(),
        "RttTexture"
    ),
    mTime(0),
    mSyncTimer(2000),
    mNetwork(aNetwork)
{
    mIngameSheet.SetSelectedName(mViewPortWidget.GetName());
    mLoadingSheet.Activate();
    mGrid = new ClientGeodesicGrid(*mNetwork, mLoadingSheet);

    UnitCountMsg unitCount;
    mNetwork->ReadMessage(unitCount);
    GetLog() << "Recived unit count " << unitCount.ShortDebugString();
    mTime = unitCount.time();

    for (size_t i = 0; i < unitCount.count(); ++i)
    {
        UnitMsg unit;
        mNetwork->ReadMessage(unit);
        mUnits.insert(std::make_pair(
                          unit.tag(),
                          new ClientUnit(mGrid->GetTile(unit.tile()), unit)
                      ));
    }
    mLoadingSheet.SetProgress(60);
    GetLog() << "Recived all units";
    mAvatar = mUnits[unitCount.avatar()];
    ClientApp::GetCamera().Goto(mAvatar->GetPosition().GetPosition());


    // Planet
    Ogre::StaticGeometry* staticPlanet = mGrid->ConstructStaticGeometry();
    mLoadingSheet.SetProgress(90);
    assert(staticPlanet);
    //mSceneMgr->getRootSceneNode()->createChildSceneNode()->attachObject(mApp.GetPlanet()->ConstructDebugMesh());

    // Units
    CreateUnitEntities();
    mLoadingSheet.SetProgress(100);

    // Create a light
    Ogre::Light* myLight = ClientApp::GetSceneMgr().createLight("Light0");
    myLight->setType(Ogre::Light::LT_DIRECTIONAL);
    myLight->setPosition(50, 0, 0);
    myLight->setDirection(-1, 0, 0);
    myLight->setDiffuseColour(1, 1, 1);
    myLight->setSpecularColour(1, 1, 1);

    mTileUnderCursor = &mGrid->GetTile(0);
    mSelectionMarker = ClientApp::GetSceneMgr().getRootSceneNode()->createChildSceneNode();
    mSelectionMarker->setScale(Ogre::Vector3(0.01));
    mSelectionMarker->attachObject(ClientApp::GetSceneMgr().createEntity("Marker", Ogre::SceneManager::PT_SPHERE));

    mTargetMarker = ClientApp::GetSceneMgr().getRootSceneNode()->createChildSceneNode();
    mTargetMarker->attachObject(ClientApp::GetSceneMgr().createEntity("Target", "TargetMarker.mesh"));
    mTargetMarker->setVisible(false);

    QuickGUI::EventHandlerManager::getSingleton().registerEventHandler("OnExit", &ClientGame::OnExit, this);
    QuickGUI::EventHandlerManager::getSingleton().registerEventHandler("OnTurn", &ClientGame::OnTurn, this);

    mIngameSheet.SetTime(mTime);
    mIngameSheet.Activate();
}

ClientGame::~ClientGame()
{
    delete mGrid;

    std::map< UnitId, ClientUnit* >::iterator i = mUnits.begin();
    for (; i != mUnits.end(); ++i)
        delete i->second;
    mUnits.clear();

    ClientApp::GetSceneMgr().clearScene();
    delete mNetwork;
}

void ClientGame::CreateUnitEntities() const
{
    std::map< int, ClientUnit* >::const_iterator i = mUnits.begin();
    for (; i != mUnits.end(); ++i)
    {
        i->second->CreateEntity();
    }
}

void ClientGame::UpdateTileUnderCursor(Ogre::Ray& aRay)
{
    Ogre::Sphere sphere(Ogre::Vector3::ZERO, 1.0f);
    std::pair<bool, Ogre::Real> res = aRay.intersects(sphere);
    if (res.first)
    {
        Ogre::Vector3 position(aRay.getPoint(res.second));
        mTileUnderCursor = mTileUnderCursor->GetTileAtPosition(position);
        mSelectionMarker->getParent()->removeChild(mSelectionMarker);
        mTileUnderCursor->GetNode().addChild(mSelectionMarker);
    }
    mSelectionMarker->setVisible(res.first);
}

void ClientGame::Select()
{
    assert(mTileUnderCursor && "Тайл под курсором должен быть!");
    mSelectedUnit = mTileUnderCursor->GetUnit();
    if (mSelectedUnit)
    {
        ClientTile* tile = mSelectedUnit->GetTarget();
        if (tile)
        {
            mTargetMarker->getParent()->removeChild(mTargetMarker);
            tile->GetNode().addChild(mTargetMarker);
            mTargetMarker->setVisible(true);
        }
        else
        {
            mTargetMarker->setVisible(false);
        }
    }
    else
    {
        mTargetMarker->setVisible(false);
    }
}

void ClientGame::Act()
{
    assert(mTileUnderCursor && "Тайл под курсором должен быть!");
    if (mSelectedUnit)
    {
        mSelectedUnit->SetTarget(mTileUnderCursor);
        mTargetMarker->getParent()->removeChild(mTargetMarker);
        mTileUnderCursor->GetNode().addChild(mTargetMarker);
        mTargetMarker->setVisible(true);
    }
}

void ClientGame::OnExit(const QuickGUI::EventArgs& args)
{
    ClientApp::Quit();
    GetLog() << "OnExit";
}

void ClientGame::OnTurn(const QuickGUI::EventArgs& args)
{
}

ClientUnit& ClientGame::GetUnit(UnitId aUnitId)
{
    ClientUnits::iterator i = mUnits.find(aUnitId);
    if(mUnits.end() != i)
    {
        return *(i->second);
    }
    else
    {
        throw std::out_of_range("No such unit " + Ogre::StringConverter::toString(aUnitId));
    }
}

void ClientGame::LoadEvents(const ResponseMsg& changes)
{
    for (int i = 0; i < changes.changes_size(); ++i)
    {
        const ChangeMsg& change = changes.changes(i);
        if (change.has_unitmove())
        {
            const UnitMoveMsg& move = change.unitmove();
            GetUnit(move.unitid()).SetPosition(mGrid->GetTile(move.position()));
        }
        else if (change.has_commanddone())
        {
            const CommandDoneMsg& command = change.commanddone();
            GetUnit(command.unitid()).SetTarget(NULL);
        }
        else if (change.has_remove())
        {
            const RemoveMsg& command = change.remove();
            ClientUnits::iterator i = mUnits.find(command.unitid());
            if (mUnits.end() == i)
            {
                throw std::out_of_range("Server requested removal of non existing unit " + command.ShortDebugString());
            }
            ClientUnit* unit = i->second;
            if (mSelectedUnit == unit)
            {
                mSelectedUnit = NULL;
            }
            delete unit;
            mUnits.erase(i);
        }
    }
}

void ClientGame::Update(unsigned long aFrameTime, const Ogre::RenderTarget::FrameStats& aStats)
{
    mIngameSheet.UpdateStats(aStats);
    mIngameSheet.SetSelectedVisible(mSelectedUnit != NULL);
    mViewPortWidget.SetUnit(mSelectedUnit);

    if (mSyncTimer.IsTime())
    {
        RequestMsg req;
        req.set_type(REQUEST_COMMANDS);
        req.set_time(mTime);
        req.set_last(true);

        std::map< UnitId, ClientUnit* >::iterator i = mUnits.begin();
        for (; i != mUnits.end(); ++i)
        {
            ClientUnit* unit = i->second;
            if (unit->GetTarget())
            {
                CommandMsg* command = req.add_commands();
                CommandMoveMsg* move = command->mutable_commandmove();
                move->set_unitid(unit->GetUnitId());
                move->set_position(unit->GetTarget()->GetTileId());
            }
        }

        GetLog() << req.ShortDebugString();
        mNetwork->WriteMessage(req);

        ResponseMsg rsp;
        mNetwork->ReadMessage(rsp);
        if (rsp.type() == RESPONSE_OK)
        {
            GetLog() << "Turn done";
        }
        else
        {
            GetLog() << rsp.ShortDebugString();
        }

        req.Clear();
        req.set_type(REQUEST_GET_TIME);
        mNetwork->WriteMessage(req);

        rsp.Clear();
        mNetwork->ReadMessage(rsp);
        switch (rsp.type())
        {
        case RESPONSE_CHANGES:
            while (!rsp.last())
            {
                LoadEvents(rsp);
                rsp.Clear();
                mNetwork->ReadMessage(rsp);
            }
            LoadEvents(rsp);
            mTime = rsp.time();
            mIngameSheet.SetTime(mTime);
            break;
        case RESPONSE_PLEASE_WAIT:
            break;
        default:
            GetLog() << rsp.ShortDebugString();
            break;
        }
        mSyncTimer.Reset(2000);
    }
}

