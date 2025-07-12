#ifndef MOCK_MARKET_DATA_SOURCE_H
#define MOCK_MARKET_DATA_SOURCE_H

#include "OrderBook.h"
#include <string>
#include <vector>   // To store multiple symbols
#include <thread>
#include <atomic>
#include <random>
#include <chrono>
#include <map>      // To store random distributions per symbol

class MockMarketDataSource {
public:
    MockMarketDataSource(OrderBook* orderBook)
        : m_orderBook(orderBook), m_running(false),
          m_randGen(std::chrono::system_clock::now().time_since_epoch().count()) {

        // Define 10 stock symbols and their initial base prices for varied distribution
        m_symbols = {
            {"AAPL", 170.0}, {"MSFT", 420.0}, {"GOOG", 180.0}, {"AMZN", 185.0},
            {"NVDA", 1000.0}, {"TSLA", 175.0}, {"META", 490.0}, {"NFLX", 650.0},
            {"ADBE", 520.0}, {"CRM", 240.0}
        };

        // Initialize a price distribution for each symbol
        for (const auto& entry : m_symbols) {
            // Each symbol gets a distribution around its base price, e.g., +/- 1.0 unit
            m_priceDists[entry.first] = std::uniform_real_distribution<>(entry.second - 1.0, entry.second + 1.0);
        }
    }

    void startGeneratingData() {
        m_running = true;
        m_dataThread = std::thread([this]() {
            while (m_running) {
                for (const auto& entry : m_symbols) {
                    const std::string& symbol = entry.first;
                    // Get the specific distribution for this symbol
                    std::uniform_real_distribution<>& currentDist = m_priceDists.at(symbol);

                    double bid = currentDist(m_randGen);
                    // Generate ask price slightly higher than bid, with a small random spread
                    double ask = bid + 0.01 + (currentDist(m_randGen) * 0.005); // Spread between 0.01 and approx (1.0 + 0.01 + 5) * 0.005

                    // Ensure ask is always strictly greater than bid
                    if (ask <= bid) {
                        ask = bid + 0.01; // Minimum spread
                    }

                    if (m_orderBook) {
                        m_orderBook->updateMarketData(symbol, bid, ask);
                    }
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // Update all symbols every 1 second
            }
        });
    }

    void stopGeneratingData() {
        m_running = false;
        if (m_dataThread.joinable()) {
            m_dataThread.join();
        }
    }

private:
    OrderBook* m_orderBook;
    std::atomic<bool> m_running;
    std::thread m_dataThread;
    std::mt19937 m_randGen;

    // Use a vector of pairs for initial symbols and their base prices
    std::vector<std::pair<std::string, double>> m_symbols;
    // Map to store a unique price distribution for each symbol
    std::map<std::string, std::uniform_real_distribution<>> m_priceDists;
};

#endif // MOCK_MARKET_DATA_SOURCE_H
