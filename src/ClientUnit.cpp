#include <pch.h>
#include <ClientUnit.h>
#include <ClientTile.h>
#include <ClientApp.h>

ClientUnit::ClientUnit(ClientTile& aTile, UnitId aUnitId): mTile(&aTile), mUnitId(aUnitId)
{
    mTile->SetUnit(this);
    mNode = mTile->GetNode().createChildSceneNode();
    mNode->setScale(Ogre::Vector3(0.01));
    mNode->setDirection(mTile->GetPosition(), Ogre::SceneNode::TS_LOCAL, Ogre::Vector3::UNIT_Y);
}

ClientUnit::~ClientUnit()
{
    //dtor
}

Ogre::Entity* ClientUnit::CreateEntity()
{
    Ogre::String indexName = Ogre::StringConverter::toString(mUnitId);
    Ogre::Entity* unit = ClientApp::GetSceneMgr().createEntity(indexName + "Unit.entity", "robot.mesh");
    mNode->attachObject(unit);
    mNode->setVisible(true);
    return unit;
}

void ClientUnit::SetPosition(ClientTile& aTile)
{
    mTile->SetUnit(NULL);
    mTile->GetNode().removeChild(mNode);
    aTile.SetUnit(this);
    aTile.GetNode().addChild(mNode);
    mTile = &aTile;
}
