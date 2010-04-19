#include <pch.h>
#include <ClientConnection.h>

#include <ServerGame.h>
#include <ServerUnit.h>
#include <ServerTile.h>
#include <ServerLog.h>
#include <Request.pb.h>
#include <Network.h>
#include <Response.pb.h>
#include <ChangeList.h>

void task_proc ClientConnectionThreadFunction(void *param)
{
    static_cast< ClientConnection* >(param)->Execute();
}

void ClientConnection::Execute()
{
    mGame.Send(*mNetwork);

    while (mNetwork->IsOk())
    {
        try
        {
            RequestMsg req;
            mNetwork->ReadMessage(req);
            if (req.has_type())
            {
                switch (req.type())
                {
                case REQUEST_DISCONNECT:
                    break;
                case REQUEST_GET_TIME:
                    if (req.has_time())
                    {
                        mGame.LoadCommands(req);
                        ChangeList::Write(*mNetwork, req.time());
                    }
                    else
                    {
                        ResponseMsg rsp;
                        rsp.set_type(RESPONSE_NOK);
                        rsp.set_reason("No time!");
                        mNetwork->WriteMessage(rsp);
                    }
                    break;
                }
            }
        }
        catch (std::runtime_error& e)
        {
            GetLog() << e.what();
        }
        task::reschedule();
    }

    mLive = false;
}

ClientConnection::ClientConnection(ServerGame& aGame, Network* aNetwork):
    mGame(aGame), mNetwork(aNetwork), mLive(true)
{
    task::create(ClientConnectionThreadFunction, this);
}

ClientConnection::~ClientConnection()
{
    delete mNetwork;
    GetLog() << "Socket deletd";
}
