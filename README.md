# HIGH-FREQUENCY-TRADING-HFT-SIMULATION
• Architected and developed a comprehensive HFT simulation platform in C++, integrating a Mock Market Data
Source, Market Data Processor, Strategy Engine, Market Maker application, and Mock Trade Client.

• Engineered a low-latency Market Data Processor that integrated QuickFIX protocol for efficient market data
ingestion, enabling real-time management of an optimized, in-memory Order Book for bid/ask spread and midprice calculations.
+--------------------+     FIX Protocol      +----------------------+
|  MockTradeClient   |  <------------------> |  MarketMakerApp (FIX)|
+--------------------+                       +----------------------+
                                                    |
                                                    ▼
                                        +----------------------+
                                        |  StrategyEngine      |
                                        | (Order Matching +    |
                                        |   Quote Generation)  |
                                        +----------------------+
                                                    |
                                                    ▼
                                        +----------------------+
                                        |  OrderBook (Prices)  |
                                        +----------------------+


HFT-MarketMaker-Simulation/
│
├── src/
│   ├── StrategyEngine.cpp / .h      # Core quoting/matching logic
│   ├── MarketMakerApp.cpp / .h      # FIX Application for MM
│   ├── OrderBook.cpp / .h           # Live market prices
│   ├── MockTradeClient.cpp / .h     # Mock FIX Client
│   ├── main_market_maker.cpp        # Entry point for MM
│   └── main_mock_client.cpp         # Entry point for client
│
├── config/
│   ├── MarketMaker.cfg              # FIX config for MM
│   └── MockClient.cfg               # FIX config for client
│
├── CMakeLists.txt
└── README.md


🏁 Getting Started
✅ Prerequisites
C++17 or later

QuickFIX

CMake

🔨 Build
mkdir build && cd build
cmake ..
make

This builds two executables:
market_maker
mock_client

# Terminal 1 - Run Market Maker
./build/market_maker config/MarketMaker.cfg

# Terminal 2 - Run Mock Client
./build/mock_client config/MockClient.cfg

Market Making Logic
The StrategyEngine periodically:

-> Calculates mid-price from OrderBook
-> Places a bid and ask around it
-> Sends these as NewOrderSingle via FIX
-> Cancels previous quotes before placing new ones

Client orders are received via:
fromApp() → crack() → onMessage() → StrategyEngine::onNewOrderSingle()

The engine matches client orders only if they cross the MM's quote:
if (clientOrderPrice >= mmAsk) → Fill BUY order
if (clientOrderPrice <= mmBid) → Fill SELL order



Technologies Used:
-> C++17
-> QuickFIX
-> CMake
-> Multithreading (std::thread)
-> Synchronization (FIX::Mutex, std::lock_guard)

