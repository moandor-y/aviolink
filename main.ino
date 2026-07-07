#include <Arduino.h>

// --- CONFIGURATION ---
constexpr int kGpsRxPin = 22;  // From MAX3232 TTL-TX (GPS incoming)
constexpr int kGpsTxPin = 23;  // To MAX3232 TTL-RX (Avionics outgoing)

constexpr float kMagVar = 4.0f;  // Madison area Magnetic Variation (~4.0 deg)
const String kMagDir = "W";      // "W" for West, "E" for East

String input_buffer = "";

// Function declarations
void ProcessSentence(const String& line);
int SplitString(const String& str, char delimiter, String* tokens,
                int max_tokens);
String CalculateChecksum(const String& sentence);

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n--- ESP32-C6 Avionics NMEA Bridge ---");

  // Start Serial1 for GPS in / Avionics out (4800 baud)
  Serial1.begin(4800, SERIAL_8N1, kGpsRxPin, kGpsTxPin);
}

void loop() {
  // Read incoming data from the GPS puck
  while (Serial1.available()) {
    char c = Serial1.read();
    input_buffer += c;

    // Once a full line is received
    if (c == '\n') {
      ProcessSentence(input_buffer);
      input_buffer = "";  // Reset buffer for the next line
    }
  }
}

void ProcessSentence(const String& line) {
  // Create a local copy so we can modify it
  String local_line = line;
  local_line.trim();

  // Strip the old checksum and the '*'
  int star_index = local_line.indexOf('*');
  String payload = local_line;
  if (star_index > 0) {
    payload = local_line.substring(0, star_index);
  }

  // Only tokenize and rebuild if it's GPRMC or GPVTG
  if (payload.substring(0, 6) == "$GPRMC" || payload.substring(0, 6) == "$GPVTG") {
    // Split the sentence by commas into an array
    String tokens[15];
    int num_tokens = SplitString(payload, ',', tokens, 15);

    // --- INJECT MAGNETIC VARIATION INTO $GPRMC ---
    if (tokens[0] == "$GPRMC" && num_tokens >= 12) {
      // In RMC, index 10 is Mag Var value, index 11 is Direction (E/W)
      tokens[10] = String(kMagVar, 1);
      tokens[11] = kMagDir;
    }
    // --- INJECT MAGNETIC TRACK INTO $GPVTG ---
    else if (tokens[0] == "$GPVTG" && num_tokens >= 9) {
      // Index 1 is True Track, Index 3 is Mag Track
      if (tokens[1].length() > 0) {
        float true_track = tokens[1].toFloat();
        float mag_track = true_track;

        // Apply magnetic math
        if (kMagDir == "W") {
          mag_track = true_track + kMagVar;
        } else {
          mag_track = true_track - kMagVar;
        }

        // Wrap around 360 degrees
        if (mag_track >= 360.0f) mag_track -= 360.0f;
        if (mag_track < 0.0f) mag_track += 360.0f;

        tokens[3] = String(mag_track, 2);  // Insert with 2 decimal places
      }
    }

    // Rebuild the payload with commas
    String new_payload = tokens[0];
    for (int i = 1; i < num_tokens; i++) {
      new_payload += "," + tokens[i];
    }

    // Calculate the new checksum
    String new_sentence =
        new_payload + "*" + CalculateChecksum(new_payload) + "\r\n";

    // Broadcast to the Avionics!
    Serial1.print(new_sentence);

    // Print to USB serial so you can verify it is working
    Serial.print("OUT: " + new_sentence);
    return;
  }

  // Pass-through case: broadcast the raw line exactly as received
  Serial1.print(line);
  Serial.print("OUT: " + line);
}

// --- HELPER FUNCTIONS ---

// Splits a string by a delimiter character
int SplitString(const String& str, char delimiter, String* tokens,
                int max_tokens) {
  int token_count = 0;
  int start_index = 0;
  int end_index = 0;
  while (end_index >= 0 && token_count < max_tokens) {
    end_index = str.indexOf(delimiter, start_index);
    if (end_index >= 0) {
      tokens[token_count] = str.substring(start_index, end_index);
      start_index = end_index + 1;
    } else {
      tokens[token_count] = str.substring(start_index);
    }
    token_count++;
  }
  return token_count;
}

// Calculates NMEA checksum (XOR of all bytes between $ and *)
String CalculateChecksum(const String& sentence) {
  int checksum = 0;
  // Start at index 1 to skip the '$'
  for (int i = 1; i < static_cast<int>(sentence.length()); i++) {
    checksum ^= sentence[i];
  }

  // Format as a 2-character uppercase HEX string
  char hex_str[3];
  snprintf(hex_str, sizeof(hex_str), "%02X", checksum);
  return String(hex_str);
}
