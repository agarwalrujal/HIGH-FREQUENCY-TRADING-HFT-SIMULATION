// src/main_market_maker.cpp
#include "MarketMakerApp.h"
#include "MarketDataProcessor.h"
#include "MockMarketDataSource.h"
#include "OrderBook.h"
#include "StrategyEngine.h" // Include StrategyEngine header

#include <quickfix/FileStore.h>
#include <quickfix/FileLog.h>
#include <quickfix/SocketAcceptor.h>
#include <quickfix/SessionSettings.h>

#include <iostream>
#include <string>
#include <fstream>
#include <thread> // For std::thread

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cout << "usage: " << argv[0] << " MarketMaker.cfg" << std::endl;
        return 0;
    }

    std::string configFile = argv[1];

    try {
        // 1. Initialize Core Components
        OrderBook orderBook;
        MockMarketDataSource mockDataSource(&orderBook); // Market data source pushes to OrderBook
        MarketDataProcessor mdProcessor(&orderBook);     // Processor handles market data updates
                                                        // (In this setup, MockMarketDataSource directly updates OrderBook,
                                                        // so MDProcessor could be simplified or used for additional processing)

        // 2. Initialize Strategy Engine
        StrategyEngine strategyEngine(&orderBook, nullptr); // Pass nullptr for MarketMakerApp initially, set later

        // 3. Initialize Market Maker Application (FIX Acceptor)
        // Pass the OrderBook and the StrategyEngine to the MarketMakerApplication
        MarketMakerApplication marketMakerApp(&orderBook, &strategyEngine);

        // 4. Link StrategyEngine back to MarketMakerApp (resolves circular dependency)
        strategyEngine.setMarketMakerApp(&marketMakerApp);


        // QUICKFIX Engine Setup
        FIX::SessionSettings settings(configFile);
        FIX::FileStoreFactory storeFactory(settings);
        FIX::FileLogFactory logFactory(settings);
        FIX::SocketAcceptor acceptor(marketMakerApp, storeFactory, settings, logFactory);

        // Start FIX Acceptor
        acceptor.start();
        std::cout << "Market Maker FIX Acceptor started." << std::endl;

        // Start Mock Market Data Source in a separate thread
        std::cout << "Starting Mock Market Data Source..." << std::endl;
        std::thread mdThread([&mockDataSource]() {
            mockDataSource.startGeneratingData();
        });

        // Keep main thread alive
        std::cout << "Press ENTER to quit" << std::endl;
        std::string line;
        std::getline(std::cin, line);

        // Shutdown sequence
        std::cout << "Shutting down..." << std::endl;
        mockDataSource.stopGeneratingData();
        mdThread.join(); // Wait for MD thread to finish
        acceptor.stop();

        std::cout << "Market Maker stopped." << std::endl;

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
