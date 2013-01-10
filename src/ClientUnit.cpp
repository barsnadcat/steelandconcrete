#include <pch.h>
#include <ClientUnit.h>

#include <ClientGridNode.h>
#include <ClientApp.h>
#include <VisualCodes.h>
#include <ClientTile.h>

const char* WALK_ANIMATION_NAME = "walk";

ClientUnit::ClientUnit(UnitId aUnitId, uint32 aVisual, ClientGridNode* aTile):
    mTile(aTile),
    mEntity(NULL),
    mUnitId(aUnitId),
    mVisualCode(aVisual),
    mPositionNode(NULL),
    mDirectonNode(NULL),
    mAnimState(NULL)
{
    assert(mTile);
    mTile->SetUnit(this);
    const Ogre::Vector3 pos = mTile->GetPosition();

    mPositionNode = ClientApp::GetSceneMgr().getRootSceneNode()->createChildSceneNode();
    const Ogre::Quaternion orientation = Ogre::Vector3::UNIT_Z.getRotationTo(pos);
    mPositionNode->setOrientation(orientation);
    mDirectonNode = mPositionNode->createChildSceneNode();
    mDirectonNode->translate(pos, Ogre::Node::TS_WORLD);
    mDirectonNode->setFixedYawAxis(true, pos);

    Ogre::String indexName = Ogre::StringConverter::toString(mUnitId);
    mEntity = ClientApp::GetSceneMgr().createEntity(indexName + "Unit.entity", GetMesh(mVisualCode));
    mDirectonNode->attachObject(mEntity);
    mDirectonNode->setVisible(true);
    if (mEntity->hasSkeleton())
    {
        mAnimState = mEntity->getAnimationState(WALK_ANIMATION_NAME);
        mAnimState->setLoop(true);
    }
}

ClientUnit::~ClientUnit()
{
    ClientGame::EraseUnitId(mUnitId);
    ClientApp::GetSceneMgr().destroyEntity(mEntity);
    ClientApp::GetSceneMgr().destroySceneNode(mPositionNode);
    ClientApp::GetSceneMgr().destroySceneNode(mDirectonNode);
    mTile->RemoveUnit();
}

void ClientUnit::UpdateMovementAnimation(FrameTime aFrameTime)
{
    if (mMoveAnim)
    {
        mMoveAnim->Update(aFrameTime);
        const Ogre::Quaternion orientation = mMoveAnim->GetPosition();
        mPositionNode->setOrientation(orientation);
        mAnimState->addTime(aFrameTime / 1000000.0f);
        if (mMoveAnim->IsDone())
        {
            mMoveAnim.reset();
            if (mAnimState)
            {
               mAnimState->setEnabled(false);
            }
        }
    }
}

void ClientUnit::SetTile(ClientGridNode* aTile)
{
    assert(aTile);
    if (aTile == mTile)
    {
        return;
    }

    const Ogre::Vector3 origin = mTile->GetPosition();
    const Ogre::Vector3 destin = aTile->GetPosition();
    mMoveAnim.reset(new MovementAnimation(origin, destin));
    if (mAnimState)
    {
        mAnimState->setEnabled(true);
    }

    const Ogre::Vector3 dir = destin - origin;
    mDirectonNode->setDirection(dir, Ogre::Node::TS_WORLD, Ogre::Vector3::NEGATIVE_UNIT_Y);

    mTile->RemoveUnit();
    aTile->SetUnit(this);
    mTile = aTile;
}
