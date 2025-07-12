#ifndef STRATEGY_ENGINE_H
#define STRATEGY_ENGINE_H

#include <quickfix/Message.h>
#include <quickfix/SessionID.h>
#include <quickfix/Mutex.h>
#include <quickfix/fix42/NewOrderSingle.h>
#include <quickfix/fix42/ExecutionReport.h> // For processing our own ERs (future)

#include <string>
#include <map>
#include <thread>
#include <random>
#include <chrono>

class OrderBook; // Forward declaration
class MarketMakerApplication; // Forward declaration for communication

class StrategyEngine {
public:
    // Constructor takes OrderBook and a reference to the MarketMakerApp for callbacks
    StrategyEngine(OrderBook* orderBook, MarketMakerApplication* mmApp);
    ~StrategyEngine();

    // Setter for MarketMakerApplication pointer (to resolve circular dependency during init)
    void setMarketMakerApp(MarketMakerApplication* mmApp) { m_mmApp = mmApp; }

    // Method to receive client orders from MarketMakerApp
    void onNewOrderSingle(const FIX42::NewOrderSingle& message, const FIX::SessionID& clientSessionID);

    // Method to receive execution reports for our own quotes (if we sent them to an upstream)
    // For this mock setup, MarketMakerApp directly acts as the exchange for clients,
    // and its own quotes are internal for now. This will be more relevant if MMApp also initiates to an exchange.
    void onOurOwnExecutionReport(const FIX42::ExecutionReport& message);

    // Start/Stop the quoting logic thread
    void startQuoting();
    void stopQuoting();

private:
    OrderBook* m_orderBook;
    MarketMakerApplication* m_mmApp; // Pointer back to the MarketMakerApp for sending messages

    FIX::Mutex m_mutex;
    bool m_quotingRunning;
    std::thread m_quotingThread;
    FIX::SessionID m_clientSessionID; // Stores the session ID of the connected trade client (for internal tracking)

    // Internal state for market making
    std::string generateNewClOrdID();
    void manageQuotes(); // The core quoting logic function

    // Random number generation for mock quoting
    std::mt19937 m_randGen;
    std::uniform_int_distribution<> m_qtyDist;

    // A map to track our own outstanding quotes (for future advanced features)
    std::map<std::string, FIX::OrderID> m_clOrdIDtoOrderID; // Our ClOrdID -> Exchange OrderID for our own quotes
    std::map<std::string, FIX42::NewOrderSingle> m_ourOpenQuotes; // Our ClOrdID -> original quote message
};

#endif // STRATEGY_ENGINE_H
