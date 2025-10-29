#include "test.h"
#include "fl/spi.h"
#include "fl/spi/impl.h"
#include "platforms/shared/spi_types.h"
#include "platforms/shared/spi_bus_manager.h"

using namespace fl::spi;
using fl::SPIError;
using fl::DMABuffer;

// ============================================================================
// Test Fixture - Reset SPIBusManager before each test
// ============================================================================
// This fixture ensures test isolation by resetting the global SPIBusManager
// singleton before each test case. Without this, devices accumulate across
// tests and can hit the max device limit (8 per clock pin).
struct SPITestFixture {
    SPITestFixture() {
        // Reset the bus manager before each test
        fl::getSPIBusManager().reset();
    }

    ~SPITestFixture() {
        // Also reset after each test for cleanliness
        fl::getSPIBusManager().reset();
    }
};

// ============================================================================
// Result<T> Tests
// ============================================================================

TEST_CASE("Result<void> basic operations") {
    SUBCASE("Default construction creates failure state") {
        Result<void> r;
        CHECK(!r.ok());
        CHECK(r.error() == SPIError::NOT_INITIALIZED);
    }

    SUBCASE("Success creation") {
        Result<void> r = Result<void>::success();
        CHECK(r.ok());
        CHECK(static_cast<bool>(r));  // Explicit bool conversion
    }

    SUBCASE("Failure creation with error code") {
        Result<void> r = Result<void>::failure(SPIError::BUFFER_TOO_LARGE);
        CHECK(!r.ok());
        CHECK(r.error() == SPIError::BUFFER_TOO_LARGE);
    }

    SUBCASE("Failure creation with error code and message") {
        Result<void> r = Result<void>::failure(SPIError::ALLOCATION_FAILED, "Out of memory");
        CHECK(!r.ok());
        CHECK(r.error() == SPIError::ALLOCATION_FAILED);
        CHECK(r.message() != nullptr);
        CHECK(fl::strcmp(r.message(), "Out of memory") == 0);
    }
}

TEST_CASE("Result<int> value type operations") {
    SUBCASE("Success with value") {
        Result<int> r = Result<int>::success(42);
        CHECK(r.ok());
        CHECK(r.value() == 42);
    }

    SUBCASE("Failure with no value") {
        Result<int> r = Result<int>::failure(SPIError::BUSY, "Device busy");
        CHECK(!r.ok());
        CHECK(r.error() == SPIError::BUSY);
        CHECK(r.message() != nullptr);
    }

    SUBCASE("Value modification") {
        Result<int> r = Result<int>::success(10);
        r.value() = 20;
        CHECK(r.value() == 20);
    }
}

TEST_CASE("Result<Transaction> example usage") {
    // This tests that Result works with complex types
    // (Transaction will be properly implemented in later iterations)

    SUBCASE("Failure case") {
        Result<int> r = Result<int>::failure(SPIError::NOT_INITIALIZED);
        CHECK(!r.ok());
        CHECK(!static_cast<bool>(r));  // Bool conversion
    }
}

// ============================================================================
// Config Tests
// ============================================================================

TEST_CASE("Config construction") {
    SUBCASE("Basic construction with default values") {
        Config cfg(18, 23);
        CHECK(cfg.clock_pin == 18);
        CHECK(cfg.data_pins.size() == 1);
        CHECK(cfg.data_pins[0] == 23);
        CHECK(cfg.clock_speed_hz == 0xffffffff);  // As fast as possible
        CHECK(cfg.spi_mode == 0);
    }

    SUBCASE("Configuration modification") {
        Config cfg(5, 6);
        cfg.clock_speed_hz = 20000000;  // 20 MHz
        cfg.spi_mode = 1;

        CHECK(cfg.clock_speed_hz == 20000000);
        CHECK(cfg.spi_mode == 1);
    }
}

// ============================================================================
// Device Construction Tests (Basic)
// ============================================================================

TEST_CASE_FIXTURE(SPITestFixture, "Device construction (basic)") {
    SUBCASE("Device can be constructed") {
        Config cfg(18, 23);
        Device spi(cfg);

        // Device should not be ready until begin() is called
        CHECK(!spi.isReady());
    }

    SUBCASE("Device configuration access") {
        Config cfg(5, 6);
        cfg.clock_speed_hz = 15000000;
        Device spi(cfg);

        const Config& stored_cfg = spi.getConfig();
        CHECK(stored_cfg.clock_pin == 5);
        CHECK(stored_cfg.data_pins.size() == 1);
        CHECK(stored_cfg.data_pins[0] == 6);
        CHECK(stored_cfg.clock_speed_hz == 15000000);
    }
}

// ============================================================================
// Device Initialization Tests (Iteration 2)
// ============================================================================

