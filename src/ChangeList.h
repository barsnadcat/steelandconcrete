#ifndef CHANGELIST_H
#define CHANGELIST_H
#include <Typedefs.h>
#include <Response.pb.h>
#include <sockio.h>

class ChangeList
{
public:
    static void AddMove(UnitId aUnit, TileId aPosition);
    static void AddCommandDone(UnitId aUnit);
    static void Clear();
    static void Write(socket_t& aSocket, GameTime aTime);
private:
    static ResponseMsg mChangeList;
};

#endif // CHANGELIST_H
