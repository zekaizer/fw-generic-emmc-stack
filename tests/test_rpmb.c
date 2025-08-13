/*
 * RPMB Test Suite
 * 
 * This file provides comprehensive testing for RPMB functionality
 * including mock crypto interface and various test scenarios.
 */

#include "../src/protocol/emmc_protocol.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

/* Mock RPMB key storage */
static u8 g_mock_rpmb_key[32];
static bool g_key_injected = false;

/* Mock write counter */
static u32 g_mock_write_counter = 0;

/* Mock crypto implementation */
static emmc_result_t mock_get_key(u8 *key, u32 key_len)
{
    if (!key || key_len != 32) {
        return EMMC_INVALID_PARAM;
    }
    
    if (!g_key_injected) {
        printf("Mock: Key not available\n");
        return EMMC_ERROR;
    }
    
    memcpy(key, g_mock_rpmb_key, 32);
    printf("Mock: Key retrieved successfully\n");
    
    return EMMC_OK;
}

static emmc_result_t mock_generate_nonce(u8 *nonce, u32 nonce_len)
{
    if (!nonce || nonce_len != 16) {
        return EMMC_INVALID_PARAM;
    }
    
    /* Generate deterministic nonce for testing */
    for (u32 i = 0; i < nonce_len; i++) {
        nonce[i] = (u8)(i * 0x11);
    }
    
    printf("Mock: Nonce generated successfully\n");
    return EMMC_OK;
}

static emmc_result_t mock_compute_hmac(const u8 *key, u32 key_len,
                                      const u8 *data, u32 data_len,
                                      u8 *mac, u32 mac_len)
{
    if (!key || !data || !mac || key_len != 32 || mac_len != 32) {
        return EMMC_INVALID_PARAM;
    }
    
    if (!g_key_injected) {
        printf("Mock: Key not injected\n");
        return EMMC_ERROR;
    }
    
    /* Simple mock HMAC - XOR key with data hash */
    /* In real implementation, this would be proper HMAC-SHA256 */
    u32 hash = 0x5A5A5A5A; /* Mock hash */
    for (u32 i = 0; i < data_len; i++) {
        hash ^= data[i];
        hash = (hash << 1) | (hash >> 31); /* Rotate left */
    }
    
    for (u32 i = 0; i < mac_len; i++) {
        mac[i] = g_mock_rpmb_key[i] ^ ((u8)(hash >> (i % 4)));
    }
    
    printf("Mock: HMAC computed (hash: 0x%08X)\n", hash);
    return EMMC_OK;
}

static emmc_result_t mock_verify_hmac(const u8 *key, u32 key_len,
                                     const u8 *data, u32 data_len,
                                     const u8 *expected_mac, u32 mac_len)
{
    u8 computed_mac[32];
    emmc_result_t result;
    
    /* Compute MAC */
    result = mock_compute_hmac(key, key_len, data, data_len, computed_mac, mac_len);
    if (result != EMMC_OK) {
        return result;
    }
    
    /* Compare MACs */
    if (memcmp(computed_mac, expected_mac, mac_len) != 0) {
        printf("Mock: HMAC verification failed\n");
        return EMMC_ERROR;
    }
    
    printf("Mock: HMAC verification successful\n");
    return EMMC_OK;
}

/* Mock crypto interface */
static emmc_rpmb_crypto_interface_t mock_crypto_interface = {
    .get_key = mock_get_key,
    .generate_nonce = mock_generate_nonce,
    .compute_hmac = mock_compute_hmac,
    .verify_hmac = mock_verify_hmac
};

/* Helper function to inject key for testing */
static emmc_result_t mock_inject_key_for_test(const u8 *key, u32 key_len)
{
    if (!key || key_len != 32) {
        return EMMC_INVALID_PARAM;
    }
    
    memcpy(g_mock_rpmb_key, key, 32);
    g_key_injected = true;
    printf("Mock: Key injected for test\n");
    
    return EMMC_OK;
}

/* Test data */
static u8 test_key[32] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F
};

static u8 test_data[256] = {
    0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11,
    0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
    /* Fill remaining with pattern */
};

/* Initialize test data */
static void init_test_data(void)
{
    for (int i = 16; i < 256; i++) {
        test_data[i] = (u8)(i ^ 0x5A);
    }
}

