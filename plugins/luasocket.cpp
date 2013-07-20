#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"

#include "DataDefs.h"

#include <vector>
#include <string>
#include <map>
#include "PassiveSocket.h"
#include "ActiveSocket.h"
#include "MiscUtils.h"
#include "LuaTools.h"
#include "DataFuncs.h"

using namespace DFHack;
using namespace df::enums;

CPassiveSocket* server=0;
std::map<int,CActiveSocket*> clients;

DFHACK_PLUGIN("luasocket");

// The error messages are taken from the clsocket source code
const char * translate_socket_error(CSimpleSocket::CSocketError err) {
    switch (err) {
        case CSimpleSocket::SocketError:
            return "Generic socket error translates to error below.";
        case CSimpleSocket::SocketSuccess:
            return "No socket error.";
        case CSimpleSocket::SocketInvalidSocket:
            return "Invalid socket handle.";
        case CSimpleSocket::SocketInvalidAddress:
            return "Invalid destination address specified.";
        case CSimpleSocket::SocketInvalidPort:
            return "Invalid destination port specified.";
        case CSimpleSocket::SocketConnectionRefused:
            return "No server is listening at remote address.";
        case CSimpleSocket::SocketTimedout:
            return "Timed out while attempting operation.";
        case CSimpleSocket::SocketEwouldblock:
            return "Operation would block if socket were blocking.";
        case CSimpleSocket::SocketNotconnected:
            return "Currently not connected.";
        case CSimpleSocket::SocketEinprogress:
            return "Socket is non-blocking and the connection cannot be completed immediately";
        case CSimpleSocket::SocketInterrupted:
            return "Call was interrupted by a signal that was caught before a valid connection arrived.";
        case CSimpleSocket::SocketConnectionAborted:
            return "The connection has been aborted.";
        case CSimpleSocket::SocketProtocolError:
            return "Invalid protocol for operation.";
        case CSimpleSocket::SocketFirewallError:
            return "Firewall rules forbid connection.";
        case CSimpleSocket::SocketInvalidSocketBuffer:
            return "The receive buffer point outside the process's address space.";
        case CSimpleSocket::SocketConnectionReset:
            return "Connection was forcibly closed by the remote host.";
        case CSimpleSocket::SocketAddressInUse:
            return "Address already in use.";
        case CSimpleSocket::SocketInvalidPointer:
            return "Pointer type supplied as argument is invalid.";
        case CSimpleSocket::SocketEunknown:
            return "Unknown error please report to mark@carrierlabs.com";
        default:
            return "No such CSimpleSocket error";
    }
}
static void handle_new_client(color_ostream &out,int id)
{

}
static void handle_data_recievend(color_ostream &out,int id,uint32_t size,uint8_t* buf)
{

}
static void handle_disconnect(color_ostream &out,int id)
{

}
DEFINE_LUA_EVENT_1(onNewClient, handle_new_client, int);
DEFINE_LUA_EVENT_3(onDataRecieved, handle_data_recievend,int,uint32_t ,uint8_t* );
DEFINE_LUA_EVENT_1(onDisconnect, handle_disconnect,int);

DFHACK_PLUGIN_LUA_EVENTS {
    DFHACK_LUA_EVENT(onNewClient),
    DFHACK_LUA_EVENT(onDataRecieved),
    DFHACK_LUA_EVENT(onDisconnect),
    DFHACK_LUA_END
};
class Connection
{
    CActiveSocket* mysock;
    tthread::thread *myThread;
    int myId;
    bool alive;
    static void work(void* args)
    {
        Connection* myself=static_cast<Connection*>(args);
        myself->run();
    }
    
