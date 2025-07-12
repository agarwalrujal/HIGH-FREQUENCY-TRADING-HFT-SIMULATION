// src/StrategyEngine.cpp
#include "StrategyEngine.h"
#include "OrderBook.h"
#include "MarketMakerApp.h" // Include to access MarketMakerApplication's methods
#include <quickfix/Session.h>
#include <quickfix/FieldConvertors.h> // For FIX::UtcTimeStamp
#include <quickfix/FixFields.h> // Ensure this is included for various FIX fields like LastQty, LastPx
#include <iomanip> // For std::fixed, std::setprecision

StrategyEngine::StrategyEngine(OrderBook* orderBook, MarketMakerApplication* mmApp)
    : m_orderBook(orderBook), m_mmApp(mmApp), m_quotingRunning(false),
      m_randGen(std::chrono::system_clock::now().time_since_epoch().count()),
      m_qtyDist(100, 500)
{}

StrategyEngine::~StrategyEngine() {
    stopQuoting();
}

std::string StrategyEngine::generateNewClOrdID() {
    static long long i = 0;
    return "MM-QUOTE-" + std::to_string(++i);
}

void StrategyEngine::startQuoting() {
    FIX::Locker locker(m_mutex);
    if (!m_quotingRunning) {
        m_quotingRunning = true;
        m_quotingThread = std::thread([this]() {
            while (m_quotingRunning) {
                // Check if the MarketMakerApp has an active client session
                if (m_mmApp && FIX::Session::doesSessionExist(m_mmApp->getClientSessionID()) &&
                    FIX::Session::lookupSession(m_mmApp->getClientSessionID())->isLoggedOn()) {
                    m_clientSessionID = m_mmApp->getClientSessionID(); // Cache client session ID
                    manageQuotes(); // Execute the quoting logic
                } else {
                    // std::cout << "StrategyEngine: Waiting for MarketMakerApp to have a client session..." << std::endl;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(3000)); // Quote every 3 seconds
            }
        });
    }
}

void StrategyEngine::stopQuoting() {
    FIX::Locker locker(m_mutex);
    if (m_quotingRunning) {
        m_quotingRunning = false;
        if (m_quotingThread.joinable()) {
            m_quotingThread.join();
        }
    }
}

void StrategyEngine::manageQuotes() {
    double midPrice = m_orderBook->getMidPrice("AAPL");
    if (midPrice == 0.0) {
        return;
    }

    double spread = 0.04; // Desired spread (e.g., 4 cents)
    double bidPrice = midPrice - (spread / 2.0);
    double askPrice = midPrice + (spread / 2.0);
    int quoteQuantity = m_qtyDist(m_randGen);

    bidPrice = std::round(bidPrice * 100.0) / 100.0;
    askPrice = std::round(askPrice * 100.0) / 100.0;

    std::cout << "StrategyEngine: My current desired quotes for AAPL: BID "
              << std::fixed << std::setprecision(2) << bidPrice
              << " x " << quoteQuantity << " | ASK "
              << std::fixed << std::setprecision(2) << askPrice
              << " x " << quoteQuantity << std::endl;
}

void StrategyEngine::onNewOrderSingle(const FIX42::NewOrderSingle& message, const FIX::SessionID& clientSessionID) {
    FIX::ClOrdID clOrdID;
    FIX::Symbol symbol;
    FIX::Side side;
    FIX::OrderQty orderQty;
    FIX::OrdType ordType;
    FIX::Price price;

    message.get(clOrdID);
    message.get(symbol);
    message.get(side);
    message.get(orderQty);
    message.get(ordType);

    std::cout << "\nStrategyEngine: Received Client Order - ClOrdID: " << clOrdID.getValue()
              << ", Symbol: " << symbol.getValue()
              << ", Side: " << (side == FIX::Side_BUY ? "BUY" : "SELL")
              << ", Qty: " << orderQty.getValue();

    if (message.isSetField(FIX::FIELD::Price)) {
        message.get(price);
        std::cout << ", Price: " << price.getValue();
    }
    std::cout << ", OrdType: " << ordType.getValue() << std::endl;

    double bestBid = m_orderBook->getBestBid(symbol.getValue());
    double bestAsk = m_orderBook->getBestAsk(symbol.getValue());
    double midPrice = m_orderBook->getMidPrice(symbol.getValue());

    FIX::ExecType execType = FIX::ExecType_FILL;
    FIX::OrdStatus ordStatus = FIX::OrdStatus_FILLED;
    std::string rejectReason = "";
    double fillPrice = 0.0;

    if (midPrice == 0.0 || bestBid == 0.0 || bestAsk == 0.0) {
        execType = FIX::ExecType_REJECTED;
        ordStatus = FIX::OrdStatus_REJECTED;
        rejectReason = "No valid market data available for matching.";
        std::cerr << "StrategyEngine: Rejecting " << clOrdID.getValue() << ": " << rejectReason << std::endl;
    } else {
        if (ordType == FIX::OrdType_MARKET) {
            if (side == FIX::Side_BUY) {
                fillPrice = bestAsk;
            } else {
                fillPrice = bestBid;
            }
            std::cout << "StrategyEngine: Filling market order " << clOrdID.getValue() << " at " << fillPrice << std::endl;

        } else if (ordType == FIX::OrdType_LIMIT) {
            message.get(price);
            bool matched = false;
            if (side == FIX::Side_BUY && price.getValue() >= bestAsk && bestAsk != 0.0) {
                fillPrice = bestAsk;
                matched = true;
            } else if (side == FIX::Side_SELL && price.getValue() <= bestBid && bestBid != 0.0) {
                fillPrice = bestBid;
                matched = true;
            }

            if (matched) {
                std::cout << "StrategyEngine: Filling limit order " << clOrdID.getValue() << " at " << fillPrice << std::endl;
            } else {
                execType = FIX::ExecType_REJECTED;
                ordStatus = FIX::OrdStatus_REJECTED;
                rejectReason = "Limit order not immediately marketable against current book.";
                std::cerr << "StrategyEngine: Rejecting " << clOrdID.getValue() << ": " << rejectReason << std::endl;
            }
        }
    }

    // Prepare and send ExecutionReport via MarketMakerApplication
    // --- START OF CRITICAL FIXES FOR EXECUTIONREPORT CONSTRUCTOR & AMBIGUITY ---
    FIX42::ExecutionReport execReport(
        FIX::OrderID("MM-ORD-" + clOrdID.getValue()),
        FIX::ExecID("MM-EXEC-" + clOrdID.getValue()),
        FIX::ExecTransType_NEW, // **FIX**: This argument was missing
        execType,
        ordStatus,
        symbol,                 // **FIX**: This argument was missing
        side,
        // **FIX**: Explicitly cast the quantity values to int to resolve ambiguity
        FIX::LeavesQty(ordStatus == FIX::OrdStatus_FILLED ? 0 : static_cast<int>(orderQty.getValue())),
        FIX::CumQty(ordStatus == FIX::OrdStatus_FILLED ? static_cast<int>(orderQty.getValue()) : 0),
        FIX::AvgPx(fillPrice)
    );
    // --- END OF CRITICAL FIXES FOR EXECUTIONREPORT CONSTRUCTOR & AMBIGUITY ---

    execReport.set(clOrdID); // Client's original Order ID
    // Note: symbol and orderQty are now passed directly to the constructor,
    // so `execReport.set(symbol);` and `execReport.set(orderQty);` might be redundant here,
    // but they don't cause harm. Leaving them commented out for clarity.
    // execReport.set(symbol);
    // execReport.set(orderQty);

    // Set LastQty and LastPx based on fill status
    if (ordStatus == FIX::OrdStatus_FILLED) {
            // execReport.set(FIX::LastQty(static_cast<int>(orderQty.getValue()))); // Original line causing error
            // execReport.set(FIX::LastPx(fillPrice)); // Original line causing error

            // **FIX:** Use the generic setField method for these fields.
            // This explicitly puts the field into the message's field map.
            execReport.setField(FIX::LastQty(static_cast<int>(orderQty.getValue())));
            execReport.setField(FIX::LastPx(fillPrice));
        } else {
            execReport.setField(FIX::LastQty(0)); // No quantity filled in this specific report
            execReport.setField(FIX::LastPx(0.0)); // No fill price if not filled
        }

    // **FIX**: Use UtcTimeStamp::now() for current time (addresses deprecation warning)
    execReport.set(FIX::TransactTime(FIX::UtcTimeStamp::now()));
    if (!rejectReason.empty()) {
        execReport.set(FIX::Text(rejectReason));
    }

    if (m_mmApp) {
        // Pass the execReport by non-const reference as required by MarketMakerApp::sendExecutionReportToClient
        m_mmApp->sendExecutionReportToClient(execReport, clientSessionID);
    }
}

void StrategyEngine::onOurOwnExecutionReport(const FIX42::ExecutionReport& message) {
    FIX::ClOrdID clOrdID;
    FIX::OrdStatus ordStatus;
    try {
        message.get(clOrdID);
        message.get(ordStatus);
        FIX::Locker locker(m_mutex);
        if (m_ourOpenQuotes.count(clOrdID.getValue())) {
            std::cout << "StrategyEngine: Our internal quote " << clOrdID.getValue()
                      << " status changed to: " << ordStatus.getValue() << std::endl;
            if (ordStatus == FIX::OrdStatus_FILLED || ordStatus == FIX::OrdStatus_CANCELED) {
                m_ourOpenQuotes.erase(clOrdID.getValue());
            }
        }
    } catch (const FIX::FieldNotFound& e) {
        std::cerr << "StrategyEngine: Field not found in our own ER: " << e.what() << std::endl;
    }
}
