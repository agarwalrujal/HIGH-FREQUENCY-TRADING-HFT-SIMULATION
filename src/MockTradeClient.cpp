#include "MockTradeClient.h"
#include <quickfix/Session.h>
#include <quickfix/FieldConvertors.h> // For FIX::UtcTimeStamp
#include <quickfix/FixFields.h>       // Needed for FIX::LastQty, FIX::LastPx etc.
#include <iomanip>                    // For std::fixed, std::setprecision
#include <algorithm>                  // For std::find (though not directly used, good to keep in mind for symbol validation)

// Constructor now takes an OrderBook pointer
MockTradeClient::MockTradeClient(OrderBook* orderBook)
    : m_orderBook(orderBook), // Initialize the OrderBook pointer
      m_clOrdID(0), m_running(false),
      m_randGen(std::chrono::system_clock::now().time_since_epoch().count()),
      m_priceFluctuationDist(-0.5, 0.5), // Orders will be +/- 0.5 from mid-price
      m_qtyDist(10, 100),               // Quantity range
      m_sideDist(0, 1),                 // 0: Buy, 1: Sell
      m_ordTypeDist(0, 1),              // 0: Market, 1: Limit
      m_transactTimeDist(1000, 5000) {   // Random delay between 1-5 seconds

    // Define the 10 stock symbols to trade (must match MockMarketDataSource)
    m_tradeSymbols = {
        "AAPL", "MSFT", "GOOG", "AMZN", "NVDA",
        "TSLA", "META", "NFLX", "ADBE", "CRM"
    };

    // Initialize distribution for randomly picking a symbol
    if (!m_tradeSymbols.empty()) {
        m_symbolIndexDist = std::uniform_int_distribution<>(0, m_tradeSymbols.size() - 1);
    }
}

void MockTradeClient::onCreate(const FIX::SessionID& sessionID) {
    std::cout << "MockTradeClient onCreate: " << sessionID << std::endl;
}

void MockTradeClient::onLogon(const FIX::SessionID& sessionID) {
    std::cout << "MockTradeClient onLogon: " << sessionID << std::endl;
    m_sessionID = sessionID;
    startSendingOrders(); // Start sending orders once logged on
}

void MockTradeClient::onLogout(const FIX::SessionID& sessionID) {
    std::cout << "MockTradeClient onLogout: " << sessionID << std::endl;
    stopSendingOrders(); // Stop sending orders on logout
}

void MockTradeClient::toAdmin(FIX::Message& message, const FIX::SessionID& sessionID) {
    // std::cout << "MockTradeClient toAdmin: " << message << std::endl;
}

void MockTradeClient::toApp(FIX::Message& message, const FIX::SessionID& sessionID) throw(FIX::DoNotSend) {
    // std::cout << "MockTradeClient toApp: " << message << std::endl;
}

void MockTradeClient::fromAdmin(const FIX::Message& message, const FIX::SessionID& sessionID) throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::RejectLogon) {
    // std::cout << "MockTradeClient fromAdmin: " << message << std::endl;
}

void MockTradeClient::fromApp(const FIX::Message& message, const FIX::SessionID& sessionID) throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::UnsupportedMessageType) {
    crack(message, sessionID); // Dispatches to onMessage handler
}

void MockTradeClient::onMessage(const FIX42::ExecutionReport& message, const FIX::SessionID& sessionID) {
    FIX::ClOrdID clOrdID;
    FIX::OrdStatus ordStatus;
    FIX::ExecType execType;
    FIX::LastQty lastQty(0);
    FIX::LastPx lastPx(0);
    FIX::CumQty cumQty(0);
    FIX::AvgPx avgPx(0);
    FIX::Text text; // For reject reason

    // Standard fields that are typically part of the ExecutionReport for FIX 4.2
    FIX::OrderID orderID;
    FIX::ExecID execID;
    FIX::Symbol symbol;
    FIX::Side side;
    FIX::OrderQty orderQty;
    FIX::LeavesQty leavesQty;

    message.get(clOrdID);
    message.get(ordStatus);
    message.get(execType);
    message.get(orderID);
    message.get(execID);
    message.get(symbol);
    message.get(side);
    message.get(orderQty);
    message.get(leavesQty);
    message.get(cumQty);
    message.get(avgPx);

    std::cout << "\nMockTradeClient: Received ExecutionReport for ClOrdID: " << clOrdID.getValue()
              << ", OrderID: " << orderID.getValue()
              << ", ExecID: " << execID.getValue()
              << ", Symbol: " << symbol.getValue()
              << ", Side: " << (side == FIX::Side_BUY ? "BUY" : "SELL")
              << ", Qty: " << static_cast<int>(orderQty.getValue())
              << ", Status: " << ordStatus.getValue()
              << ", ExecType: " << execType.getValue();

    if (message.isSetField(FIX::FIELD::LastQty)) {
        message.getField(lastQty);
    }
    if (message.isSetField(FIX::FIELD::LastPx)) {
        message.getField(lastPx);
    }

    if (message.isSetField(FIX::FIELD::Text)) {
        message.get(text);
    }

    std::cout << ", LastQty: " << static_cast<int>(lastQty.getValue())
              << ", LastPx: " << std::fixed << std::setprecision(2) << lastPx.getValue()
              << ", CumQty: " << static_cast<int>(cumQty.getValue())
              << ", AvgPx: " << std::fixed << std::setprecision(2) << avgPx.getValue();

    if (!text.getValue().empty()) {
        std::cout << ", Text: " << text.getValue();
    }
    std::cout << std::endl;
}

