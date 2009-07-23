#ifndef CLIENTUNIT_H
#define CLIENTUNIT_H
#include <Ogre.h>
#include <Typedefs.h>

class ClientTile;

class ClientUnit
{
public:
    ClientUnit(ClientTile& aTile, UnitId aUnitId);
    void SetPosition(ClientTile& aTile);
    void SetTarget(ClientTile* aTile);
    ClientTile* GetTarget() const { return mTarget; }
    ~ClientUnit();
    Ogre::Entity* CreateEntity();
    Ogre::SceneNode& GetNode() { return *mNode; }
protected:
private:
    ClientTile* mTile;
    ClientTile* mTarget;
    Ogre::SceneNode* mNode;
    UnitId mUnitId;
};

#endif // CLIENTUNIT_H