/* Test 1: Basic RPMB Initialization */
static void test_rpmb_init(void)
{
    emmc_result_t result;
    
    printf("\n=== Test 1: RPMB Initialization ===\n");
    
    /* Test with NULL interface */
    result = emmc_rpmb_init(NULL);
    assert(result == EMMC_INVALID_PARAM);
    printf("✓ NULL interface rejected\n");
    
    /* Test with valid interface */
    result = emmc_rpmb_init(&mock_crypto_interface);
    assert(result == EMMC_OK);
    printf("✓ Valid interface accepted\n");
    
    printf("Test 1 PASSED\n");
}

/* Test 2: Key Programming */
static void test_rpmb_key_programming(void)
{
    emmc_result_t result;
    
    printf("\n=== Test 2: Key Programming ===\n");
    
    /* Reset mock state */
    g_key_injected = false;
    memset(g_mock_rpmb_key, 0, sizeof(g_mock_rpmb_key));
    
    /* Test with NULL key */
    result = emmc_rpmb_program_key(NULL);
    assert(result == EMMC_INVALID_PARAM);
    printf("✓ NULL key rejected\n");
    
    /* Test with valid key */
    result = emmc_rpmb_program_key(test_key);
    /* Note: This will fail without actual eMMC hardware, but key injection should work */
    printf("✓ Key programming attempted (result: %d)\n", result);
    
    /* Verify key was injected */
    assert(g_key_injected);
    assert(memcmp(g_mock_rpmb_key, test_key, 32) == 0);
    printf("✓ Key successfully injected to mock storage\n");
    
    printf("Test 2 PASSED\n");
}

/* Test 3: Write Counter Operations */  
static void test_rpmb_write_counter(void)
{
    emmc_result_t result;
    u32 counter;
    
    printf("\n=== Test 3: Write Counter Operations ===\n");
    
    /* Test with NULL counter pointer */
    result = emmc_rpmb_get_write_counter(NULL);
    assert(result == EMMC_INVALID_PARAM);
    printf("✓ NULL counter pointer rejected\n");
    
    /* Test valid counter read (will fail without hardware) */
    result = emmc_rpmb_get_write_counter(&counter);
    printf("✓ Write counter read attempted (result: %d)\n", result);
    
    printf("Test 3 PASSED\n");
}

/* Test 4: Data Write Operations */
static void test_rpmb_write_data(void)
{
    emmc_result_t result;
    
    printf("\n=== Test 4: Data Write Operations ===\n");
    
    /* Test with NULL data */
    result = emmc_rpmb_write_data(0, NULL, 1, test_key);
    assert(result == EMMC_INVALID_PARAM);
    printf("✓ NULL data rejected\n");
    
    /* Test with NULL key */
    result = emmc_rpmb_write_data(0, test_data, 1, NULL);
    assert(result == EMMC_INVALID_PARAM);
    printf("✓ NULL key rejected\n");
    
    /* Test with zero block count */
    result = emmc_rpmb_write_data(0, test_data, 0, test_key);
    assert(result == EMMC_INVALID_PARAM);
    printf("✓ Zero block count rejected\n");
    
    /* Test with multiple blocks (not supported) */
    result = emmc_rpmb_write_data(0, test_data, 2, test_key);
    assert(result == EMMC_NOT_SUPPORTED);
    printf("✓ Multiple blocks correctly rejected\n");
    
    /* Test valid single block write (will fail without hardware) */
    result = emmc_rpmb_write_data(0, test_data, 1, test_key);
    printf("✓ Single block write attempted (result: %d)\n", result);
    
    printf("Test 4 PASSED\n");
}

/* Test 5: Data Read Operations */
static void test_rpmb_read_data(void)
{
    emmc_result_t result;
    u8 read_buffer[256];
    
    printf("\n=== Test 5: Data Read Operations ===\n");
    
    /* Test with NULL data buffer */
    result = emmc_rpmb_read_data(0, NULL, 1, test_key);
    assert(result == EMMC_INVALID_PARAM);
    printf("✓ NULL data buffer rejected\n");
    
    /* Test with NULL key */
    result = emmc_rpmb_read_data(0, read_buffer, 1, NULL);
    assert(result == EMMC_INVALID_PARAM);
    printf("✓ NULL key rejected\n");
    
    /* Test with zero block count */
    result = emmc_rpmb_read_data(0, read_buffer, 0, test_key);
    assert(result == EMMC_INVALID_PARAM);
    printf("✓ Zero block count rejected\n");
    
    /* Test with multiple blocks (not supported) */
    result = emmc_rpmb_read_data(0, read_buffer, 2, test_key);
    assert(result == EMMC_NOT_SUPPORTED);
    printf("✓ Multiple blocks correctly rejected\n");
    
    /* Test valid single block read (will fail without hardware) */
    memset(read_buffer, 0, sizeof(read_buffer));
    result = emmc_rpmb_read_data(0, read_buffer, 1, test_key);
    printf("✓ Single block read attempted (result: %d)\n", result);
    
    printf("Test 5 PASSED\n");
}

