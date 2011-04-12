#ifndef GEODESICGRIDTEST_H_INCLUDED
#define GEODESICGRIDTEST_H_INCLUDED

#include <cxxtest/TestSuite.h>

#include <ServerGeodesicGrid.h>
#include <CompareEdgesAngles.h>

class GeodesicGridTest: public CxxTest::TestSuite
{
public:
    void setUp()
    {
    }

    void tearDown()
    {
    }
    void TestGeodesicGrid()
    {
        ServerGeodesicGrid grid1(0, 5000);
        const Ogre::Real len = grid1.GetTile(0).GetPosition().length();

        for (size_t i = 0; i < grid1.GetTileCount(); ++i)
        {
            ServerTile& tile1 = grid1.GetTile(i);
            TS_ASSERT_DELTA(len, tile1.GetPosition().length(), 0.00001f);
            const size_t tileCount = tile1.GetNeighbourCount();
            TS_ASSERT(tileCount == 5 || tileCount == 6);
            const Ogre::Real dist = (tile1.GetPosition() - tile1.GetNeighbour(0).GetPosition()).length();
            for (size_t k = 0; k < tileCount; ++k)
            {
                const Ogre::Real curdist = (tile1.GetPosition() - tile1.GetNeighbour(k).GetPosition()).length();
                TS_ASSERT_DELTA(dist, curdist, dist * 0.15f);
            }
        }
    }

    void TestGeodesicGridSave()
    {
        ServerGeodesicGrid grid1(0, 5000);
        grid1.Save("test.gg");
        ServerGeodesicGrid grid2("test.gg");

        TS_ASSERT_EQUALS(grid1.GetTileCount(), grid2.GetTileCount());
        TS_ASSERT_EQUALS(grid1.GetEdgeCount(), grid2.GetEdgeCount());
        for (size_t i = 0; i < grid1.GetTileCount(); ++i)
        {
            ServerTile& tile1 = grid1.GetTile(i);
            ServerTile& tile2 = grid2.GetTile(i);
            TS_ASSERT(tile1.GetPosition().positionCloses(tile2.GetPosition(), 0.00001f));
            TS_ASSERT_EQUALS(tile1.GetTileId(), i);
            TS_ASSERT_EQUALS(tile2.GetTileId(), i);
            TS_ASSERT_EQUALS(tile1.GetNeighbourCount(), tile2.GetNeighbourCount());
            for (size_t j = 0; j < tile1.GetNeighbourCount(); ++j)
            {
                TS_ASSERT_EQUALS(tile1.GetNeighbour(j).GetTileId(), tile2.GetNeighbour(j).GetTileId());
            }
        }
    }
    void TestCompareAngles()
    {
        Ogre::Vector3 root(0,           0.52573108,  0.850650787);
        Ogre::Vector3 pole(0.309016973, 0.809016943, 0.49999997);
        CompareEdgesAngles<ServerTile> compare(root, pole);
        Ogre::Vector3 u(-0.309016973, 0.809016943, 0.49999997);
        Ogre::Radian angle = compare.CalcAngle(u);
        TS_ASSERT(angle == Ogre::Radian(5.02654839));
    }


    void TestCompareAngles2()
    {
        std::vector<ServerTile*> m;
        m.push_back(new ServerTile(Ogre::Vector3( 09.11,  23.83,  14.73), 0));
        m.push_back(new ServerTile(Ogre::Vector3(-09.11,  23.83,  14.73), 0));
        m.push_back(new ServerTile(Ogre::Vector3( 14.73,  09.11,  23.84), 0));
        m.push_back(new ServerTile(Ogre::Vector3(-14.73,  09.11,  23.84), 0));
        m.push_back(new ServerTile(Ogre::Vector3( 00.00,  15.49,  25.07), 0));
        m[0]->SetTileId(0);
        m[1]->SetTileId(1);
        m[2]->SetTileId(2);
        m[3]->SetTileId(3);
        m[4]->SetTileId(4);

        Ogre::Vector3 root(00.00,  15.49,  25.07);

        CompareEdgesAngles<ServerTile> compare(root, m.at(0)->GetPosition());
        std::sort(m.begin() + 1, m.end(), compare);

        TS_ASSERT_EQUALS(m[0]->GetTileId(), TileId(0));
        TS_ASSERT_EQUALS(m[1]->GetTileId(), TileId(2));
        TS_ASSERT_EQUALS(m[2]->GetTileId(), TileId(4));
        TS_ASSERT_EQUALS(m[3]->GetTileId(), TileId(3));
        TS_ASSERT_EQUALS(m[4]->GetTileId(), TileId(1));
    }

};


#endif // GEODESICGRIDTEST_H_INCLUDED
