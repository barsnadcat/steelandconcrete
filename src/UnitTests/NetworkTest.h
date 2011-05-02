#ifndef NETWORKTEST_H_INCLUDED
#define NETWORKTEST_H_INCLUDED

// MyTestSuite.h
#include <cxxtest/TestSuite.h>
#include <proto/Tile.pb.h>
#include <proto/Vector.pb.h>

#include <iostream>
#include <cstdlib>
#include <ChangeList.h>
#include <DummyNetwork.h>
#include <Exceptions.h>

class MyTestSuite : public CxxTest::TestSuite
{
public:
    void setUp()
    {
        mNetwork = new DummyNetwork();
        mChangeList1 = new ChangeList();
        mChangeList2 = new ChangeList();
        mChangeList2->SetTileId(2);
        mChangeList1->SetTileId(1);
    }

    void tearDown()
    {
        delete mNetwork;
        delete mChangeList1;
        delete mChangeList2;
    }

    void TestChangeListOneBlock()
    {
        const int count = 50;
        for (int i = 0; i < count; ++i)
        {
            mChangeList1->AddRemove(i);
        }
        VisibleTiles visibleTiels;
        mChangeList1->Write(*mNetwork, 0, visibleTiels);

        TS_ASSERT_EQUALS(mNetwork->GetChangesWrited(), count);
        TS_ASSERT_EQUALS(mNetwork->GetWrites(), 1);
    }

    void TestLeave()
    {
        mChangeList1->AddLeave(1, 2);
        VisibleTiles visibleTiels;
        visibleTiels.insert(std::make_pair(1, mChangeList1));
        mChangeList1->Write(*mNetwork, 0, visibleTiels);

        TS_ASSERT_EQUALS(mNetwork->GetChangesWrited(), 1);
        TS_ASSERT_EQUALS(mNetwork->GetWrites(), 1);
    }

    void TestEnter()
    {
        mChangeList1->AddEnter(1, 1, 2);
        VisibleTiles visibleTiels;
        visibleTiels.insert(std::make_pair(1, mChangeList1));
        mChangeList1->Write(*mNetwork, 0, visibleTiels);

        TS_ASSERT_EQUALS(mNetwork->GetChangesWrited(), 1);
        TS_ASSERT_EQUALS(mNetwork->GetWrites(), 1);
    }



    void TestChangeListTwoTimes()
    {
        mChangeList1->AddRemove(0);
        mChangeList1->Commit();
        mChangeList1->AddRemove(1);
        mChangeList1->Commit();
        mChangeList1->AddRemove(2);
        VisibleTiles visibleTiels;
        mChangeList1->Write(*mNetwork, 0, visibleTiels);


        TS_ASSERT_EQUALS(mNetwork->GetChangesWrited(), 1);
        TS_ASSERT_EQUALS(mNetwork->GetWrites(), 1);
    }

    void TestChangeListClientWrong()
    {
        mChangeList1->AddRemove(0);
        mChangeList1->Commit();
        mChangeList1->AddRemove(1);
        VisibleTiles visibleTiels;
        TS_ASSERT_THROWS_ANYTHING(mChangeList1->Write(*mNetwork, 2, visibleTiels));


        TS_ASSERT_EQUALS(mNetwork->GetChangesWrited(), 0);
        TS_ASSERT_EQUALS(mNetwork->GetWrites(), 0);
    }

    void TestClientEvents()
    {
        mChangeList1->AddRemove(3);
        mChangeList2->AddRemove(2);
        mChangeList1->Commit();
        mChangeList2->Commit();
        mChangeList1->AddEnter(1, 1, 2);
        mChangeList2->AddLeave(1, 1);

        VisibleTiles visibleTiles;
        visibleTiles.insert(std::make_pair(1, mChangeList1));
        visibleTiles.insert(std::make_pair(2, mChangeList2));
        SendChanges(*mNetwork, visibleTiles, 8, 10, 1);
        TS_ASSERT_EQUALS(mNetwork->GetChangesWrited(), 4);
        TS_ASSERT_EQUALS(mNetwork->GetWrites(), 4);
    }

private:
    DummyNetwork* mNetwork;
    ChangeList* mChangeList1;
    ChangeList* mChangeList2;
};


#endif