TEST_CASE_FIXTURE(SPITestFixture, "Device initialization with begin()") {
    SUBCASE("Device starts not ready") {
        Config cfg(18, 23);
        Device spi(cfg);
        CHECK(!spi.isReady());
    }

    SUBCASE("begin() initializes device") {
        Config cfg(18, 23);
        Device spi(cfg);

        fl::optional<fl::Error> result = spi.begin();
        CHECK(!result);  // No error means success
        CHECK(spi.isReady());
    }

    SUBCASE("Double begin() is idempotent") {
        Config cfg(18, 23);
        Device spi(cfg);

        fl::optional<fl::Error> result1 = spi.begin();
        CHECK(!result1);  // No error means success
        CHECK(spi.isReady());

        // Second begin() should also succeed
        fl::optional<fl::Error> result2 = spi.begin();
        CHECK(!result2);  // No error means success
        CHECK(spi.isReady());
    }

    SUBCASE("end() shuts down device") {
        Config cfg(18, 23);
        Device spi(cfg);

        spi.begin();
        CHECK(spi.isReady());

        spi.end();
        CHECK(!spi.isReady());
    }

    SUBCASE("Multiple begin/end cycles") {
        Config cfg(18, 23);
        Device spi(cfg);

        // First cycle
        spi.begin();
        CHECK(spi.isReady());
        spi.end();
        CHECK(!spi.isReady());

        // Second cycle
        fl::optional<fl::Error> result = spi.begin();
        CHECK(!result);  // No error means success
        CHECK(spi.isReady());
        spi.end();
        CHECK(!spi.isReady());
    }

    // NOTE: write(), read(), transfer() methods are not implemented in Device class
    // Device only provides writeAsync() and zero-copy DMA API
    // SUBCASE("Operations fail when not initialized") {
    //     Config cfg(18, 23);
    //     Device spi(cfg);
    //
    //     // Try operations without calling begin()
    //     uint8_t data[4] = {1, 2, 3, 4};
    //     uint8_t buffer[4] = {0};
    //
    //     Result<void> write_result = spi.write(data, 4);
    //     CHECK(!write_result.ok());
    //     CHECK(write_result.error() == SPIError::NOT_INITIALIZED);
    //
    //     Result<void> read_result = spi.read(buffer, 4);
    //     CHECK(!read_result.ok());
    //     CHECK(read_result.error() == SPIError::NOT_INITIALIZED);
    //
    //     Result<void> transfer_result = spi.transfer(data, buffer, 4);
    //     CHECK(!transfer_result.ok());
    //     CHECK(transfer_result.error() == SPIError::NOT_INITIALIZED);
    // }
}

TEST_CASE_FIXTURE(SPITestFixture, "Device destructor cleanup") {
    SUBCASE("Destructor cleans up initialized device") {
        Config cfg(18, 23);
        {
            Device spi(cfg);
            spi.begin();
            CHECK(spi.isReady());
            // Destructor should be called here
        }
        // If we get here without crashes, RAII cleanup worked
        CHECK(true);
    }

    SUBCASE("Destructor handles uninitialized device") {
        Config cfg(18, 23);
        {
            Device spi(cfg);
            // Never call begin()
            CHECK(!spi.isReady());
            // Destructor should handle this gracefully
        }
        CHECK(true);
    }
}

TEST_CASE_FIXTURE(SPITestFixture, "Device state transitions") {
    Config cfg(18, 23);
    Device spi(cfg);

    SUBCASE("Initial state") {
        CHECK(!spi.isReady());
        CHECK(!spi.isBusy());
    }

    SUBCASE("After begin()") {
        spi.begin();
        CHECK(spi.isReady());
        CHECK(!spi.isBusy());
    }

    SUBCASE("After end()") {
        spi.begin();
        spi.end();
        CHECK(!spi.isReady());
        CHECK(!spi.isBusy());
    }
}

TEST_CASE_FIXTURE(SPITestFixture, "Device configuration updates") {
    SUBCASE("Clock speed can be updated") {
        Config cfg(18, 23);
        cfg.clock_speed_hz = 10000000;
        Device spi(cfg);

        fl::optional<fl::Error> result = spi.setClockSpeed(20000000);
        CHECK(!result);  // No error means success

        const Config& stored = spi.getConfig();
        CHECK(stored.clock_speed_hz == 20000000);
    }

    SUBCASE("Configuration persists after begin()") {
        Config cfg(18, 23);
        cfg.clock_speed_hz = 15000000;
        cfg.spi_mode = 2;

        Device spi(cfg);
        spi.begin();

        const Config& stored = spi.getConfig();
        CHECK(stored.clock_pin == 18);
        CHECK(stored.data_pins.size() == 1);
        CHECK(stored.data_pins[0] == 23);
        CHECK(stored.clock_speed_hz == 15000000);
        CHECK(stored.spi_mode == 2);
    }
}

TEST_CASE_FIXTURE(SPITestFixture, "Multiple devices on different pins") {
    SUBCASE("Two devices can coexist") {
        // Use different clock pins to avoid conflicts with previous tests
        // Note: SPIBusManager is a global singleton and not reset between tests
        Config cfg1(10, 11);
        Config cfg2(12, 13);

        Device spi1(cfg1);
        Device spi2(cfg2);

        fl::optional<fl::Error> result1 = spi1.begin();
        fl::optional<fl::Error> result2 = spi2.begin();

        CHECK(!result1);  // No error means success
        CHECK(!result2);  // No error means success
        CHECK(spi1.isReady());
        CHECK(spi2.isReady());

        // Cleanup
        spi1.end();
        spi2.end();
    }
}

