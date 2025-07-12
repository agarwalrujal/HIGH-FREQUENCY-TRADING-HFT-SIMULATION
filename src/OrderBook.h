//
// OrderBook.h
// HFT
//
// Created by rujal agarwal on 6/6/25.
//
#ifndef ORDER_BOOK_H
#define ORDER_BOOK_H

#include <string>
#include <map>      // Now actively used for multiple symbols
#include <mutex>    // Crucial for thread safety
#include <iostream>
#include <cmath>    // For std::round (if needed for rounding prices)

class OrderBook {
public:
    // Structure to hold market data for a single symbol
    struct MarketData {
        double bid;
        double ask;
        double mid;

        // Constructor to initialize
        MarketData() : bid(0.0), ask(0.0), mid(0.0) {}
    };

    OrderBook() {} // Constructor no longer initializes member variables directly, map is empty

    void updateMarketData(const std::string& symbol, double bid, double ask) {
        std::lock_guard<std::mutex> lock(m_mutex); // Lock for thread safety

        // Get or create the MarketData entry for this symbol
        MarketData& data = m_symbolData[symbol]; // This will create a default-constructed MarketData if symbol doesn't exist

        data.bid = bid;
        data.ask = ask;

        if (bid > 0 && ask > 0) {
            data.mid = (bid + ask) / 2.0;
        } else {
            data.mid = 0.0;
        }

        std::cout << "OrderBook Updated: " << symbol
                  << " Bid=" << data.bid
                  << ", Ask=" << data.ask
                  << ", Mid=" << data.mid << std::endl;
    }

    // Helper to get MarketData for a symbol, returns a copy (thread-safe for read)
    // Or consider returning optional/pointer if symbol might not exist.
    // For simplicity, returns default MarketData if symbol not found.
    MarketData getMarketData(const std::string& symbol) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_symbolData.find(symbol);
        if (it != m_symbolData.end()) {
            return it->second; // Return the actual data
        }
        return MarketData(); // Return default (all zeros) if symbol not found
    }

    double getBestBid(const std::string& symbol) {
        return getMarketData(symbol).bid;
    }

    double getBestAsk(const std::string& symbol) {
        return getMarketData(symbol).ask;
    }

    double getMidPrice(const std::string& symbol) {
        return getMarketData(symbol).mid;
    }

private:
    std::mutex m_mutex;
    // Map to store MarketData for each symbol
    std::map<std::string, MarketData> m_symbolData;
};

#endif // ORDER_BOOK_H
