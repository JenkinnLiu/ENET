#include "../EdoyunNet/TcpConnection.h"
#include "LoadBanceServer.h"

class LoadBanceConnection : public TcpConnection
{
public:
	LoadBanceConnection(std::shared_ptr<LoadBanceServer> loadbance_server,TaskScheduler* task_scheduler, int sockfd);
	~LoadBanceConnection();
protected:
    void DisConnection();
	bool OnRead(BufferReader& buffer);
	bool IsTimeout(uint64_t timestamp);
	void HandleMessage(BufferReader& buffer);
	void HnadleLogin(BufferReader& buffer);
	void HandleMinoterInfo(BufferReader& buffer);
	private:
	int socket_;
	std::weak_ptr<LoadBanceServer> loadbance_server_;
};