// ============================================================================
// Device Write Operations Tests (Iteration 3)
// ============================================================================
// NOTE: Device class does not have write(), read(), transfer() methods
// It only provides writeAsync() and zero-copy DMA API (acquireBuffer/transmit)
// These tests are commented out

/*
TEST_CASE_FIXTURE(SPITestFixture, "Device write operations") {
    SUBCASE("Basic write succeeds") {
        Config cfg(14, 15);
        Device spi(cfg);

        Result<void> begin_result = spi.begin();
        REQUIRE(begin_result.ok());

        uint8_t data[4] = {0xAA, 0xBB, 0xCC, 0xDD};
        Result<void> write_result = spi.write(data, 4);

        CHECK(write_result.ok());
        CHECK(!spi.isBusy());  // Should be complete after blocking write

        spi.end();
    }

    SUBCASE("Write fails without begin()") {
        Config cfg(14, 15);
        Device spi(cfg);

        uint8_t data[4] = {1, 2, 3, 4};
        Result<void> result = spi.write(data, 4);

        CHECK(!result.ok());
        CHECK(result.error() == SPIError::NOT_INITIALIZED);
    }

    SUBCASE("Write fails with null data") {
        Config cfg(14, 15);
        Device spi(cfg);
        spi.begin();

        Result<void> result = spi.write(nullptr, 4);

        CHECK(!result.ok());
        CHECK(result.error() == SPIError::ALLOCATION_FAILED);

        spi.end();
    }

    SUBCASE("Write fails with zero size") {
        Config cfg(14, 15);
        Device spi(cfg);
        spi.begin();

        uint8_t data[4] = {1, 2, 3, 4};
        Result<void> result = spi.write(data, 0);

        CHECK(!result.ok());
        CHECK(result.error() == SPIError::ALLOCATION_FAILED);

        spi.end();
    }

    SUBCASE("Multiple writes work correctly") {
        Config cfg(14, 15);
        Device spi(cfg);
        spi.begin();

        uint8_t data1[4] = {0x11, 0x22, 0x33, 0x44};
        uint8_t data2[8] = {0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC};

        Result<void> result1 = spi.write(data1, 4);
        CHECK(result1.ok());

        Result<void> result2 = spi.write(data2, 8);
        CHECK(result2.ok());

        spi.end();
    }

    SUBCASE("Write with varying buffer sizes") {
        Config cfg(14, 15);
        Device spi(cfg);
        spi.begin();

        // Small buffer
        uint8_t small[2] = {0x01, 0x02};
        Result<void> result_small = spi.write(small, 2);
        CHECK(result_small.ok());

        // Medium buffer
        uint8_t medium[64];
        for (size_t i = 0; i < 64; i++) medium[i] = static_cast<uint8_t>(i);
        Result<void> result_medium = spi.write(medium, 64);
        CHECK(result_medium.ok());

        // Large buffer
        uint8_t large[256];
        for (size_t i = 0; i < 256; i++) large[i] = static_cast<uint8_t>(i);
        Result<void> result_large = spi.write(large, 256);
        CHECK(result_large.ok());

        spi.end();
    }
}
*/

TEST_CASE_FIXTURE(SPITestFixture, "Device buffer acquisition") {
    SUBCASE("acquireBuffer returns valid buffer") {
        Config cfg(16, 17);
        Device spi(cfg);
        spi.begin();

        DMABuffer buffer = spi.acquireBuffer(64);

        CHECK(buffer.ok());
        CHECK(buffer.size() == 64);

        spi.end();
    }

    SUBCASE("acquireBuffer fails without begin()") {
        Config cfg(16, 17);
        Device spi(cfg);

        DMABuffer buffer = spi.acquireBuffer(64);

        CHECK(!buffer.ok());
        CHECK(buffer.error() == SPIError::NOT_INITIALIZED);
    }

    SUBCASE("Multiple buffer acquisitions work") {
        Config cfg(16, 17);
        Device spi(cfg);
        spi.begin();

        DMABuffer buffer1 = spi.acquireBuffer(32);
        CHECK(buffer1.ok());

        DMABuffer buffer2 = spi.acquireBuffer(64);
        CHECK(buffer2.ok());

        spi.end();
    }
}

TEST_CASE_FIXTURE(SPITestFixture, "Device transmit operations") {
    SUBCASE("Transmit with valid buffer succeeds (blocking)") {
        Config cfg(16, 17);
        Device spi(cfg);
        spi.begin();

        DMABuffer buffer = spi.acquireBuffer(16);
        REQUIRE(buffer.ok());

        // Fill buffer with test data
        fl::span<uint8_t> data = buffer.data();
        for (size_t i = 0; i < data.size(); i++) {
            data[i] = static_cast<uint8_t>(i);
        }

        fl::optional<fl::Error> result = spi.transmit(buffer, false);  // Blocking
        CHECK(!result);  // No error means success
        CHECK(!spi.isBusy());

        spi.end();
    }

    SUBCASE("Transmit fails without begin()") {
        Config cfg(16, 17);
        Device spi(cfg);

        DMABuffer buffer(64);
        fl::optional<fl::Error> result = spi.transmit(buffer, false);

        CHECK(result.has_value());  // Error present means failure
    }

    SUBCASE("Transmit with invalid buffer fails") {
        Config cfg(16, 17);
        Device spi(cfg);
        spi.begin();

        DMABuffer invalid_buffer(SPIError::ALLOCATION_FAILED);
        fl::optional<fl::Error> result = spi.transmit(invalid_buffer, false);

        CHECK(result.has_value());  // Error present means failure

        spi.end();
    }
}

