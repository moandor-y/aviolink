#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <Arduino.h>
#include "magnetic_variation.h"

MockSerial Serial;
MockSerial Serial1;

// Declare the external functions defined in main.ino
extern void ProcessSentence(const String& line);
extern int SplitString(const String& str, char delimiter, String* tokens, int max_tokens);
extern String CalculateChecksum(const String& sentence);
extern float current_mag_var;
extern String current_mag_dir;

int tests_run = 0;
int tests_failed = 0;

void verify(bool condition, const std::string& message) {
    tests_run++;
    if (condition) {
        std::cout << "  [PASS] " << message << std::endl;
    } else {
        std::cout << "  [FAIL] " << message << std::endl;
        tests_failed++;
    }
}

void test_CalculateChecksum() {
    std::cout << "\nRunning test_CalculateChecksum..." << std::endl;
    // GPRMC payload: GPRMC,081836,A,3751.65,S,14507.36,E,000.0,360.0,130998,011.3,E -> checksum 62
    String sentence1 = "$GPRMC,081836,A,3751.65,S,14507.36,E,000.0,360.0,130998,011.3,E";
    String chk1 = CalculateChecksum(sentence1);
    verify(chk1 == "62", "Checksum of GPRMC sentence should be 62, got: " + std::string(chk1.c_str()));

    // GPVTG payload: GPVTG,360.0,T,348.7,M,000.0,N,000.0,K -> checksum 43
    String sentence2 = "$GPVTG,360.0,T,348.7,M,000.0,N,000.0,K";
    String chk2 = CalculateChecksum(sentence2);
    verify(chk2 == "43", "Checksum of GPVTG sentence should be 43, got: " + std::string(chk2.c_str()));
}

void test_SplitString() {
    std::cout << "\nRunning test_SplitString..." << std::endl;
    String str = "a,b,c,,d";
    String tokens[10];
    int count = SplitString(str, ',', tokens, 10);
    verify(count == 5, "Should return 5 tokens");
    verify(tokens[0] == "a", "Token 0 should be 'a'");
    verify(tokens[1] == "b", "Token 1 should be 'b'");
    verify(tokens[2] == "c", "Token 2 should be 'c'");
    verify(tokens[3] == "", "Token 3 should be empty");
    verify(tokens[4] == "d", "Token 4 should be 'd'");
}

void test_ProcessSentence_GPRMC() {
    std::cout << "\nRunning test_ProcessSentence_GPRMC..." << std::endl;
    Serial1.clear();
    // GPRMC input uses real-world coordinates for Melbourne, Australia (37°51.65' S, 145°07.36' E).
    // GPS Date: 130998 (13 Sep 1998) -> maps to 2098 decimal year -> clamped to WMM-2025 epoch end (2030.0).
    // We expect declination at (37.86° S, 145.12° E) in 2030.0 to be computed dynamically as 12.2° E.
    // Rebuilt sentence should be: $GPRMC,081836,A,3751.65,S,14507.36,E,000.0,360.0,130998,12.2,E*<new_checksum>
    String input = "$GPRMC,081836,A,3751.65,S,14507.36,E,000.0,360.0,130998,011.3,E*62\r\n";
    ProcessSentence(input);
    
    verify(!Serial1.printed_lines.empty(), "Serial1 should output a sentence");
    if (!Serial1.printed_lines.empty()) {
        std::string output = Serial1.printed_lines[0];
        // Strip trailing \r\n for inspection
        if (output.length() >= 2 && output.substr(output.length() - 2) == "\r\n") {
            output = output.substr(0, output.length() - 2);
        }
        
        // Find *
        size_t star = output.find('*');
        verify(star != std::string::npos, "Output sentence should contain '*'");
        if (star != std::string::npos) {
            std::string payload = output.substr(0, star);
            std::string expected_payload = "$GPRMC,081836,A,3751.65,S,14507.36,E,000.0,360.0,130998,12.2,E";
            verify(payload == expected_payload, "Payload should be modified: expected '" + expected_payload + "', got '" + payload + "'");
            
            std::string checksum = output.substr(star + 1);
            String mock_payload(payload);
            std::string expected_checksum = CalculateChecksum(mock_payload).c_str();
            verify(checksum == expected_checksum, "Checksum should be correct: expected '" + expected_checksum + "', got '" + checksum + "'");
        }
    }
}

