#ifndef MARKET_MAKER_APP_H
#define MARKET_MAKER_APP_H

#include <quickfix/Application.h>
#include <quickfix/MessageCracker.h>
#include <quickfix/Mutex.h>
#include <quickfix/fix42/NewOrderSingle.h>
#include <quickfix/fix42/ExecutionReport.h>
#include <quickfix/fix42/OrderCancelReject.h>

#include <string>
#include <iostream>
#include <map>

class OrderBook; // Forward declaration
class StrategyEngine; // Forward declaration for new StrategyEngine

class MarketMakerApplication : public FIX::Application, public FIX::MessageCracker
{
public:
    // Constructor takes OrderBook and StrategyEngine
    MarketMakerApplication(OrderBook* orderBook, StrategyEngine* strategyEngine);

    void onCreate(const FIX::SessionID& sessionID) override;
    void onLogon(const FIX::SessionID& sessionID) override;
    void onLogout(const FIX::SessionID& sessionID) override;
    void toAdmin(FIX::Message& message, const FIX::SessionID& sessionID) override;
    void toApp(FIX::Message& message, const FIX::SessionID& sessionID) throw(FIX::DoNotSend) override;
    void fromAdmin(const FIX::Message& message, const FIX::SessionID& sessionID) throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::RejectLogon) override;
    void fromApp(const FIX::Message& message, const FIX::SessionID& sessionID) throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::UnsupportedMessageType) override;

    // MessageCracker overloads for incoming client orders
    void onMessage(const FIX42::NewOrderSingle& message, const FIX::SessionID& sessionID) override;

    // Public method for StrategyEngine to send Execution Reports to clients
    // FIX: Changed parameter from const FIX42::ExecutionReport& to FIX42::ExecutionReport&
    void sendExecutionReportToClient(FIX42::ExecutionReport& message, const FIX::SessionID& clientSessionID);

    // Get the current client session ID (used by StrategyEngine to check if a client is connected)
    FIX::SessionID getClientSessionID() const;

private:
    OrderBook* m_orderBook;
    StrategyEngine* m_strategyEngine; // Pointer to the StrategyEngine

    FIX::Mutex m_mutex;
    FIX::SessionID m_clientSessionID; // Stores the session ID of the connected client (MockTradeClient)
};

#endif // MARKET_MAKER_APP_H