TEST_CASE_FIXTURE(SPITestFixture, "Device busy state and waitComplete") {
    SUBCASE("Device not busy after initialization") {
        Config cfg(18, 19);
        Device spi(cfg);
        spi.begin();

        CHECK(!spi.isBusy());

        spi.end();
    }

    SUBCASE("Device not busy after blocking write") {
        Config cfg(18, 19);
        Device spi(cfg);
        spi.begin();

        uint8_t data[4] = {1, 2, 3, 4};
        Result<Transaction> result = spi.writeAsync(data, 4);
        CHECK(result.ok());
        Transaction txn = fl::move(result.value());
        CHECK(txn.wait());  // Wait for completion (blocking)
        CHECK(!spi.isBusy());

        spi.end();
    }

    SUBCASE("waitComplete returns true when not busy") {
        Config cfg(18, 19);
        Device spi(cfg);
        spi.begin();

        bool completed = spi.waitComplete(1000);
        CHECK(completed);

        spi.end();
    }
}

// Note: Buffer caching is handled internally by the hardware controller (SpiHw1/2/4/8),
// not at the Device level. This simplifies the API and ensures proper synchronization.

// ============================================================================
// Device Read Operations (Iteration 4)
// ============================================================================
// NOTE: Device class does not have read() method (TX-only API)
// These tests are commented out

/*
TEST_CASE_FIXTURE(SPITestFixture, "Device read operations") {
    SUBCASE("Basic read succeeds") {
        Config cfg(18, 19);
        Device spi(cfg);
        spi.begin();

        uint8_t buffer[4] = {0xFF, 0xFF, 0xFF, 0xFF};
        Result<void> result = spi.read(buffer, 4);

        CHECK(result.ok());
        // Note: Current implementation fills buffer with 0x00 until hardware RX is supported
        CHECK(buffer[0] == 0x00);
        CHECK(buffer[1] == 0x00);
        CHECK(buffer[2] == 0x00);
        CHECK(buffer[3] == 0x00);

        spi.end();
    }

    SUBCASE("Read fails without begin()") {
        Config cfg(18, 19);
        Device spi(cfg);

        uint8_t buffer[4];
        Result<void> result = spi.read(buffer, 4);

        CHECK(!result.ok());
        CHECK(result.error() == SPIError::NOT_INITIALIZED);
    }

    SUBCASE("Read fails with null buffer") {
        Config cfg(18, 19);
        Device spi(cfg);
        spi.begin();

        Result<void> result = spi.read(nullptr, 4);

        CHECK(!result.ok());
        CHECK(result.error() == SPIError::ALLOCATION_FAILED);

        spi.end();
    }

    SUBCASE("Read fails with zero size") {
        Config cfg(18, 19);
        Device spi(cfg);
        spi.begin();

        uint8_t buffer[4];
        Result<void> result = spi.read(buffer, 0);

        CHECK(!result.ok());
        CHECK(result.error() == SPIError::ALLOCATION_FAILED);

        spi.end();
    }

    SUBCASE("Multiple reads work correctly") {
        Config cfg(18, 19);
        Device spi(cfg);
        spi.begin();

        uint8_t buffer1[2];
        uint8_t buffer2[4];

        Result<void> result1 = spi.read(buffer1, 2);
        CHECK(result1.ok());

        Result<void> result2 = spi.read(buffer2, 4);
        CHECK(result2.ok());

        spi.end();
    }

    SUBCASE("Read with varying buffer sizes") {
        Config cfg(18, 19);
        Device spi(cfg);
        spi.begin();

        // Small buffer
        uint8_t small[2];
        CHECK(spi.read(small, 2).ok());

        // Medium buffer
        uint8_t medium[64];
        CHECK(spi.read(medium, 64).ok());

        // Large buffer
        uint8_t large[256];
        CHECK(spi.read(large, 256).ok());

        spi.end();
    }
}
*/

// ============================================================================
// Device Transfer Operations (Iteration 4)
// ============================================================================
// NOTE: Device class does not have transfer() method (TX-only API)
// These tests are commented out