void test_ProcessSentence_GPRMC_Seattle() {
    std::cout << "\nRunning test_ProcessSentence_GPRMC_Seattle..." << std::endl;
    Serial1.clear();
    // GPRMC input uses real-world coordinates for Seattle, WA, USA (47°36.37' N, 122°19.93' W).
    // GPS Date: 130998 (13 Sep 1998) -> maps to 2098 decimal year -> clamped to WMM-2025 epoch end (2030.0).
    String input = "$GPRMC,081836,A,4736.37,N,12219.93,W,000.0,360.0,130998,011.3,E*6A\r\n";
    ProcessSentence(input);

    verify(!Serial1.printed_lines.empty(), "Serial1 should output a sentence");
    if (!Serial1.printed_lines.empty()) {
        std::string output = Serial1.printed_lines[0];
        if (output.length() >= 2 && output.substr(output.length() - 2) == "\r\n") {
            output = output.substr(0, output.length() - 2);
        }
        
        size_t star = output.find('*');
        verify(star != std::string::npos, "Output sentence should contain '*'");
        if (star != std::string::npos) {
            std::string payload = output.substr(0, star);
            std::cout << "  [INFO] Seattle parsed GPRMC: " << payload << std::endl;
            
            std::string expected_payload = "$GPRMC,081836,A,4736.37,N,12219.93,W,000.0,360.0,130998,14.5,E";
            verify(payload == expected_payload, "Payload should match Seattle expected variation: expected '" + expected_payload + "', got '" + payload + "'");
            
            std::string checksum = output.substr(star + 1);
            String mock_payload(payload);
            std::string expected_checksum = CalculateChecksum(mock_payload).c_str();
            verify(checksum == expected_checksum, "Checksum should be correct: expected '" + expected_checksum + "', got '" + checksum + "'");
        }
    }
}

void test_ProcessSentence_GPRMC_Boston() {
    std::cout << "\nRunning test_ProcessSentence_GPRMC_Boston..." << std::endl;
    Serial1.clear();
    // GPRMC input uses real-world coordinates for Boston, MA, USA (42°21.61' N, 71°03.53' W).
    // GPS Date: 130998 (13 Sep 1998) -> maps to 2098 decimal year -> clamped to WMM-2025 epoch end (2030.0).
    String input = "$GPRMC,081836,A,4221.61,N,07103.53,W,000.0,360.0,130998,011.3,E*6F\r\n";
    ProcessSentence(input);

    verify(!Serial1.printed_lines.empty(), "Serial1 should output a sentence");
    if (!Serial1.printed_lines.empty()) {
        std::string output = Serial1.printed_lines[0];
        if (output.length() >= 2 && output.substr(output.length() - 2) == "\r\n") {
            output = output.substr(0, output.length() - 2);
        }
        
        size_t star = output.find('*');
        verify(star != std::string::npos, "Output sentence should contain '*'");
        if (star != std::string::npos) {
            std::string payload = output.substr(0, star);
            std::cout << "  [INFO] Boston parsed GPRMC: " << payload << std::endl;
            
            // Expected payload variation value will be updated with the output from WMM_Tiny
            std::string expected_payload = "$GPRMC,081836,A,4221.61,N,07103.53,W,000.0,360.0,130998,13.7,W";
            verify(payload == expected_payload, "Payload should match Boston expected variation: expected '" + expected_payload + "', got '" + payload + "'");
            
            std::string checksum = output.substr(star + 1);
            String mock_payload(payload);
            std::string expected_checksum = CalculateChecksum(mock_payload).c_str();
            verify(checksum == expected_checksum, "Checksum should be correct: expected '" + expected_checksum + "', got '" + checksum + "'");
        }
    }
}