void MockTradeClient::onMessage(const FIX42::OrderCancelReject& message, const FIX::SessionID& sessionID) {
    FIX::ClOrdID clOrdID;
    FIX::OrdStatus ordStatus;
    message.get(clOrdID);
    message.get(ordStatus);

    std::cout << "MockTradeClient: Received OrderCancelReject for ClOrdID: " << clOrdID.getValue()
              << ", Status: " << ordStatus.getValue() << std::endl;
}

std::string MockTradeClient::generateClOrdID() {
    return "CLIENT-ORDER-" + std::to_string(++m_clOrdID);
}

void MockTradeClient::sendNewOrderSingle() {
    if (!FIX::Session::doesSessionExist(m_sessionID) || !FIX::Session::lookupSession(m_sessionID)->isLoggedOn()) {
        // std::cout << "MockTradeClient: Not logged on, cannot send order." << std::endl;
        return;
    }

    if (m_tradeSymbols.empty()) {
        std::cerr << "MockTradeClient Error: No symbols defined to trade." << std::endl;
        return;
    }

    // 1. Randomly select a symbol
    std::string selectedSymbol = m_tradeSymbols[m_symbolIndexDist(m_randGen)];

    FIX42::NewOrderSingle newOrderSingle;
    newOrderSingle.set(FIX::ClOrdID(generateClOrdID()));
    newOrderSingle.set(FIX::HandlInst(FIX::HandlInst_AUTOMATED_EXECUTION_NO_INTERVENTION));
    newOrderSingle.set(FIX::Symbol(selectedSymbol)); // Set the randomly chosen symbol

    FIX::Side side = (m_sideDist(m_randGen) == 0) ? FIX::Side_BUY : FIX::Side_SELL;
    newOrderSingle.set(FIX::Side(side));

    newOrderSingle.set(FIX::TransactTime(FIX::UtcTimeStamp::now()));
    newOrderSingle.set(FIX::OrderQty(m_qtyDist(m_randGen)));

    FIX::OrdType ordType = (m_ordTypeDist(m_randGen) == 0) ? FIX::OrdType_MARKET : FIX::OrdType_LIMIT;
    newOrderSingle.set(FIX::OrdType(ordType));

    if (ordType == FIX::OrdType_LIMIT) {
        // Get the current mid-price for the selected symbol from the OrderBook
        double currentMidPrice = m_orderBook->getMidPrice(selectedSymbol);

        if (currentMidPrice <= 0.0) {
            // Fallback: If no market data yet, skip sending limit order or use a default.
            // For this mock, we'll just skip to avoid sending bad prices.
            // In a real system, you might queue it or use a default price.
            std::cerr << "MockTradeClient Warning: No valid mid-price for " << selectedSymbol << ". Skipping limit order." << std::endl;
            return;
        }

        // Generate a price around the mid-price using the fluctuation distribution
        double price = currentMidPrice + m_priceFluctuationDist(m_randGen);
        price = std::round(price * 100.0) / 100.0; // Round to 2 decimal places for realism

        // Ensure bid/ask makes sense relative to mid-price and spread
        // If it's a BUY limit, it should ideally be at or below current ask.
        // If it's a SELL limit, it should ideally be at or above current bid.
        // The +/- 0.5 from mid will likely make it marketable for now.
        // More sophisticated logic would consider current bid/ask and tick size.

        newOrderSingle.set(FIX::Price(price));
    }

    try {
        FIX::Session::sendToTarget(newOrderSingle, m_sessionID);

        FIX::ClOrdID sentClOrdID;
        FIX::Symbol sentSymbol;
        FIX::Side sentSide;
        FIX::OrderQty sentOrderQty;
        FIX::OrdType sentOrdType;
        FIX::Price sentPrice;

        newOrderSingle.get(sentClOrdID);
        newOrderSingle.get(sentSymbol);
        newOrderSingle.get(sentSide);
        newOrderSingle.get(sentOrderQty);
        newOrderSingle.get(sentOrdType);

        std::cout << "MockTradeClient: Sent NewOrderSingle - ClOrdID: " << sentClOrdID.getValue()
                  << ", Symbol: " << sentSymbol.getValue()
                  << ", Side: " << sentSide.getValue()
                  << ", Qty: " << static_cast<int>(sentOrderQty.getValue())
                  << ", OrdType: " << sentOrdType.getValue();

        if (newOrderSingle.isSetField(FIX::FIELD::Price)) {
            newOrderSingle.get(sentPrice);
            std::cout << ", Price: " << sentPrice.getValue();
        }
        std::cout << std::endl;
    } catch (const FIX::SessionNotFound& e) {
        std::cerr << "MockTradeClient Error: Session not found when sending order: " << e.what() << std::endl;
    }
}

void MockTradeClient::startSendingOrders() {
    FIX::Locker locker(m_mutex);
    if (!m_running) {
        m_running = true;
        m_orderSendingThread = std::thread([this]() {
            while (m_running) {
                sendNewOrderSingle();
                std::this_thread::sleep_for(std::chrono::milliseconds(m_transactTimeDist(m_randGen)));
            }
        });
    }
}

void MockTradeClient::stopSendingOrders() {
    FIX::Locker locker(m_mutex);
    if (m_running) {
        m_running = false;
        if (m_orderSendingThread.joinable()) {
            m_orderSendingThread.join();
        }
    }
}