/*
TEST_CASE_FIXTURE(SPITestFixture, "Device transfer operations") {
    SUBCASE("Basic transfer succeeds") {
        Config cfg(18, 19);
        Device spi(cfg);
        spi.begin();

        uint8_t tx_data[4] = {0xAA, 0xBB, 0xCC, 0xDD};
        uint8_t rx_buffer[4] = {0xFF, 0xFF, 0xFF, 0xFF};

        Result<void> result = spi.transfer(tx_data, rx_buffer, 4);

        CHECK(result.ok());
        // Note: Current implementation fills RX buffer with 0x00 until hardware RX is supported
        // But TX data should still be transmitted
        CHECK(rx_buffer[0] == 0x00);
        CHECK(rx_buffer[1] == 0x00);
        CHECK(rx_buffer[2] == 0x00);
        CHECK(rx_buffer[3] == 0x00);

        spi.end();
    }

    SUBCASE("Transfer fails without begin()") {
        Config cfg(18, 19);
        Device spi(cfg);

        uint8_t tx_data[4] = {1, 2, 3, 4};
        uint8_t rx_buffer[4];

        Result<void> result = spi.transfer(tx_data, rx_buffer, 4);

        CHECK(!result.ok());
        CHECK(result.error() == SPIError::NOT_INITIALIZED);
    }

    SUBCASE("Transfer fails with null tx_data") {
        Config cfg(18, 19);
        Device spi(cfg);
        spi.begin();

        uint8_t rx_buffer[4];
        Result<void> result = spi.transfer(nullptr, rx_buffer, 4);

        CHECK(!result.ok());
        CHECK(result.error() == SPIError::ALLOCATION_FAILED);

        spi.end();
    }

    SUBCASE("Transfer fails with null rx_buffer") {
        Config cfg(18, 19);
        Device spi(cfg);
        spi.begin();

        uint8_t tx_data[4] = {1, 2, 3, 4};
        Result<void> result = spi.transfer(tx_data, nullptr, 4);

        CHECK(!result.ok());
        CHECK(result.error() == SPIError::ALLOCATION_FAILED);

        spi.end();
    }

    SUBCASE("Transfer fails with zero size") {
        Config cfg(18, 19);
        Device spi(cfg);
        spi.begin();

        uint8_t tx_data[4] = {1, 2, 3, 4};
        uint8_t rx_buffer[4];
        Result<void> result = spi.transfer(tx_data, rx_buffer, 0);

        CHECK(!result.ok());
        CHECK(result.error() == SPIError::ALLOCATION_FAILED);

        spi.end();
    }

    SUBCASE("Multiple transfers work correctly") {
        Config cfg(18, 19);
        Device spi(cfg);
        spi.begin();

        uint8_t tx1[2] = {0x01, 0x02};
        uint8_t rx1[2];
        uint8_t tx2[4] = {0xAA, 0xBB, 0xCC, 0xDD};
        uint8_t rx2[4];

        Result<void> result1 = spi.transfer(tx1, rx1, 2);
        CHECK(result1.ok());

        Result<void> result2 = spi.transfer(tx2, rx2, 4);
        CHECK(result2.ok());

        spi.end();
    }

    SUBCASE("Transfer with varying buffer sizes") {
        Config cfg(18, 19);
        Device spi(cfg);
        spi.begin();

        // Small buffer
        uint8_t tx_small[2] = {0x01, 0x02};
        uint8_t rx_small[2];
        CHECK(spi.transfer(tx_small, rx_small, 2).ok());

        // Medium buffer
        uint8_t tx_medium[64];
        uint8_t rx_medium[64];
        for (size_t i = 0; i < 64; i++) tx_medium[i] = i & 0xFF;
        CHECK(spi.transfer(tx_medium, rx_medium, 64).ok());

        // Large buffer
        uint8_t tx_large[256];
        uint8_t rx_large[256];
        for (size_t i = 0; i < 256; i++) tx_large[i] = i & 0xFF;
        CHECK(spi.transfer(tx_large, rx_large, 256).ok());

        spi.end();
    }

    SUBCASE("Transfer transmits TX data correctly") {
        Config cfg(18, 19);
        Device spi(cfg);
        spi.begin();

        uint8_t tx_data[4] = {0x12, 0x34, 0x56, 0x78};
        uint8_t rx_buffer[4];

        Result<void> result = spi.transfer(tx_data, rx_buffer, 4);
        CHECK(result.ok());

        // We can't directly verify that TX data was transmitted since we don't have
        // access to the stub's internal state from here, but we can verify that
        // the transfer operation completed successfully

        spi.end();
    }
}
*/

// ============================================================================
// Async Write Tests (Iteration 5)
// ============================================================================

