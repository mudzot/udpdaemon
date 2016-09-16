#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <poll.h>

#include <string>

struct DaemonConfig {
	DaemonConfig():
	bindAddr("0.0.0.0"), port("9872"), receiveBroadcast(false)
	{}

	std::string bindAddr;
	std::string port;
	bool receiveBroadcast;
};

class UdpDaemon {
public:
	UdpDaemon(const DaemonConfig& cfg):
	_cfg(cfg), _socket(-1), _timeout(1000)
	{
		_pfd.fd = -1;
		_pfd.events = POLLIN;
		_pfd.revents = 0;
	}
	virtual ~UdpDaemon() {}

	int bindAddr() {
		struct addrinfo *result, *rp;
		int rc = -1;
		int optOne = 1;

		//Empty configured address taken as wildcard
		if (_cfg.bindAddr.empty()) {
			rc = _getAddr(&result, NULL, _cfg.port.c_str());
		} else {
			rc = _getAddr(&result, _cfg.bindAddr.c_str(), _cfg.port.c_str());
		}

		if (rc < 0) {
			perror("getaddrinfo");
			return rc;
		}

		//Iterate over possible addresses
		for (rp = result; rp != NULL; rp = rp->ai_next) {
			_socket = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
			if (_socket < 0)
				continue;

			if (_cfg.receiveBroadcast) {
				setsockopt(_socket, SOL_SOCKET, SO_BROADCAST, &optOne, sizeof(optOne));
			}

			if (bind(_socket, rp->ai_addr, rp->ai_addrlen) == 0)
				break;

			close(_socket);
			_socket = -1;
		}

		//Done with result
		freeaddrinfo(result);

		if (rp == NULL) {
			//Cannot bind
			return 1;
		}

		return 0;
	}

	int run() {
		struct sockaddr_in peerAddr = {0};
		socklen_t addrSize = sizeof(peerAddr);
		int rc = -1;
		int nBytes = -1;

		_pfd.fd = _socket;
		while (1) {
			rc = poll(&_pfd, 1, _timeout);
			if (rc < 0) {
				//serious shit happened
				perror("poll");
				close(_socket);
				_socket = -1;
				return 1;
			}
			if (_pfd.revents & (POLLERR | POLLHUP)) {
				//less serious but should not continue either
				int error = 0;
				socklen_t errlen = sizeof(error);
				getsockopt(_socket, SOL_SOCKET, SO_ERROR, (void *)&error, &errlen);
				fprintf(stderr, "socket error %d", error);
				close(_socket);
				_socket = -1;
				return 2;
			}
			if (_pfd.revents & POLLIN) {
				addrSize = sizeof(peerAddr);
				nBytes = recvfrom(_socket, _buffer, sizeof(_buffer), 0, (struct sockaddr*)&peerAddr, &addrSize);
				if (nBytes < 0) {
					//discard error
					continue;
				}
				_onPacket(_buffer, nBytes, (struct sockaddr*)&peerAddr, addrSize);
			}
		}
	}

protected:
	//send with human readable recipient address
	int _sendPacket(const char* data, size_t sz, const char* addr, const char* port) {
		struct addrinfo * result;
		int rc = _getAddr(&result, addr, port);
		if (rc) {
			return rc;
		}
		//only take first found address
		if (result != NULL) {
			return _sendPacket(data, sz, result->ai_addr, result->ai_addrlen);
		}
		//no address found
		return 1;
	}

	virtual int _sendPacket(const char* data, size_t sz, struct sockaddr* rcpt, socklen_t addrSize) {
		return sendto(_socket, data, sz, 0, rcpt, addrSize);
	}

	virtual int _onPacket(const char* data, size_t sz, struct sockaddr* peerAddr, socklen_t addrSize) {
		if (sz >= 4 && strncasecmp(data, "PING", 4) == 0) {
			//Reply PONG
			_sendPacket("PONG", 4, peerAddr, addrSize);
			printf("Received PING\n");
		}
		return 0;
	}

	int _getAddr(struct addrinfo ** pResult, const char* addr, const char* port) {
		struct addrinfo hints;

		memset(&hints, 0, sizeof(struct addrinfo));
		hints.ai_family = AF_UNSPEC;	//Support both IPv4 & v6
		hints.ai_socktype = SOCK_DGRAM;
		hints.ai_flags = AI_PASSIVE;	//Allow wildcard addresses
		hints.ai_protocol = 0;
		hints.ai_canonname = NULL;
		hints.ai_addr = NULL;
		hints.ai_next = NULL;

		return getaddrinfo(addr, port, &hints, pResult);
	}

protected:
	const static int BUFFER_SIZE = 4096;
	DaemonConfig _cfg;
	int _socket;
	int _timeout;
	pollfd _pfd;
	char _buffer[BUFFER_SIZE];
};
