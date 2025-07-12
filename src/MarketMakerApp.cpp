#include "MarketMakerApp.h"
#include "OrderBook.h"
#include "StrategyEngine.h"
#include <quickfix/Session.h>
#include <quickfix/FieldConvertors.h>
// Ensure these are included if you use them directly, though often
// they are transitively included by message headers.
#include <quickfix/fix42/NewOrderSingle.h>
#include <quickfix/fix42/ExecutionReport.h>


MarketMakerApplication::MarketMakerApplication(OrderBook* orderBook, StrategyEngine* strategyEngine)
    : m_orderBook(orderBook), m_strategyEngine(strategyEngine)
{}

void MarketMakerApplication::onCreate(const FIX::SessionID& sessionID) {
    std::cout << "MarketMakerApp onCreate: " << sessionID << std::endl;
}

void MarketMakerApplication::onLogon(const FIX::SessionID& sessionID) {
    std::cout << "MarketMakerApp onLogon: " << sessionID << std::endl;
    m_clientSessionID = sessionID; // Store client session ID
    if (m_strategyEngine) {
        m_strategyEngine->startQuoting(); // Start market making logic when client connects
    }
}

void MarketMakerApplication::onLogout(const FIX::SessionID& sessionID) {
    std::cout << "MarketMakerApp onLogout: " << sessionID << std::endl;
    if (m_strategyEngine) {
        m_strategyEngine->stopQuoting(); // Stop market making when client disconnects
    }
    m_clientSessionID = FIX::SessionID(); // Clear session ID
}

void MarketMakerApplication::toAdmin(FIX::Message& message, const FIX::SessionID& sessionID) {
    // std::cout << "MarketMakerApp toAdmin: " << message << std::endl;
}

void MarketMakerApplication::toApp(FIX::Message& message, const FIX::SessionID& sessionID) throw(FIX::DoNotSend) {
    // This callback is invoked before a message is sent to the counterparty.
    // Use this to log outgoing messages or add/modify fields before sending.
    // std::cout << "MarketMakerApp toApp: " << message << std::endl;
}

void MarketMakerApplication::fromAdmin(const FIX::Message& message, const FIX::SessionID& sessionID) throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::RejectLogon) {
    // std::cout << "MarketMakerApp fromAdmin: " << message << std::endl;
}

void MarketMakerApplication::fromApp(const FIX::Message& message, const FIX::SessionID& sessionID) throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::UnsupportedMessageType) {
    // This is the entry point for all incoming application messages from the client
    crack(message, sessionID); // Dispatches to the appropriate onMessage handler
}

// --- Specific Client Order Handlers ---

void MarketMakerApplication::onMessage(const FIX42::NewOrderSingle& message, const FIX::SessionID& sessionID) {
    // MarketMakerApp receives client order and forwards it to StrategyEngine
    if (m_strategyEngine) {
        m_strategyEngine->onNewOrderSingle(message, sessionID);
    } else {
        std::cerr << "MarketMakerApp: No StrategyEngine hooked up to handle NewOrderSingle. Rejecting order." << std::endl;

        // Declare local QuickFIX field variables to hold the extracted values
        FIX::ClOrdID clOrdID;
        FIX::Side side;
        FIX::OrderQty orderQty;
        FIX::Symbol symbol; // *** FIX: Added symbol declaration ***

        // Populate the field objects using message.get()
        message.get(clOrdID);
        message.get(side);
        message.get(orderQty);
        message.get(symbol); // *** FIX: Populating symbol ***

        // *** FIX: Added FIX::ExecTransType_NEW to the constructor call ***
        FIX42::ExecutionReport rejectReport(
            FIX::OrderID("MM-REJECT-" + clOrdID.getValue()),
            FIX::ExecID("MM-REJECT-EXEC-" + clOrdID.getValue()),
            FIX::ExecTransType_NEW, // *** FIX: Missing argument for constructor ***
            FIX::ExecType_REJECTED,
            FIX::OrdStatus_REJECTED,
            symbol,         // *** FIX: Added symbol to constructor ***
            side,
            FIX::LeavesQty(orderQty.getValue()),
            FIX::CumQty(0),
            FIX::AvgPx(0)
        );
        // No need to set clOrdID, symbol, orderQty again if already in constructor,
        // but it doesn't hurt. I'll comment out the duplicates.
        // rejectReport.set(clOrdID);
        // rejectReport.set(symbol);
        // rejectReport.set(orderQty);

        // *** FIX: Changed UtcTimeStamp() to UtcTimeStamp::now() ***
        rejectReport.set(FIX::TransactTime(FIX::UtcTimeStamp::now()));
        rejectReport.set(FIX::Text("No active strategy engine to process orders."));

        // *** FIX: Changed parameter type from const FIX42::ExecutionReport& to FIX42::ExecutionReport ***
        // This will allow sendExecutionReportToClient to modify the message
        // before sending (which QuickFIX requires).
        sendExecutionReportToClient(rejectReport, sessionID);
    }
}

// --- Public methods for StrategyEngine to send messages through MarketMakerApp ---

// *** FIX: Changed the message parameter from const FIX42::ExecutionReport& to FIX42::ExecutionReport& ***
void MarketMakerApplication::sendExecutionReportToClient(FIX42::ExecutionReport& message, const FIX::SessionID& clientSessionID) {
    try {
        FIX::Session::sendToTarget(message, clientSessionID);
        // It's good practice to get the ClOrdID after sending, in case QuickFIX
        // internally modifies the message and adds it if it was missing or changes it.
        // However, in this case, we populate it ourselves.
        FIX::ClOrdID clOrdID;
        message.get(clOrdID); // Safely retrieve ClOrdID after sendToTarget
        std::cout << "MarketMakerApp: Sent ExecutionReport (from StrategyEngine) for ClOrdID: "
                  << clOrdID.getValue() << " to client." << std::endl;
    } catch (const FIX::SessionNotFound& e) {
        std::cerr << "MarketMakerApp Error sending ER to client: Session Not Found - " << e.what() << std::endl;
    } catch (const FIX::FieldNotFound& e) {
        std::cerr << "MarketMakerApp Error sending ER to client: Field not found - " << e.what() << std::endl;
    } catch (const std::exception& e) { // Catch any other standard exceptions
        std::cerr << "MarketMakerApp Error sending ER to client: " << e.what() << std::endl;
    }
}

FIX::SessionID MarketMakerApplication::getClientSessionID() const {
    return m_clientSessionID;
}
