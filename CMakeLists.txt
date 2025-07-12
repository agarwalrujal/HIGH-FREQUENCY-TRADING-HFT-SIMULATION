cmake_minimum_required(VERSION 3.10)
project(MarketMakerSystem)

# Explicitly set architecture for macOS to arm64
set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "Build architectures for macOS")


# Find QuickFIX library and include path
find_library(QUICKFIX_LIBRARY quickfix PATHS /usr/local/lib REQUIRED)
find_path(QUICKFIX_INCLUDE_DIR quickfix/Application.h PATHS /usr/local/include REQUIRED)

# --- DEBUGGING LINES START ---
message(STATUS "DEBUG: QUICKFIX_LIBRARY found at: ${QUICKFIX_LIBRARY}")
message(STATUS "DEBUG: QUICKFIX_INCLUDE_DIR found at: ${QUICKFIX_INCLUDE_DIR}")
# --- DEBUGGING LINES END ---

# Keep this for your project's own headers
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# Define source files for the Market Maker executable
set(MARKET_MAKER_SRCS
    src/main_market_maker.cpp
    src/MarketMakerApp.cpp
    src/MarketDataProcessor.cpp
    src/MockMarketDataSource.cpp
    src/OrderBook.cpp
    src/StrategyEngine.cpp
)

# Add the Market Maker executable
add_executable(market_maker ${MARKET_MAKER_SRCS})

# Link Market Maker executable with QuickFIX library
target_link_libraries(market_maker ${QUICKFIX_LIBRARY})

# Explicitly include QuickFIX headers for this target
target_include_directories(market_maker PRIVATE ${QUICKFIX_INCLUDE_DIR})


# Define source files for the Mock Trade Client executable
set(MOCK_CLIENT_SRCS
    src/main_mock_client.cpp
    src/MockTradeClient.cpp
)

# Add the Mock Trade Client executable
add_executable(mock_client ${MOCK_CLIENT_SRCS})

# Link Mock Trade Client executable with QuickFIX library
target_link_libraries(mock_client ${QUICKFIX_LIBRARY})

# Explicitly include QuickFIX headers for this target
target_include_directories(mock_client PRIVATE ${QUICKFIX_INCLUDE_DIR})