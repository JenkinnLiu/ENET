#ifndef RTMP_HANDSHAKE
#define RTMP_HANDSHAKE
#include "BufferReader.h"

class RtmpHandshake
{
public:
	enum State
	{
		HANDSHAKE_C0C1,
		HANDSHAKE_S0S1S2,
		HANDSHAKE_C2,
		HANDSHAKE_COMPLETE
	};

	RtmpHandshake(State state);
	virtual ~RtmpHandshake();

	int Parse(BufferReader& in_buffer, char* res_buf, uint32_t res_buf_size);

	bool IsCompleted() const
	{
		return handshake_state_ == HANDSHAKE_COMPLETE;
	}
	
	int BuildC0C1(char* buf,uint32_t buf_size);

private:
	State handshake_state_;
};
#endif
