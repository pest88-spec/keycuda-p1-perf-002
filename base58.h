#pragma once

#include <string>
#include <vector>
#include <cstdint>

// Simple Base58 encoding implementation
class Base58 {
public:
    static std::string encode(const std::vector<uint8_t>& input) {
        const char* alphabet = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

        // Convert to big integer
        std::vector<uint8_t> digits = input;

        // Count leading zeros
        size_t leading_zeros = 0;
        while (leading_zeros < digits.size() && digits[leading_zeros] == 0) {
            leading_zeros++;
        }

        // Encode
        std::string result;
        while (!digits.empty()) {
            std::vector<uint8_t> quotient;
            uint32_t remainder = 0;

            for (uint8_t digit : digits) {
                uint32_t value = (remainder << 8) + digit;
                remainder = value % 58;
                value = value / 58;
                if (!quotient.empty() || value != 0) {
                    quotient.push_back((uint8_t)value);
                }
            }

            digits = quotient;
            result = alphabet[remainder] + result;
        }

        // Add leading '1's for each leading zero
        for (size_t i = 0; i < leading_zeros; i++) {
            result = '1' + result;
        }

        return result;
    }

    static std::string encode(const uint8_t* data, size_t len) {
        std::vector<uint8_t> input(data, data + len);
        return encode(input);
    }
};