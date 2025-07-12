#ifndef MOCK_TRADE_CLIENT_H
#define MOCK_TRADE_CLIENT_H

#include <quickfix/Application.h>
#include <quickfix/MessageCracker.h>
#include <quickfix/Message.h> // Corrected from 'Messages.h' in a previous step
#include <quickfix/Mutex.h>
#include "OrderBook.h" // Your custom OrderBook header

// IMPORTANT: Include specific FIX 4.2 message headers from the 'fix42' subdirectory
#include <quickfix/fix42/NewOrderSingle.h>
#include <quickfix/fix42/ExecutionReport.h>
#include <quickfix/fix42/OrderCancelReject.h>

// You also implicitly need these, which might be in the top-level QuickFIX headers or other specific ones.
// It's good practice to include them explicitly if they are used by name in your .cpp file.
#include <quickfix/FieldConvertors.h> // For FIX::UtcTimeStamp
#include <quickfix/FixFields.h>       // For fields like FIX::LastQty, FIX::LastPx etc.

#include <string>
#include <atomic>
#include <thread>
#include <random>
#include <chrono>
#include <vector> // For storing list of symbols

class MockTradeClient : public FIX::Application, public FIX::MessageCracker {
public:
    MockTradeClient(OrderBook* orderBook);

    // QuickFIX Callbacks
    void onCreate(const FIX::SessionID& sessionID) override;
    void onLogon(const FIX::SessionID& sessionID) override;
    void onLogout(const FIX::SessionID& sessionID) override;
    void toAdmin(FIX::Message& message, const FIX::SessionID& sessionID) override;
    void toApp(FIX::Message& message, const FIX::SessionID& sessionID) throw(FIX::DoNotSend) override;
    void fromAdmin(const FIX::Message& message, const FIX::SessionID& sessionID) throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::RejectLogon) override;
    void fromApp(const FIX::Message& message, const FIX::SessionID& sessionID) throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::UnsupportedMessageType) override;

    // QuickFIX Message Handlers (dispatched by crack())
    void onMessage(const FIX42::ExecutionReport& message, const FIX::SessionID& sessionID) override;
    void onMessage(const FIX42::OrderCancelReject& message, const FIX::SessionID& sessionID) override;

    // Custom methods for client logic
    void sendNewOrderSingle();
    void startSendingOrders();
    void stopSendingOrders();

private:
    std::string generateClOrdID();

    OrderBook* m_orderBook;
    FIX::SessionID m_sessionID;
    long m_clOrdID;
    std::atomic<bool> m_running;
    std::thread m_orderSendingThread;
    FIX::Mutex m_mutex;

    std::mt19937 m_randGen;
    std::uniform_real_distribution<> m_priceFluctuationDist;
    std::uniform_int_distribution<> m_qtyDist;
    std::uniform_int_distribution<> m_sideDist;
    std::uniform_int_distribution<> m_ordTypeDist;
    std::uniform_int_distribution<> m_transactTimeDist;

    std::vector<std::string> m_tradeSymbols;
    std::uniform_int_distribution<> m_symbolIndexDist;
};

#endif // MOCK_TRADE_CLIENT_H
