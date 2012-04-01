#ifndef SERVERUNITTEST_H_INCLUDED
#define SERVERUNITTEST_H_INCLUDED

#include <cxxtest/TestSuite.h>
#include <ServerUnit.h>
#include <ServerTile.h>
#include <Ogre.h>
#include <ChangeList.h>

class ServerUnitTest: public CxxTest::TestSuite
{
public:
    void setUp()
    {
        mClass = new UnitClass(0, 100, 1);
        mTile = new ServerTile(1, Ogre::Vector3::UNIT_X);
        mTile2 = new ServerTile(2, Ogre::Vector3::UNIT_Z);
        mUnit = new ServerUnit(*mTile, *mClass, 1);
    }

    void tearDown()
    {
        delete mUnit;
        delete mTile;
        delete mTile2;
        delete mClass;
    }

    void TestMove()
    {
        mUnit->Move(*mTile2);
        TS_ASSERT_EQUALS(mTile->GetUnitId(), 0);
        TS_ASSERT_EQUALS(mTile2->GetUnitId(), mUnit->GetUnitId());
        TS_ASSERT_EQUALS(mTile2->GetTileId(), mUnit->GetUnitTile().GetTileId());
    }

    void TestDelete()
    {
        delete mUnit;
        mUnit = NULL;
        TS_ASSERT_EQUALS(mTile->GetUnitId(), 0);
    }

    void TestAgeZero()
    {
        TS_ASSERT(!mUnit->UpdateAgeAndIsTimeToDie(0));
    }

    void TestAgeThousend()
    {
        TS_ASSERT(mUnit->UpdateAgeAndIsTimeToDie(1000));
    }
private:
    ServerUnit* mUnit;
    ServerTile* mTile;
    ServerTile* mTile2;
    UnitClass* mClass;
};

#endif