TEST_CASE_FIXTURE(SPITestFixture, "Device writeAsync operations") {
    SUBCASE("Basic writeAsync succeeds and returns Transaction") {
        Config cfg(18, 19);
        Device spi(cfg);
        spi.begin();

        uint8_t data[4] = {0x01, 0x02, 0x03, 0x04};
        Result<Transaction> result = spi.writeAsync(data, 4);

        CHECK(result.ok());

        // Get transaction and wait for completion
        Transaction txn = fl::move(result.value());
        CHECK(txn.wait());
        CHECK(txn.isDone());
        CHECK(!txn.isPending());
        CHECK(!txn.getResult());  // No error means success

        spi.end();
    }

    SUBCASE("writeAsync fails without begin()") {
        Config cfg(18, 19);
        Device spi(cfg);

        uint8_t data[4] = {0x01, 0x02, 0x03, 0x04};
        Result<Transaction> result = spi.writeAsync(data, 4);

        CHECK(!result.ok());
        CHECK(result.error() == SPIError::NOT_INITIALIZED);
    }

    SUBCASE("writeAsync fails with null data") {
        Config cfg(18, 19);
        Device spi(cfg);
        spi.begin();

        Result<Transaction> result = spi.writeAsync(nullptr, 4);

        CHECK(!result.ok());
        CHECK(result.error() == SPIError::ALLOCATION_FAILED);

        spi.end();
    }

    SUBCASE("writeAsync fails with zero size") {
        Config cfg(18, 19);
        Device spi(cfg);
        spi.begin();

        uint8_t data[4] = {0x01, 0x02, 0x03, 0x04};
        Result<Transaction> result = spi.writeAsync(data, 0);

        CHECK(!result.ok());
        CHECK(result.error() == SPIError::ALLOCATION_FAILED);

        spi.end();
    }

    SUBCASE("Multiple sequential async writes work") {
        Config cfg(18, 19);
        Device spi(cfg);
        spi.begin();

        // First async write
        uint8_t data1[4] = {0x01, 0x02, 0x03, 0x04};
        Result<Transaction> result1 = spi.writeAsync(data1, 4);
        CHECK(result1.ok());
        Transaction txn1 = fl::move(result1.value());
        CHECK(txn1.wait());
        CHECK(txn1.isDone());

        // Second async write (after first completes)
        uint8_t data2[4] = {0x05, 0x06, 0x07, 0x08};
        Result<Transaction> result2 = spi.writeAsync(data2, 4);
        CHECK(result2.ok());
        Transaction txn2 = fl::move(result2.value());
        CHECK(txn2.wait());
        CHECK(txn2.isDone());

        spi.end();
    }

    SUBCASE("writeAsync with varying buffer sizes") {
        Config cfg(18, 19);
        Device spi(cfg);
        spi.begin();

        // Small buffer
        uint8_t small[2] = {0x01, 0x02};
        Result<Transaction> r1 = spi.writeAsync(small, 2);
        CHECK(r1.ok());
        Transaction t1 = fl::move(r1.value());
        CHECK(t1.wait());

        // Medium buffer
        uint8_t medium[64];
        for (size_t i = 0; i < 64; i++) medium[i] = i & 0xFF;
        Result<Transaction> r2 = spi.writeAsync(medium, 64);
        CHECK(r2.ok());
        Transaction t2 = fl::move(r2.value());
        CHECK(t2.wait());

        // Large buffer
        uint8_t large[256];
        for (size_t i = 0; i < 256; i++) large[i] = i & 0xFF;
        Result<Transaction> r3 = spi.writeAsync(large, 256);
        CHECK(r3.ok());
        Transaction t3 = fl::move(r3.value());
        CHECK(t3.wait());

        spi.end();
    }

    SUBCASE("Transaction auto-waits on destruction") {
        Config cfg(18, 19);
        Device spi(cfg);
        spi.begin();

        uint8_t data[4] = {0x01, 0x02, 0x03, 0x04};

        {
            Result<Transaction> result = spi.writeAsync(data, 4);
            CHECK(result.ok());
            Transaction txn = fl::move(result.value());
            // Transaction destructor will auto-wait
        }

        // After transaction is destroyed, we should be able to start another
        Result<Transaction> result2 = spi.writeAsync(data, 4);
        CHECK(result2.ok());

        spi.end();
    }
}

// ============================================================================
// Async Read Tests (Iteration 5)
// ============================================================================
// NOTE: Device class does not have readAsync() method (TX-only API)
// These tests are commented out

/*
TEST_CASE_FIXTURE(SPITestFixture, "Device readAsync operations") {
    SUBCASE("Basic readAsync succeeds and returns Transaction") {
        Config cfg(18, 19);
        Device spi(cfg);
        spi.begin();

        uint8_t buffer[4];
        Result<Transaction> result = spi.readAsync(buffer, 4);

        CHECK(result.ok());

        Transaction txn = fl::move(result.value());
        CHECK(txn.isDone());  // Should complete immediately (stub implementation)
        CHECK(txn.getResult().ok());

        // Buffer should be filled with 0x00 (no RX data yet)
        for (size_t i = 0; i < 4; i++) {
            CHECK(buffer[i] == 0x00);
        }

        spi.end();
    }

    SUBCASE("readAsync fails without begin()") {
        Config cfg(18, 19);
        Device spi(cfg);

        uint8_t buffer[4];
        Result<Transaction> result = spi.readAsync(buffer, 4);

        CHECK(!result.ok());
        CHECK(result.error() == SPIError::NOT_INITIALIZED);
    }

    SUBCASE("readAsync fails with null buffer") {
        Config cfg(18, 19);
        Device spi(cfg);
        spi.begin();

        Result<Transaction> result = spi.readAsync(nullptr, 4);

        CHECK(!result.ok());
        CHECK(result.error() == SPIError::ALLOCATION_FAILED);

        spi.end();
    }

    SUBCASE("readAsync fails with zero size") {
        Config cfg(18, 19);
        Device spi(cfg);
        spi.begin();

        uint8_t buffer[4];
        Result<Transaction> result = spi.readAsync(buffer, 0);

        CHECK(!result.ok());
        CHECK(result.error() == SPIError::ALLOCATION_FAILED);

        spi.end();
    }
}
*/

