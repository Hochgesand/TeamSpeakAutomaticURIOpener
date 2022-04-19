#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

int main(int argc, char** argv);
int connectToTeamspeak(std::function<void(std::shared_ptr<beast::flat_buffer>)> callback);
void printBufferToConsole(std::shared_ptr<beast::flat_buffer> buffer);
void saveNewApiKey(std::string const& key);
std::string checkForExistingConfiguration();