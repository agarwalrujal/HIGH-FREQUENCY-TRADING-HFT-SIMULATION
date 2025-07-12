//
//  MarketDataProcessor.h
//  HFT
//
//  Created by rujal agarwal on 6/6/25.
//
#ifndef MARKET_DATA_PROCESSOR_H
#define MARKET_DATA_PROCESSOR_H

#include "OrderBook.h"
#include <string>
#include <iostream>

class MarketDataProcessor {
public:
    MarketDataProcessor(OrderBook* orderBook) : m_orderBook(orderBook) {}

    // In a real system, this would receive raw market data messages
    // and parse/process them to update the OrderBook.
    // For this mock, the MockMarketDataSource directly calls OrderBook.
    void processMarketData(const std::string& rawData) {
        // Example: parse rawData and call m_orderBook->updateMarketData(...)
        // std::cout << "MarketDataProcessor: Processing raw data (mock): " << rawData << std::endl;
    }

private:
    OrderBook* m_orderBook;
};

#endif // MARKET_DATA_PROCESSOR_H