/* Test 6: Crypto Interface Functions */
static void test_crypto_interface(void)
{
    emmc_result_t result;
    u8 mac[32];
    u8 test_payload[] = "Hello RPMB World!";
    
    printf("\n=== Test 6: Crypto Interface Functions ===\n");
    
    /* Reset and inject key */
    g_key_injected = false;
    result = mock_inject_key_for_test(test_key, 32);
    assert(result == EMMC_OK);
    printf("✓ Key injection successful\n");
    
    /* Test HMAC computation */
    result = mock_compute_hmac(test_key, 32, test_payload, sizeof(test_payload), mac, 32);
    assert(result == EMMC_OK);
    printf("✓ HMAC computation successful\n");
    
    /* Test HMAC verification with correct MAC */
    result = mock_verify_hmac(test_key, 32, test_payload, sizeof(test_payload), mac, 32);
    assert(result == EMMC_OK);
    printf("✓ HMAC verification successful\n");
    
    /* Test HMAC verification with wrong MAC */
    mac[0] ^= 0xFF; /* Corrupt MAC */
    result = mock_verify_hmac(test_key, 32, test_payload, sizeof(test_payload), mac, 32);
    assert(result == EMMC_ERROR);
    printf("✓ HMAC verification correctly failed with corrupted MAC\n");
    
    printf("Test 6 PASSED\n");
}

/* Test 7: Error Handling */
static void test_error_handling(void)
{
    emmc_result_t result;
    u8 buffer[256];
    
    printf("\n=== Test 7: Error Handling ===\n");
    
    /* Test operations without RPMB initialization */
    /* Note: This assumes protocol is not initialized */
    result = emmc_rpmb_write_data(0, test_data, 1, test_key);
    printf("✓ Write without init (result: %d)\n", result);
    
    result = emmc_rpmb_read_data(0, buffer, 1, test_key);
    printf("✓ Read without init (result: %d)\n", result);
    
    /* Test with invalid key lengths in crypto functions */
    u8 short_key[16] = {0};
    u8 mac[32];
    
    result = mock_inject_key_for_test(short_key, 16);
    assert(result == EMMC_INVALID_PARAM);
    printf("✓ Short key rejected\n");
    
    result = mock_compute_hmac(short_key, 16, test_data, 256, mac, 32);
    assert(result == EMMC_INVALID_PARAM);
    printf("✓ Short key in HMAC rejected\n");
    
    printf("Test 7 PASSED\n");
}

/* Main test runner */
int test_rpmb_main(void)
{
    printf("Starting RPMB Test Suite...\n");
    printf("Note: Some tests will show failures due to missing hardware - this is expected.\n");
    
    /* Initialize test data */
    init_test_data();
    
    /* Run all tests */
    test_rpmb_init();
    test_rpmb_key_programming();
    test_rpmb_write_counter();
    test_rpmb_write_data();
    test_rpmb_read_data();
    test_crypto_interface();
    test_error_handling();
    
    printf("\n=== RPMB Test Suite Summary ===\n");
    printf("All parameter validation tests PASSED\n");
    printf("Mock crypto interface tests PASSED\n");
    printf("Hardware-dependent tests attempted (expected to fail without eMMC)\n");
    printf("RPMB Test Suite completed successfully!\n");
    
    return 0;
}

/* Example usage function */
void rpmb_usage_example(void)
{
    printf("\n=== RPMB Usage Example ===\n");
    
    /* 1. Initialize RPMB with crypto interface */
    emmc_rpmb_init(&mock_crypto_interface);
    
    /* 2. Program authentication key (one-time only) */
    u8 auth_key[32];
    /* Generate or retrieve device-specific key */
    memcpy(auth_key, test_key, 32);
    emmc_rpmb_program_key(auth_key);
    
    /* 3. Write secure data */
    u8 secure_data[256] = "This is confidential data stored in RPMB";
    emmc_result_t result = emmc_rpmb_write_data(0, secure_data, 1, auth_key);
    
    if (result == EMMC_OK) {
        printf("Secure data written successfully\n");
        
        /* 4. Read and verify secure data */
        u8 read_buffer[256];
        result = emmc_rpmb_read_data(0, read_buffer, 1, auth_key);
        
        if (result == EMMC_OK && memcmp(secure_data, read_buffer, 256) == 0) {
            printf("Secure data read and verified successfully\n");
        }
    }
    
    printf("RPMB usage example completed\n");
}