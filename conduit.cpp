#include <chrono>
#include <iostream>

#ifdef _WIN32
#define _WIN32_WINNT 0x0A00

#endif
#define ASIO_STANDALONE
#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>


std::vector<char> vBuffer(1 * 1024);

// reading the data from open socket asynchronously using above vector as buffer
void GrabSomeData(asio::ip::tcp::socket& socket)
{
	socket.async_read_some(asio::buffer(vBuffer.data(), vBuffer.size()), 
		[&](const asio::error_code& ec, std::size_t bytes_transferred) // lambda function to be called when data is read
		{
			if (!ec)
			{
				std::cout << "\n\nBytes Transferred:" << bytes_transferred << "bytes\n\n";
				for (auto c : vBuffer)
				{
					std::cout << c;
				}
				GrabSomeData(socket); // call GrabSomeData function again to read more data from socket
				// it will keep running if there is data to read then wait until there is more data to read.
			}
			else
			{ 
				std::cout << "Error:" << ec.message() << "\n";
			}
		}
	);
}

int main() 
{
	// asio error entity
	asio::error_code ec;

	// platform specific interface
	asio::io_context context;

	// To solve the problem of context exiting immediately if it does not have jobs to do
	asio::io_context::work idleWork(context); // keep context idle by giving it fake tasks

	// run the context in its own thread
	std::thread thrContext = std::thread([&]() { context.run(); }); // so if context wants to stop, it wont stop the 
	// main thread

	// asio endpoint entity, endpoint is a specific address and port
	asio::ip::tcp::endpoint endpoint(asio::ip::make_address("xx.xx.xx.yy", ec), 80);
	
	// asio socket entity, socket is a connection to the endpoint
	asio::ip::tcp::socket socket(context);

	// let socket connect to the endpoint
	socket.connect(endpoint, ec);

	if (!ec) {
		std::cout << "connected!" << "\n";
	}
	else
	{
		std::cout << "failed to connect to address"<<ec.message() << "\n";
	}
	// added this for =just a quick check
	// check if socket is open
	if (socket.is_open())
	{	
		// we will read some data from the socket asynchronously before sending the request 
		GrabSomeData(socket);

		std::string sRequest = "Get /index.html HTTP/1.1\r\n"
			"Host: example.com\r\n"
			"Connection: close\r\n\r\n";

		// write some data from sRequest string using asio buffer and also give size
		socket.write_some(asio::buffer(sRequest.data(), sRequest.size()), ec);

		using namespace std::chrono_literals; // using namespace to avoid polluting the global namespace
		std::this_thread::sleep_for(20000ms); // sleep for 20 seconds to get all the data from the socket

		// IMPORTANT: 
		// following is the code for synchronous approach and is not recommended
		{	

			// // IMPORTANT: asio sockets are non-blocking, so we need to check if the socket is open
			// // we try to read as soon as we write to socket so we will never get a response and byte will always be 0
			// // as for processing the response, we need to wait for the socket and the socket would have exited by then
			// // brute-forcing a delay of 2 sec to see if we get response reads from open socket

			// // CODE:
			// // using namespace std::chrono_literals;
			// // std::this_thread::sleep_for(200ms);
			// // but instead of this we can use asio's wait function
			// socket.wait(asio::socket_base::wait_type::wait_read, ec);	
			
			// // bytes is the size of data available in the socket to read(a response)
			// size_t bytes = socket.available(); // size_t is a typedef for unsigned int for bytes available to read
			// std::cout << "Bytes Available:" << bytes << "\n";

			// // if bytes is greater than 0, then we have data to read
			// if (bytes > 0)
			// {	
			// 	// create a vector of char to store the data read from the socket
			// 	std::vector<char> vBuffer(bytes);
			// 	// read some data from the socket using asio buffer and also give size
			// 	socket.read_some(asio::buffer(vBuffer.data(), vBuffer.size()), ec); 

			// 	// print the data read from the socket
			// 	for (auto c : vBuffer)
			// 	{
			// 		std::cout << c;	
			// 	}
			// }

		}

	};


	system("pause");
	return 0;
};