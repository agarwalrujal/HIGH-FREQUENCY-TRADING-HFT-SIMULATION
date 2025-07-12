// src/main_mock_client.cpp
#include "MockTradeClient.h"
#include "OrderBook.h" // Crucial: Include OrderBook header

#include <quickfix/FileStore.h>
#include <quickfix/FileLog.h> // Using FileLog as per your last main_mock_client.cpp
#include <quickfix/SocketInitiator.h>
#include <quickfix/SessionSettings.h>

#include <iostream>
#include <string>
#include <fstream> // Required if you're reading config from a file
#include <thread>  // For std::this_thread::sleep_for
#include <chrono>  // For std::chrono::seconds

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cout << "usage: " << argv[0] << " <path_to_config_file>" << std::endl;
        return 0;
    }

    std::string configFile = argv[1];

    try {
        // 1. Create an instance of OrderBook.
        // In a real system, this would likely be shared and managed more robustly (e.g., via smart pointers).
        OrderBook orderBook;

        // 2. Pass a pointer to the OrderBook instance to MockTradeClient.
        // The constructor now expects MockTradeClient(OrderBook* orderBook).
        MockTradeClient mockClientApp(&orderBook);

        FIX::SessionSettings settings(configFile);
        FIX::FileStoreFactory storeFactory(settings);
        FIX::FileLogFactory logFactory(settings); // Using FileLogFactory
        FIX::SocketInitiator initiator(mockClientApp, storeFactory, settings, logFactory);

        initiator.start();
        std::cout << "Mock Trade Client FIX Initiator started." << std::endl;

        // Start the continuous order sending loop in MockTradeClient
        mockClientApp.startSendingOrders();

        std::cout << "Press ENTER to quit" << std::endl;
        std::string line;
        std::getline(std::cin, line);

        // Stop the continuous order sending loop before stopping the initiator
        mockClientApp.stopSendingOrders();
        initiator.stop();
        std::cout << "Mock Trade Client stopped." << std::endl;

        return 0;

    } catch (const FIX::Exception& e) {
        std::cerr << "FIX Exception: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Standard Exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown Exception" << std::endl;
        return 1;
    }
}
