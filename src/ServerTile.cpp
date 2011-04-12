#include <pch.h>

#include <ServerTile.h>

#include <CompareEdgesAngles.h>
#include <ServerUnit.h>

ServerTile::ServerTile(const Ogre::Vector3& aPosition, int32 aHeight):
        mPosition(aPosition),
		mHeight(aHeight),
        mTileId(0),
		mUnit(NULL)
{
    mNeighbourhood.reserve(6);
}

ServerTile::~ServerTile()
{
    //dtor
}

bool CompareEdgesAltitude(ServerTile* a,ServerTile* b)
{
    return a->GetPosition().z < b->GetPosition().z;
};

void ServerTile::SortNeighbourhood()
{
    assert(mNeighbourhood[0]);
    std::sort(mNeighbourhood.begin(), mNeighbourhood.end(), CompareEdgesAltitude);
    std::sort(mNeighbourhood.begin() + 1, mNeighbourhood.end(), CompareEdgesAngles<ServerTile>(mPosition, mNeighbourhood[0]->mPosition));
}


void ServerTile::RemoveNeighbour(ServerTile& aTile)
{
    std::vector< ServerTile* >::iterator i = std::find(mNeighbourhood.begin(), mNeighbourhood.end(), &aTile);
    assert(i != mNeighbourhood.end());
    mNeighbourhood.erase(i);
}

Ogre::Real CalcDistance(const Ogre::Vector3& a, const Ogre::Vector3& b)
{
    return acos(a.dotProduct(b));
}