// ============================================================================
// Async Transfer Tests (Iteration 5)
// ============================================================================
// NOTE: Device class does not have transferAsync() method (TX-only API)
// These tests are commented out

/*
TEST_CASE_FIXTURE(SPITestFixture, "Device transferAsync operations") {
    SUBCASE("Basic transferAsync succeeds and returns Transaction") {
        Config cfg(18, 19);
        Device spi(cfg);
        spi.begin();

        uint8_t tx_data[4] = {0x12, 0x34, 0x56, 0x78};
        uint8_t rx_buffer[4];
        Result<Transaction> result = spi.transferAsync(tx_data, rx_buffer, 4);

        CHECK(result.ok());

        Transaction txn = fl::move(result.value());
        CHECK(txn.wait());
        CHECK(txn.isDone());
        CHECK(txn.getResult().ok());

        // RX buffer should be filled with 0x00 (no RX data yet)
        for (size_t i = 0; i < 4; i++) {
            CHECK(rx_buffer[i] == 0x00);
        }

        spi.end();
    }

    SUBCASE("transferAsync fails without begin()") {
        Config cfg(18, 19);
        Device spi(cfg);

        uint8_t tx_data[4] = {0x01, 0x02, 0x03, 0x04};
        uint8_t rx_buffer[4];
        Result<Transaction> result = spi.transferAsync(tx_data, rx_buffer, 4);

        CHECK(!result.ok());
        CHECK(result.error() == SPIError::NOT_INITIALIZED);
    }

    SUBCASE("transferAsync fails with null tx_data") {
        Config cfg(18, 19);
        Device spi(cfg);
        spi.begin();

        uint8_t rx_buffer[4];
        Result<Transaction> result = spi.transferAsync(nullptr, rx_buffer, 4);

        CHECK(!result.ok());
        CHECK(result.error() == SPIError::ALLOCATION_FAILED);

        spi.end();
    }

    SUBCASE("transferAsync fails with null rx_buffer") {
        Config cfg(18, 19);
        Device spi(cfg);
        spi.begin();

        uint8_t tx_data[4] = {0x01, 0x02, 0x03, 0x04};
        Result<Transaction> result = spi.transferAsync(tx_data, nullptr, 4);

        CHECK(!result.ok());
        CHECK(result.error() == SPIError::ALLOCATION_FAILED);

        spi.end();
    }

    SUBCASE("transferAsync fails with zero size") {
        Config cfg(18, 19);
        Device spi(cfg);
        spi.begin();

        uint8_t tx_data[4] = {0x01, 0x02, 0x03, 0x04};
        uint8_t rx_buffer[4];
        Result<Transaction> result = spi.transferAsync(tx_data, rx_buffer, 0);

        CHECK(!result.ok());
        CHECK(result.error() == SPIError::ALLOCATION_FAILED);

        spi.end();
    }
}
*/

// ============================================================================
// Transaction Tests (Iteration 5)
// ============================================================================

TEST_CASE_FIXTURE(SPITestFixture, "Transaction lifecycle") {
    SUBCASE("Transaction isDone() and isPending() work correctly") {
        Config cfg(18, 19);
        Device spi(cfg);
        spi.begin();

        uint8_t data[4] = {0x01, 0x02, 0x03, 0x04};
        Result<Transaction> result = spi.writeAsync(data, 4);
        CHECK(result.ok());

        Transaction txn = fl::move(result.value());

        // Wait for completion
        CHECK(txn.wait());

        // After wait, should be done
        CHECK(txn.isDone());
        CHECK(!txn.isPending());

        spi.end();
    }

    SUBCASE("Transaction cancel() marks as completed") {
        Config cfg(18, 19);
        Device spi(cfg);
        spi.begin();

        uint8_t data[4] = {0x01, 0x02, 0x03, 0x04};
        Result<Transaction> result = spi.writeAsync(data, 4);
        CHECK(result.ok());

        Transaction txn = fl::move(result.value());

        // Cancel immediately
        bool cancelled = txn.cancel();
        CHECK(cancelled);
        CHECK(txn.isDone());
        CHECK(!txn.isPending());

        // Calling cancel again should fail
        CHECK(!txn.cancel());

        spi.end();
    }

    SUBCASE("Transaction getResult() returns correct result") {
        Config cfg(18, 19);
        Device spi(cfg);
        spi.begin();

        uint8_t data[4] = {0x01, 0x02, 0x03, 0x04};
        Result<Transaction> result = spi.writeAsync(data, 4);
        CHECK(result.ok());

        Transaction txn = fl::move(result.value());
        CHECK(txn.wait());

        fl::optional<fl::Error> txn_result = txn.getResult();
        CHECK(!txn_result);  // No error means success

        spi.end();
    }

    SUBCASE("Transaction move semantics work") {
        Config cfg(18, 19);
        Device spi(cfg);
        spi.begin();

        uint8_t data[4] = {0x01, 0x02, 0x03, 0x04};
        Result<Transaction> result = spi.writeAsync(data, 4);
        CHECK(result.ok());

        Transaction txn1 = fl::move(result.value());

        // Move to another transaction
        Transaction txn2 = fl::move(txn1);

        CHECK(txn2.wait());
        CHECK(txn2.isDone());

        spi.end();
    }
}

