#ifndef H_SERVER
#define H_SERVER

#include <memory>
#include <ev.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <system_error>

#include "log.h"

#define BAD_FD (-1)

class Server
{
	struct Client
	{
		ev_io read_watcher;
		int fd = BAD_FD;
		std::vector<char> read_buffer;
		std::vector<char> write_buffer;
		size_t write_offset = 0;

		explicit Client(int socket_fd) : fd(socket_fd)
		{
			read_buffer.reserve(1024);
			write_buffer.reserve(1024);
		}

		Client(const Client &) = delete;
		Client &operator=(const Client &) = delete;

		~Client()
		{
			if(fd != BAD_FD)
				close(fd);
		}
	};

  public:
	explicit Server(int port) : port_(port)
	{
	}
	~Server()
	{
		Stop();
	}

	Server(const Server &) = delete;
	Server &operator=(const Server &) = delete;

	void Start()
	{
		if(server_fd_ != BAD_FD)
		{
			LOG_I("Server already running");
			return;
		}

		server_fd_ = CreateSocket();
		SetupSocket(server_fd_);
		BindSocket(server_fd_);
		ListenSocket(server_fd_);

		loop_ = ev_loop_new(EVFLAG_AUTO);
		StartAcceptWatcher();

		LOG_I("Server started on port " + std::to_string(port_));
		ev_run(loop_, 0);
	}

	void Stop() noexcept
	{
		if(server_fd_ == BAD_FD)
			return;

		close(server_fd_);
		server_fd_ = BAD_FD;

		ev_io_stop(loop_, accept_watcher_.get());

		ev_break(loop_, EVBREAK_ALL);
		ev_loop_destroy(loop_);
	}

  private:
	static void ReadCallback(struct ev_loop *loop, ev_io *watcher, __attribute__((unused)) int revents)
	{
		auto *client = static_cast<Client *>(watcher->data);
		char buffer[1024];

		const ssize_t nread = read(client->fd, buffer, sizeof(buffer));
		if(nread < 0)
		{
			if(errno == EAGAIN || errno == EWOULDBLOCK)
				return;
			DeleteClient(loop, client);
			return;
		}

		if(nread == 0)
		{
			DeleteClient(loop, client);
			return;
		}

		client->read_buffer.insert(client->read_buffer.end(), buffer, buffer + nread);
		ProcessClientData(loop, client);
	}

	static void ProcessClientData(struct ev_loop *loop, Client *client)
	{
		LOG_I("Received: " + std::string(client->read_buffer.begin(), client->read_buffer.end()));

		client->write_buffer = client->read_buffer;
		client->read_buffer.clear();
		client->write_offset = 0;

		ssize_t written = 0;
		do
		{
			written = write(client->fd, client->write_buffer.data() + client->write_offset,
			                client->write_buffer.size() - client->write_offset);

			if(written < 0)
			{
				if(errno == EAGAIN || errno == EWOULDBLOCK)
					return;
				DeleteClient(loop, client);
				return;
			}

			client->write_offset += written;

		} while(client->write_offset != client->write_buffer.size());

		client->write_buffer.clear();
		client->write_offset = 0;
	}

	static void AcceptCallback(struct ev_loop *loop, ev_io *watcher, __attribute__((unused)) int revents)
	{
		sockaddr_in client_addr{};
		socklen_t client_len = sizeof(client_addr);

		const int client_fd = accept(watcher->fd, reinterpret_cast<sockaddr *>(&client_addr), &client_len);
		if(client_fd < 0)
		{
			if(errno != EAGAIN && errno != EWOULDBLOCK)
			{
				LOG_E("Accept error: " + std::string(strerror(errno)));
			}
			return;
		}

		try
		{
			SetNonBlocking(client_fd);
			auto client = new Client(client_fd);

			ev_io_init(&client->read_watcher, ReadCallback, client_fd, EV_READ);
			client->read_watcher.data = client;
			ev_io_start(loop, &client->read_watcher);

			std::ostringstream ss;
			char ip_str[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, INET_ADDRSTRLEN);
			ss << "New connection from: " << ip_str << ":" << ntohs(client_addr.sin_port);
			LOG_I(ss.str());
		}
		catch(const std::exception &e)
		{
			LOG_E("Client setup failed: " + std::string(e.what()));
		}
	}

	static void SetNonBlocking(int fd)
	{
		const int flags = fcntl(fd, F_GETFL);
		if(flags == -1)
			throw std::system_error(errno, std::system_category(), "fcntl F_GETFL");
		if(fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
		{
			throw std::system_error(errno, std::system_category(), "fcntl F_SETFL");
		}
	}

	static void DeleteClient(struct ev_loop *loop, Client *client) noexcept
	{
		ev_io_stop(loop, &client->read_watcher);
		delete client;
	}

	int CreateSocket() const
	{
		const int fd = socket(AF_INET, SOCK_STREAM, 0);
		if(fd < 0)
			throw std::system_error(errno, std::system_category(), "socket");
		return fd;
	}

	void SetupSocket(int fd) const
	{
		int opt = 1;
		if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
		{
			throw std::system_error(errno, std::system_category(), "setsockopt");
		}
		SetNonBlocking(fd);
	}

	void BindSocket(int fd) const
	{
		sockaddr_in addr{};
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port_);
		addr.sin_addr.s_addr = INADDR_ANY;

		if(bind(fd, reinterpret_cast<const sockaddr *>(&addr), sizeof(addr)) < 0)
		{
			throw std::system_error(errno, std::system_category(), "bind");
		}
	}

	void ListenSocket(int fd) const
	{
		if(listen(fd, SOMAXCONN) < 0)
		{
			throw std::system_error(errno, std::system_category(), "listen");
		}
	}

	void StartAcceptWatcher()
	{
		accept_watcher_ = std::make_unique<ev_io>();
		ev_io_init(accept_watcher_.get(), AcceptCallback, server_fd_, EV_READ);
		ev_io_start(loop_, accept_watcher_.get());
	}

	const int port_;
	int server_fd_ = BAD_FD;
	struct ev_loop *loop_ = nullptr;
	std::unique_ptr<ev_io> accept_watcher_;
};
#endif /* H_SERVER */