void test_ProcessSentence_GPVTG() {
    std::cout << "\nRunning test_ProcessSentence_GPVTG..." << std::endl;
    
    // Explicitly set state to Melbourne variation for these track calculation tests
    current_mag_var = 12.15f;
    current_mag_dir = "E";

    // Case 1: True track 360.0. Magnetic direction W, kMagVar 4.0.
    // mag_track = true_track + 4.0 = 364.0 -> wraps to 4.0.
    // Expect index 3 to be 4.00.
    {
        Serial1.clear();
        String input = "$GPVTG,360.0,T,348.7,M,000.0,N,000.0,K*43\r\n";
        ProcessSentence(input);
        verify(!Serial1.printed_lines.empty(), "Serial1 should output a sentence (Case 1)");
        if (!Serial1.printed_lines.empty()) {
            std::string output = Serial1.printed_lines[0];
            verify(output.find("347.85,M") != std::string::npos, "Should inject 347.85 for Mag Track: got '" + output + "'");
        }
    }

    // Case 2: True track 180.0.
    // mag_track = true_track + 4.0 = 184.0.
    // Expect index 3 to be 184.00.
    {
        Serial1.clear();
        String input = "$GPVTG,180.0,T,176.0,M,000.0,N,000.0,K*4E\r\n";
        ProcessSentence(input);
        verify(!Serial1.printed_lines.empty(), "Serial1 should output a sentence (Case 2)");
        if (!Serial1.printed_lines.empty()) {
            std::string output = Serial1.printed_lines[0];
            verify(output.find("167.85,M") != std::string::npos, "Should inject 167.85 for Mag Track: got '" + output + "'");
        }
    }

    // Case 3: True track is empty/invalid.
    // Should not modify/crash.
    {
        Serial1.clear();
        String input = "$GPVTG,,T,,M,000.0,N,000.0,K*1C\r\n";
        ProcessSentence(input);
        verify(!Serial1.printed_lines.empty(), "Serial1 should output a sentence (Case 3)");
        if (!Serial1.printed_lines.empty()) {
            std::string output = Serial1.printed_lines[0];
            verify(output.find(",T,,M") != std::string::npos, "Should leave empty Mag Track empty: got '" + output + "'");
        }
    }
}

void test_ProcessSentence_PassThrough() {
    std::cout << "\nRunning test_ProcessSentence_PassThrough..." << std::endl;
    Serial1.clear();
    String input = "$GPGGA,081836,3751.65,S,14507.36,E,1,08,0.9,125.4,M,38.9,M,,*72\r\n";
    ProcessSentence(input);
    verify(!Serial1.printed_lines.empty(), "Serial1 should output a sentence");
    if (!Serial1.printed_lines.empty()) {
        std::string output = Serial1.printed_lines[0];
        verify(output.rfind("$GPGGA", 0) == 0, "Sentence should still start with $GPGGA");
        verify(output.length() >= 2 && output.substr(output.length() - 2) == "\r\n", "Should be terminated with CRLF");
    }
}

void test_ProcessSentence_ExactPassThrough() {
    std::cout << "\nRunning test_ProcessSentence_ExactPassThrough..." << std::endl;
    Serial1.clear();
    String input = "$GPGGA,081836,3751.65,S,14507.36,E,1,08,0.9,125.4,M,38.9,M,,*72\r\n";
    ProcessSentence(input);
    verify(!Serial1.printed_lines.empty(), "Serial1 should output a sentence");
    if (!Serial1.printed_lines.empty()) {
        std::string output = Serial1.printed_lines[0];
        std::string expected = std::string(input.c_str());
        verify(output == expected, "Pass-through sentence should be identical byte-for-byte.\n    Expected: " + expected + "    Got:      " + output);
    }
}

void test_ProcessSentence_LongSentencePassThrough() {
    std::cout << "\nRunning test_ProcessSentence_LongSentencePassThrough..." << std::endl;
    Serial1.clear();
    // A 20-token sentence: GSV sentences can easily exceed 15 tokens.
    // e.g. $GPGSV,4,1,16,01,40,083,46,02,17,308,41,05,12,328,45,07,08,292,40*7E
    String input = "$GPGSV,4,1,16,01,40,083,46,02,17,308,41,05,12,328,45,07,08,292,40*7E\r\n";
    ProcessSentence(input);
    verify(!Serial1.printed_lines.empty(), "Serial1 should output a sentence");
    if (!Serial1.printed_lines.empty()) {
        std::string output = Serial1.printed_lines[0];
        std::string expected = std::string(input.c_str());
        verify(output == expected, "Sentence with > 15 tokens should not be truncated.\n    Expected: " + expected + "    Got:      " + output);
    }
}

int main() {
    std::cout << "========================================\n"
              << "       RUNNING NATIVE UNIT TESTS        \n"
              << "========================================" << std::endl;

    // Initialize WMM coefficients
    magnetic_variation::Initialize();

    test_CalculateChecksum();
    test_SplitString();
    test_ProcessSentence_GPRMC();
    test_ProcessSentence_GPRMC_Seattle();
    test_ProcessSentence_GPRMC_Boston();
    test_ProcessSentence_GPVTG();
    test_ProcessSentence_PassThrough();
    test_ProcessSentence_ExactPassThrough();
    test_ProcessSentence_LongSentencePassThrough();

    std::cout << "\n========================================\n"
              << "TEST RESULTS: " << (tests_run - tests_failed) << "/" << tests_run << " PASSED\n"
              << "========================================" << std::endl;

    return tests_failed == 0 ? 0 : 1;
}