    void run()
    {
        while(alive)
        {
            int pos=mysock->Receive(sizeof(uint32_t));
            if(pos<=0)
            {
                alive=false;
                return;
            }
            uint32_t *d=(uint32_t*)mysock->GetData();
            if(d!=0)
            {
                uint32_t size=ntohl(*d);
                uint8_t* buffer=new uint8_t[size]; //std::vector here...
                uint8_t* cptr=buffer;
                uint32_t csize=0;
                while(csize<size)
                {
                    int ret=mysock->Receive(sizeof(uint32_t));
                    if(ret<=0)
                    {
                        alive=false;
                        delete [] buffer;
                        return;
                    }
                    memcpy(cptr,mysock->GetData(),ret);
                    cptr+=ret;
                    csize+=ret;
                }
                CoreSuspender suspend;
                color_ostream_proxy out(Core::getInstance().getConsole());
                onDataRecieved(out,myId,size,buffer);
                delete [] buffer;
            }
            else
            {
                CoreSuspender suspend;
                color_ostream_proxy out(Core::getInstance().getConsole());
                out.printerr("%s\n",translate_socket_error(mysock->GetSocketError()));
                return;
            }
        }
    }
public:
    Connection(CActiveSocket* socket,int id):mysock(socket),myThread(0),myId(id)
    {
        socket->SetBlocking();
        alive=true;
    }
    bool isAlive(){return alive;}
    void Start()
    {
        myThread=new tthread::thread(work,this);
    }
    void Close()
    {
        alive=false;
        mysock->Close();//should do everything okay?
    }
    void Send(uint8* buf,int32 size)
    {
        uint8* bufn=new uint8[size+sizeof(int32)];
        int32* sizeB=reinterpret_cast<int32*>(bufn);
        *sizeB=htonl(size);
        memcpy(bufn+sizeof(int32),buf,size);
        if(mysock->Send(bufn,size+sizeof(int32))!=size+sizeof(int32))
        {
            CoreSuspender suspend;
            color_ostream_proxy out(Core::getInstance().getConsole());
            out.printerr("%s\n",translate_socket_error(mysock->GetSocketError()));
        }
        delete [] bufn;
    }
    ~Connection()
    {
        alive=false;
        mysock->Close();
        if(myThread->joinable())
            myThread->join();

        delete mysock;
        delete myThread;
    }
};

static void lua_sock_listen(std::string ip,int port)
{
    if(server==0)
    {
        server=new CPassiveSocket;
        server->SetNonblocking();  
        server->Listen((uint8_t*)ip.c_str(),port);
    }

}
static void lua_sock_connect(std::string ip,int port)
{
    CActiveSocket *sock=new CActiveSocket;
    if(!sock->Open((uint8*)ip.c_str(),port))
    {
        throw std::runtime_error("Failed to connect");
    }
    Connection *t=new Connection(sock,-1);
    clients[-1]=t;
    t->Start();
    CoreSuspender suspend;
    color_ostream_proxy out(Core::getInstance().getConsole());
    onNewClient(out,-1);
}
static void lua_sock_disconnect(int id)
{
    if(id==-2 && server) //server shutdown
    {
        CoreSuspender suspend;
        server->Close();
        for(auto it=clients.begin();it!=clients.end();it++)
        {
            delete it->second;
        }
        clients.clear();
        return;
    }
    if(clients.find(id)!=clients.end())
    {
        clients[id]->Close();
        //delete clients[id]; can't delete here. not my thread prob...
        //clients.erase(id);
        CoreSuspender suspend;
        color_ostream_proxy out(Core::getInstance().getConsole());
        onDisconnect(out,id);
    }

}
static void lua_sock_send(int id,int32 size,char* buf)
{

    if(clients.find(id)!=clients.end())
    {
        CActiveSocket *sock=clients[id];
        sock->Send((uint8*)buf,size);
    }

}
DFHACK_PLUGIN_LUA_FUNCTIONS {
    DFHACK_LUA_FUNCTION(lua_sock_listen),
    DFHACK_LUA_FUNCTION(lua_sock_connect),
    DFHACK_LUA_FUNCTION(lua_sock_disconnect),
    DFHACK_LUA_FUNCTION(lua_sock_send),
    DFHACK_LUA_END
};
DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    
    return CR_OK;
}

DFhackCExport command_result plugin_onupdate(color_ostream &out)
{
    static int last_id=0;
    /*if(server)
    {
        CActiveSocket* sock=server->Accept();
        if(sock)
        {
            clients[last_id]=sock;
            CoreSuspendClaimer claim;
            onNewClient(out,last_id);
            last_id++;
        }
    }
    for(auto it=clients.begin();it!=clients.end();)
    {
        CActiveSocket* a=it->second;
        uint32_t pos=a->Receive(sizeof(uint32_t));
        if(pos==0)
        {
            auto toErase=it;
            it++;
            clients.erase(toErase);
            onDisconnect(out,toErase->first);
        }
        else
            it++;
    }
    return CR_OK;
}
DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    if(server)
        delete server;
    for(std::map<int,CActiveSocket*>::iterator it=clients.begin();it!=clients.end();it++)
        delete it->second;
        onDisconnect(out,it->first);
    }
    clients.clear();
    return CR_OK;
}