// ============================================================================
// Configuration Management Tests (Iteration 7)
// ============================================================================

TEST_CASE_FIXTURE(SPITestFixture, "Device configuration management") {
    SUBCASE("getConfig() returns correct configuration") {
        Config cfg(18, 19);
        cfg.clock_speed_hz = 5000000;  // 5 MHz
        cfg.spi_mode = 0;

        Device spi(cfg);

        const Config& retrieved = spi.getConfig();
        CHECK(retrieved.clock_pin == 18);
        CHECK(retrieved.data_pins.size() == 1);
        CHECK(retrieved.data_pins[0] == 19);
        CHECK(retrieved.clock_speed_hz == 5000000);
        CHECK(retrieved.spi_mode == 0);
    }

    SUBCASE("setClockSpeed() updates configuration before begin()") {
        Config cfg(18, 19);
        cfg.clock_speed_hz = 10000000;  // 10 MHz

        Device spi(cfg);

        // Update clock speed before initialization
        fl::optional<fl::Error> result = spi.setClockSpeed(20000000);  // 20 MHz
        CHECK(!result);  // No error means success

        // Verify configuration was updated
        const Config& retrieved = spi.getConfig();
        CHECK(retrieved.clock_speed_hz == 20000000);
    }

    SUBCASE("setClockSpeed() updates configuration after begin()") {
        Config cfg(18, 19);
        Device spi(cfg);

        CHECK(!spi.begin());  // No error means success

        // Update clock speed after initialization
        fl::optional<fl::Error> result = spi.setClockSpeed(15000000);  // 15 MHz
        CHECK(!result);  // No error means success

        // Verify configuration was updated
        const Config& retrieved = spi.getConfig();
        CHECK(retrieved.clock_speed_hz == 15000000);

        // Note: Hardware clock speed won't change until next begin()
        // This is documented behavior

        spi.end();
    }

    SUBCASE("setClockSpeed() with zero speed") {
        Config cfg(18, 19);
        Device spi(cfg);

        // Setting to zero should succeed (though may not be practical)
        fl::optional<fl::Error> result = spi.setClockSpeed(0);
        CHECK(!result);  // No error means success
        CHECK(spi.getConfig().clock_speed_hz == 0);
    }

    SUBCASE("setClockSpeed() with very high speed") {
        Config cfg(18, 19);
        Device spi(cfg);

        // Setting to very high speed should succeed
        // (hardware will clamp to maximum supported speed)
        fl::optional<fl::Error> result = spi.setClockSpeed(80000000);  // 80 MHz
        CHECK(!result);  // No error means success
        CHECK(spi.getConfig().clock_speed_hz == 80000000);
    }
}

TEST_CASE_FIXTURE(SPITestFixture, "SPI mode configuration") {
    SUBCASE("Mode 0 (default) is accepted") {
        Config cfg(18, 19);
        cfg.spi_mode = 0;

        Device spi(cfg);
        fl::optional<fl::Error> result = spi.begin();
        CHECK(!result);  // No error means success

        spi.end();
    }

    SUBCASE("Mode 1 generates warning but initialization succeeds") {
        Config cfg(18, 19);
        cfg.spi_mode = 1;

        Device spi(cfg);
        fl::optional<fl::Error> result = spi.begin();
        // Should succeed despite warning (mode is ignored)
        CHECK(!result);  // No error means success

        spi.end();
    }

    SUBCASE("Mode 2 generates warning but initialization succeeds") {
        Config cfg(18, 19);
        cfg.spi_mode = 2;

        Device spi(cfg);
        fl::optional<fl::Error> result = spi.begin();
        CHECK(!result);  // No error means success

        spi.end();
    }

    SUBCASE("Mode 3 generates warning but initialization succeeds") {
        Config cfg(18, 19);
        cfg.spi_mode = 3;

        Device spi(cfg);
        fl::optional<fl::Error> result = spi.begin();
        CHECK(!result);  // No error means success

        spi.end();
    }

    SUBCASE("Invalid mode (>3) is rejected") {
        Config cfg(18, 19);
        cfg.spi_mode = 4;  // Invalid

        Device spi(cfg);
        fl::optional<fl::Error> result = spi.begin();
        CHECK(result.has_value());  // Error present means failure
    }

    SUBCASE("Invalid mode (255) is rejected") {
        Config cfg(18, 19);
        cfg.spi_mode = 255;  // Very invalid

        Device spi(cfg);
        fl::optional<fl::Error> result = spi.begin();
        CHECK(result.has_value());  // Error present means failure
    }

    SUBCASE("Mode configuration is preserved in getConfig()") {
        Config cfg(18, 19);
        cfg.spi_mode = 2;

        Device spi(cfg);

        const Config& retrieved = spi.getConfig();
        CHECK(retrieved.spi_mode == 2);
    }
}
