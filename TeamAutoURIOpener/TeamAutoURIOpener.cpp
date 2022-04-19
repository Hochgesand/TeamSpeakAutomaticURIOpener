#include "TeamAutoURIOpener.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <boost/asio/connect.hpp>
#include <nlohmann/json.hpp>
#include <Windows.h>
#include <shellapi.h>

using namespace nlohmann;

const auto defaultSettings = R"(
{
	"apiKey": "",
	"user": ""
}
)";

const auto initRequest = R"(
{
  "type": "auth",
  "payload": {
    "identifier": "cock.balltorture",
    "version": "0",
    "name": "Teamspeak Automatic Spotify musicplayer",
    "description": "An automatic spotify uri injector",
    "content": {
		"apiKey": ""
    }
  }
}
)";

json loadedConfig;

const std::string filename{ "settings.json" };
std::thread teamspeakConnectorWS;

void printBufferToConsole(std::shared_ptr<beast::flat_buffer> buffer)
{
	std::cout << beast::make_printable(buffer->data()) << "\n";
};

void playByUriOnSpotify(std::string spotifyUri)
{
	const std::wstring temp = std::wstring(spotifyUri.begin(), spotifyUri.end());
	const LPCWSTR wideString = temp.c_str();
	ShellExecute(0, 0, wideString, 0, 0, SW_SHOW);
}

int connectToTeamspeak(std::function<void(std::shared_ptr<beast::flat_buffer>)> callback)
{
    teamspeakConnectorWS = std::thread{
        [callback]()
        {
            try
		    {
				int isFirstPacket = 0;
		        auto const host = "localhost";
		        auto const port = "5899";

		        // The io_context is required for all I/O
		        net::io_context ioc;

		        // These objects perform our I/O
		        tcp::resolver resolver{ ioc };
		        websocket::stream<tcp::socket> ws{ ioc };

		        // Look up the domain name
		        auto const results = resolver.resolve(host, port);

		        // Make the connection on the IP address we get from a lookup
		        net::connect(ws.next_layer(), results.begin(), results.end());

		        // Set a decorator to change the User-Agent of the handshake
		        ws.set_option(websocket::stream_base::decorator(
		            [](websocket::request_type& req)
		            {
		                req.set(http::field::user_agent,
		                    std::string(BOOST_BEAST_VERSION_STRING) +
		                    " websocket-client-coro");
		            }));

		        // Perform the websocket handshake
		        ws.handshake(host, "/");

		        // Send the message
				if (loadedConfig.empty())
				{
					ws.write(net::buffer(std::string(initRequest)));
				}
				else
				{
					json initrequestJson = json::parse(initRequest);
					initrequestJson["payload"]["content"]["apiKey"] = loadedConfig["apiKey"];
					ws.write(net::buffer(std::string(initrequestJson.dump())));
				}

		        while (true)
		        {
			        // This buffer will hold the incoming message
					std::shared_ptr<beast::flat_buffer> buffer = std::make_shared<beast::flat_buffer>();
		            // Read a message into our buffer
		            ws.read(*buffer);
					callback(buffer);

					// Convert requestdata to string which we can parse
					std::ostringstream ss;
					ss << beast::make_printable(buffer->data());
					json json = json::parse(ss.str());

					// Check if spotifyuri, and open.
			        try
			        {
						auto soos = json["type"].get<std::string>();
				        if (soos._Equal("textMessage"))
				        {
							auto isSpotify = json["payload"]["message"].get<std::string>().substr(0, 8);
					        if (isSpotify._Equal("spotify:"))
					        {
								if (!loadedConfig["user"].get<std::string>()._Equal(json["payload"]["invoker"]["nickname"]))
								{
									playByUriOnSpotify(json["payload"]["message"].get<std::string>());
								}
					        }
				        }
			        }
			        catch (...)
			        {
			        }

					if (isFirstPacket == 0)
					{
						saveNewApiKey(json["payload"]["apiKey"].get<std::string>());
						isFirstPacket++;
					}

		        }
		    }
		    catch (std::exception const& e)
		    {
		        std::cerr << "Error: " << e.what() << std::endl;
		        return EXIT_FAILURE;
		    }
        }
    };
	return 1;
}

std::string checkForExistingConfiguration()
{
	try
	{
		std::ifstream ifs;
		ifs = std::ifstream(filename);
		loadedConfig = json::parse(ifs);
		return loadedConfig["apiKey"].get<std::string>();
	}
	catch (...)
	{
		std::cout << "Creating new file";
		std::fstream createdFile;
		createdFile.open(filename, std::ios_base::out);

		// Fill json
		json json = json::parse(defaultSettings);

		createdFile << json.dump();
		createdFile.close();
		return "";
	}
	
}

void saveNewApiKey(std::string const& key)
{
	try
	{
		std::ifstream ifs;
		ifs = std::ifstream(filename);
		json json = json::parse(ifs);
		json["apiKey"] = key;
		std::fstream createdFile;
		createdFile.open(filename, std::ios_base::out);

		// write json
		createdFile << json.dump();
		createdFile.close();
	} catch (...)
	{
		std::cout << "Something went wrong in saveNewApiKey";
		throw std::exception("Something went wrong in saveNewApiKey");
	}
}

// Sends a WebSocket message and prints the response
int main(int argc, char** argv)
{
	checkForExistingConfiguration();
    connectToTeamspeak(printBufferToConsole);
	teamspeakConnectorWS.join();
}