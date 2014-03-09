#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"
#include "DataDefs.h"

#include <vector>
#include <string>
#include <map>
#include <memory>
#include <PassiveSocket.h>
#include <ActiveSocket.h>
#include "tinythread.h"
#include "MiscUtils.h"
#include "LuaTools.h"
#include "DataFuncs.h"
#include <stdexcept> //todo convert errors to lua-errors and co. Then remove this

using namespace DFHack;
using namespace df::enums;

class Connection;
CPassiveSocket* server=0;
tthread::recursive_mutex clients_mutex;
typedef tthread::lock_guard<tthread::recursive_mutex> guard_t;
std::map<int,std::shared_ptr<Connection> > clients;

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
            try{
                int pos=mysock->Receive(sizeof(uint32_t));
                if(pos<=0)
                {
                    throw std::runtime_error(translate_socket_error(mysock->GetSocketError()));
                }
                uint32_t *d=(uint32_t*)mysock->GetData();
                if(d==0)
                    throw std::runtime_error(translate_socket_error(mysock->GetSocketError()));
                uint32_t size=ntohl(*d);
                uint8_t* buffer=new uint8_t[size]; //std::vector here...
                uint8_t* cptr=buffer;
                uint32_t csize=0;
                while(csize<size)
                {
                    int ret=mysock->Receive(sizeof(uint32_t));
                    if(ret<=0)
                    {
                        delete [] buffer;
                        throw std::runtime_error(translate_socket_error(mysock->GetSocketError()));
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
            catch(std::exception& e)
            {
                if(alive)
                {
                    CoreSuspender suspend;
                    color_ostream_proxy out(Core::getInstance().getConsole());
                    out.printerr("%s\n",e.what());
                    Close();
                }
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
        guard_t guard(clients_mutex);
        clients.erase(myId);
    }
    void Send(const uint8* buf,int32 size)
    {
        if(!alive)
            return;
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
        {
            CoreSuspender claim;
            color_ostream_proxy out(Core::getInstance().getConsole());
            onDisconnect(out,myId);
        }
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
        if(!server->Initialize())
        {
            throw std::runtime_error(translate_socket_error(server->GetSocketError()));
        }
        server->SetNonblocking();  
        if(!server->Listen((uint8_t*)ip.c_str(),port))
        {
            throw std::runtime_error(translate_socket_error(server->GetSocketError()));
        }
    }
}
static void lua_sock_connect(std::string ip,int port)
{
    CActiveSocket *sock=new CActiveSocket;
    if(!sock->Initialize())
    {
        throw std::runtime_error(translate_socket_error(sock->GetSocketError()));
    }
    if(!sock->Open((uint8*)ip.c_str(),port))
    {
        const char* err=translate_socket_error(sock->GetSocketError());
        delete sock;
        throw std::runtime_error(err);
    }
    std::shared_ptr<Connection> t(new Connection(sock,-1));
    {
        guard_t guard(clients_mutex);
        clients[-1]=t;
    }
    t->Start();

    CoreSuspender suspend;
    color_ostream_proxy out(Core::getInstance().getConsole());
    onNewClient(out,-1);
}
static void lua_sock_disconnect(int id)
{
    //from lua, so it's suspended already?
    if(id==-2 && server) //server shutdown
    {
        server->Close();
        guard_t guard(clients_mutex);
        clients.clear();
        return;
    }
    {    
        guard_t guard(clients_mutex);
        if(clients.find(id)!=clients.end())
        {
            clients[id]->Close();
        }
    }
}
static int lua_sock_send(lua_State* L)
{
    int id=luaL_checkint(L,1);
    guard_t guard(clients_mutex);
    if(clients.find(id)!=clients.end())
    {
        std::shared_ptr<Connection> sock=clients[id];
        if(!sock->isAlive())
            luaL_error(L,"Socket somehow died...");
        int size=luaL_checkint(L,2);
        uint8_t* data=Lua::CheckDFObject<uint8_t>(L,3,true);
        sock->Send(data,size);
    }
    else
    {
        luaL_error(L,"Invalid client id");
        //lua_error?
    }
    return 0;
}
DFHACK_PLUGIN_LUA_COMMANDS{
    DFHACK_LUA_COMMAND(lua_sock_send),
    DFHACK_LUA_END
};
DFHACK_PLUGIN_LUA_FUNCTIONS {
    DFHACK_LUA_FUNCTION(lua_sock_listen),
    DFHACK_LUA_FUNCTION(lua_sock_connect),
    DFHACK_LUA_FUNCTION(lua_sock_disconnect),
    DFHACK_LUA_END
};
DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    
    return CR_OK;
}

DFhackCExport command_result plugin_onupdate(color_ostream &out)
{
    static int last_id=0;
    if(server)
    {
        CActiveSocket* sock=server->Accept();
        if(sock)
        {
            
            std::shared_ptr<Connection> new_con(new Connection(sock,last_id));
            {
                guard_t guard(clients_mutex);
                clients[last_id]=new_con;
            }
            CoreSuspendClaimer claim;
            onNewClient(out,last_id);
            last_id++;
            new_con->Start();
        }
    }
    return CR_OK;
}
DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    CoreSuspendClaimer claim;
    if(server)
    {
        delete server;
        server=NULL;
    }
    guard_t guard(clients_mutex);
    clients.clear();
    return CR_OK;
}