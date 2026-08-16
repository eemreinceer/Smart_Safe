/*
 * SMARTSAFE RFID/NFC COMPATIBILITY PROBE
 * ---------------------------------------------------------------
 * Standalone experiment firmware. NOT SmartSafe authentication code.
 *
 * Purpose: answer, experimentally, which real-world personal cards and
 * NFC devices an MFRC522/RC522 reader can see, and whether a stable UID
 * can be obtained from them.
 *
 * Scope limit (deliberate): discovery / anti-collision only.
 * REQA + WUPA, anti-collision, SAK, PICC type. Nothing else.
 * No sector reads, no key attempts, no writes, no APDUs, no EMV.
 *
 * Hardware: standard ESP32 DevKit (ESP32-WROOM-32) + RC522 on VSPI.
 */

#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>

// ---------------------------------------------------------------
// Pin map (ESP32 DevKit, VSPI)
// ---------------------------------------------------------------
#define PIN_SCK   18
#define PIN_MISO  19
#define PIN_MOSI  23
#define PIN_SS     5
#define PIN_RST   22

// ---------------------------------------------------------------
// Timing / anti-spam
// ---------------------------------------------------------------
static const uint32_t POLL_INTERVAL_MS   = 120;  // RF poll period
static const uint8_t  ABSENT_POLLS_GONE  = 4;    // misses before "target removed"
static const uint32_t HEARTBEAT_MS       = 20000;// idle "still alive" line

// A card moving into the field can answer REQA before it is coupled well
// enough to complete anti-collision. Retry activation before believing a
// UID failure, otherwise good cards get misreported as unstable.
static const uint8_t  ACTIVATION_ATTEMPTS = 4;
static const uint16_t ACTIVATION_RETRY_MS = 12;

static const uint8_t  MAX_SAMPLES        = 12;   // stored reads per session
static const uint8_t  TARGET_SAMPLES     = 5;    // reads required for a verdict

MFRC522 mfrc522(PIN_SS, PIN_RST);

// ---------------------------------------------------------------
// One measurement
// ---------------------------------------------------------------
struct Sample {
  bool     detected;      // RF response to REQA/WUPA
  bool     uidValid;      // anti-collision completed, UID obtained
  uint16_t atqa;
  uint8_t  sak;
  uint8_t  uidLen;
  uint8_t  uid[10];
  bool     wokenByWupa;   // needed WUPA instead of REQA
  uint8_t  attempts;      // activation attempts used (1 = first try)
};

// ---------------------------------------------------------------
// Test session state
// ---------------------------------------------------------------
static char     g_label[40] = "unnamed target";
static Sample   g_samples[MAX_SAMPLES];
static uint8_t  g_sampleCount   = 0;
static uint16_t g_detections    = 0;   // incl. samples past MAX_SAMPLES
static uint16_t g_uidReads      = 0;
static uint16_t g_uidFailures   = 0;   // detected but anti-collision failed

// ---------------------------------------------------------------
// Presence tracking
// ---------------------------------------------------------------
static bool     g_targetPresent = false;
static uint8_t  g_absentPolls   = 0;
static uint32_t g_lastPoll      = 0;
static uint32_t g_lastHeartbeat = 0;

// ---------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------
static void printHex2(uint8_t v) {
  if (v < 0x10) Serial.print('0');
  Serial.print(v, HEX);
}

static void printUid(const uint8_t *uid, uint8_t len, char sep) {
  for (uint8_t i = 0; i < len; i++) {
    if (i) Serial.print(sep);
    printHex2(uid[i]);
  }
}

static bool sameUid(const Sample &a, const Sample &b) {
  if (!a.uidValid || !b.uidValid) return false;
  if (a.uidLen != b.uidLen) return false;
  return memcmp(a.uid, b.uid, a.uidLen) == 0;
}

// UID heuristics. A 4-byte UID starting with 0x08 is, per ISO/IEC 14443-3,
// a random ID -- it is expected to change on every activation.
static bool looksRandom(const Sample &s) {
  return s.uidValid && s.uidLen == 4 && s.uid[0] == 0x08;
}

static void resetSession(const char *label) {
  strncpy(g_label, label, sizeof(g_label) - 1);
  g_label[sizeof(g_label) - 1] = '\0';
  g_sampleCount = 0;
  g_detections  = 0;
  g_uidReads    = 0;
  g_uidFailures = 0;
}

// ---------------------------------------------------------------
// Reader self-test / version
// ---------------------------------------------------------------
static void printReaderInfo() {
  uint8_t v = mfrc522.PCD_ReadRegister(MFRC522::VersionReg);
  Serial.print(F("RC522 VersionReg : 0x"));
  printHex2(v);
  Serial.print(F("  ("));
  switch (v) {
    case 0x88: Serial.print(F("clone / FM17522"));  break;
    case 0x90: Serial.print(F("MFRC522 v0.0"));     break;
    case 0x91: Serial.print(F("MFRC522 v1.0"));     break;
    case 0x92: Serial.print(F("MFRC522 v2.0"));     break;
    case 0xB2: Serial.print(F("clone / FM17522E")); break;
    default:   Serial.print(F("unknown"));          break;
  }
  Serial.println(F(")"));

  if (v == 0x00 || v == 0xFF) {
    Serial.println(F("!! Reader not responding. Check 3.3V, GND and SPI wiring."));
    Serial.println(F("!! Do NOT power the RC522 from 5V."));
  }

  uint8_t gain = (mfrc522.PCD_GetAntennaGain() >> 4) & 0x07;
  Serial.print(F("Antenna gain     : 0x"));
  Serial.print(gain, HEX);
  Serial.println(F(" (7 = max, 48 dB)"));
}

// One REQA/WUPA + anti-collision attempt.
static bool probeAttempt(Sample &s) {
  uint8_t atqa[2];
  uint8_t len = sizeof(atqa);

  MFRC522::StatusCode st = mfrc522.PICC_RequestA(atqa, &len);

  // Cards left in HALT state (common after a previous read, and the state
  // many ISO14443-4 cards sit in) only answer WUPA, not REQA.
  if (st != MFRC522::STATUS_OK && st != MFRC522::STATUS_COLLISION) {
    len = sizeof(atqa);
    st = mfrc522.PICC_WakeupA(atqa, &len);
    if (st == MFRC522::STATUS_OK || st == MFRC522::STATUS_COLLISION) {
      s.wokenByWupa = true;
    }
  }

  if (st != MFRC522::STATUS_OK && st != MFRC522::STATUS_COLLISION) {
    return false;   // no RF answer at all
  }

  s.detected = true;
  s.atqa = (uint16_t)atqa[0] | ((uint16_t)atqa[1] << 8);

  // Anti-collision + select. This is where randomized-UID and
  // "sees the field but will not complete activation" devices drop out.
  MFRC522::StatusCode sel = mfrc522.PICC_Select(&mfrc522.uid, 0);
  if (sel == MFRC522::STATUS_OK && mfrc522.uid.size > 0) {
    s.uidValid = true;
    s.uidLen   = mfrc522.uid.size;
    memcpy(s.uid, mfrc522.uid.uidByte, min((size_t)mfrc522.uid.size, sizeof(s.uid)));
    s.sak = mfrc522.uid.sak;
  }

  mfrc522.PICC_HaltA();
  return true;
}

// ---------------------------------------------------------------
// One RF probe cycle.
//
// A card entering the field is only partially coupled for a few
// milliseconds: it can answer REQA while anti-collision still fails.
// Recording that as "UID not readable" would be a measurement artifact,
// not a property of the card, so activation is retried before a failure
// is believed. Only a target that answers repeatedly and still refuses
// to complete activation is reported as detected-without-UID.
// ---------------------------------------------------------------
static bool probeOnce(Sample &s) {
  memset(&s, 0, sizeof(s));

  bool detectedAny = false;

  for (uint8_t i = 0; i < ACTIVATION_ATTEMPTS; i++) {
    Sample attempt;
    memset(&attempt, 0, sizeof(attempt));

    if (probeAttempt(attempt)) {
      detectedAny = true;
      attempt.attempts = i + 1;
      s = attempt;
      if (attempt.uidValid) return true;   // good read, done
    }

    if (i + 1 < ACTIVATION_ATTEMPTS) delay(ACTIVATION_RETRY_MS);
  }

  return detectedAny;
}

// ---------------------------------------------------------------
// Reporting
// ---------------------------------------------------------------
static void reportMeasurement(const Sample &s, const Sample *prev) {
  uint16_t index = g_detections;   // already incremented by caller

  Serial.println(F("----------------------------------------"));
  Serial.println(F("NEW RFID/NFC TARGET"));
  Serial.print  (F("Label         : ")); Serial.println(g_label);
  Serial.print  (F("Measurement # ")); Serial.println(index);
  Serial.println(F("Card detected : YES"));

  Serial.print  (F("ATQA          : 0x"));
  printHex2((uint8_t)(s.atqa >> 8)); printHex2((uint8_t)(s.atqa & 0xFF));
  Serial.println();

  Serial.print  (F("Wake command  : "));
  Serial.println(s.wokenByWupa ? F("WUPA (0x52)") : F("REQA (0x26)"));

  Serial.print  (F("Activation    : "));
  Serial.print(s.attempts);
  Serial.print(F(" / "));
  Serial.print(ACTIVATION_ATTEMPTS);
  Serial.println(s.attempts > 1 ? F(" attempts (weak coupling)") : F(" attempts"));

  if (s.uidValid) {
    Serial.print(F("UID           : "));
    printUid(s.uid, s.uidLen, ':');
    Serial.println();
    Serial.print(F("UID length    : ")); Serial.print(s.uidLen); Serial.println(F(" bytes"));
    Serial.print(F("SAK           : 0x")); printHex2(s.sak); Serial.println();

    MFRC522::PICC_Type type = mfrc522.PICC_GetType(s.sak);
    Serial.print(F("PICC type     : "));
    Serial.println(mfrc522.PICC_GetTypeName(type));

    // The library's SAK table only covers a handful of values and returns
    // "Unknown type" for common composite ones (0x28 = SmartMX-style
    // ISO14443-4 + MIFARE emulation, seen on payment cards). The SAK bit
    // fields are defined by ISO/IEC 14443-3, so decode them directly.
    Serial.print(F("SAK decode    : ISO14443-4 "));
    Serial.print((s.sak & 0x20) ? F("yes") : F("no"));
    Serial.print(F(", ISO18092/NFC "));
    Serial.print((s.sak & 0x40) ? F("yes") : F("no"));
    Serial.print(F(", cascade "));
    Serial.println((s.sak & 0x04) ? F("incomplete") : F("complete"));

    if (looksRandom(s)) {
      Serial.println(F("NOTE          : 4-byte UID starting with 0x08 = random ID"));
      Serial.println(F("                per ISO14443-3. Expect it to change."));
    }
  } else {
    Serial.println(F("UID           : NOT READABLE"));
    Serial.println(F("UID length    : -"));
    Serial.println(F("SAK           : -"));
    Serial.println(F("PICC type     : UNKNOWN (anti-collision did not complete)"));
    Serial.println(F("NOTE          : RF/ISO14443 answer present, activation failed."));
  }

  // Live comparison against the previous stored sample.
  if (prev != nullptr) {
    Serial.print(F("vs previous   : "));
    if (s.uidValid && prev->uidValid) {
      if (sameUid(*prev, s)) {
        Serial.println(F("SAME UID"));
      } else {
        // Either the target really has a changing UID, or a different
        // object was presented without starting a new session. Mixing two
        // objects in one session fakes an "unstable UID" verdict, so say
        // so here rather than letting the report draw a wrong conclusion.
        Serial.println(F("*** UID CHANGED ***"));
        Serial.println(F("!! Different object? Start a new session with"));
        Serial.println(F("!! 'n <label>' before testing another target."));
        Serial.println(F("!! Same object => this target has an unstable UID."));
      }
    } else {
      Serial.println(F("n/a (a UID was missing)"));
    }
  }

  Serial.print(F("Progress      : "));
  Serial.print(g_detections);
  Serial.print(F(" / "));
  Serial.print(TARGET_SAMPLES);
  Serial.println(F(" reads"));

  Serial.println(F("Remove target and present again for"));
  Serial.println(F("stability testing.  ('r' = report)"));
  Serial.println(F("----------------------------------------"));
}

static void reportSession() {
  Serial.println();
  Serial.println(F("========================================"));
  Serial.println(F("STABILITY REPORT"));
  Serial.print  (F("Target        : ")); Serial.println(g_label);
  Serial.print  (F("Detections    : ")); Serial.println(g_detections);
  Serial.print  (F("UID reads     : ")); Serial.println(g_uidReads);
  Serial.print  (F("UID failures  : ")); Serial.println(g_uidFailures);
  Serial.println(F("----------------------------------------"));

  for (uint8_t i = 0; i < g_sampleCount; i++) {
    Serial.print(F("UID #"));
    Serial.print(i + 1);
    Serial.print(F(" : "));
    if (g_samples[i].uidValid) {
      printUid(g_samples[i].uid, g_samples[i].uidLen, ' ');
      Serial.print(F("   ("));
      Serial.print(g_samples[i].uidLen);
      Serial.print(F(" bytes, SAK 0x"));
      printHex2(g_samples[i].sak);
      Serial.print(F(", ATQA 0x"));
      printHex2((uint8_t)(g_samples[i].atqa >> 8));
      printHex2((uint8_t)(g_samples[i].atqa & 0xFF));
      Serial.print(F(")"));
      if (g_samples[i].attempts > 1) {
        Serial.print(F("  [needed "));
        Serial.print(g_samples[i].attempts);
        Serial.print(F(" attempts]"));
      }
      Serial.println();
    } else {
      Serial.println(F("<detected, UID not readable>"));
    }
  }
  Serial.println(F("----------------------------------------"));

  const bool detected     = (g_detections > 0);
  const bool uidReadable  = (g_uidReads > 0);

  // Two different failure modes, deliberately kept apart:
  //   uidConsistent -- every UID we DID read was identical
  //   uidStable     -- consistent AND every detection produced a UID
  // A card that reads intermittently but always returns the same UID is a
  // coupling problem; a card that returns different UIDs is a different
  // problem entirely. Conflating them blames the wrong thing.
  const Sample *first = nullptr;
  bool uidConsistent = true;
  for (uint8_t i = 0; i < g_sampleCount; i++) {
    if (!g_samples[i].uidValid) continue;
    if (first == nullptr) { first = &g_samples[i]; continue; }
    if (!sameUid(*first, g_samples[i])) uidConsistent = false;
  }

  const bool uidStable = uidReadable && uidConsistent &&
                         (g_uidFailures == 0) && (g_sampleCount >= 2);

  bool randomUid = false;
  for (uint8_t i = 0; i < g_sampleCount; i++) {
    if (looksRandom(g_samples[i])) randomUid = true;
  }

  Serial.print(F("DETECTED      : ")); Serial.println(detected    ? F("YES") : F("NO"));
  Serial.print(F("UID READABLE  : "));
  if (!uidReadable) {
    Serial.println(F("NO"));
  } else {
    Serial.print(F("YES ("));
    Serial.print(g_uidReads);
    Serial.print(F("/"));
    Serial.print(g_detections);
    Serial.println(F(" detections)"));
  }
  Serial.print(F("UID CONSISTENT: "));
  if (!uidReadable)           Serial.println(F("n/a"));
  else if (g_uidReads < 2)    Serial.println(F("UNKNOWN (one UID read)"));
  else                        Serial.println(uidConsistent ? F("YES (all reads agreed)")
                                                           : F("NO (UIDs differed)"));
  Serial.print(F("UID STABLE    : "));
  if (!uidReadable)                Serial.println(F("NO (no UID)"));
  else if (g_sampleCount < 2)      Serial.println(F("UNKNOWN (need >= 2 reads)"));
  else                             Serial.println(uidStable ? F("YES") : F("NO"));

  Serial.println(F("----------------------------------------"));
  Serial.print(F("CLASS         : "));
  if (!detected) {
    Serial.println(F("C"));
    Serial.println(F("  Not reachable with RC522 + this ISO14443A probe."));
    Serial.println(F("  This does NOT mean the device has no NFC."));
  } else if (uidStable && g_detections >= TARGET_SAMPLES) {
    Serial.println(F("A"));
    Serial.println(F("  Detected, UID readable, UID stable."));
    Serial.println(F("  Usable as a SmartSafe V1 credential."));
  } else if (uidStable) {
    Serial.print  (F("A (provisional - only "));
    Serial.print(g_detections);
    Serial.println(F(" reads)"));
    Serial.print  (F("  Present it "));
    Serial.print(TARGET_SAMPLES - g_detections);
    Serial.println(F(" more time(s) to confirm."));
  } else {
    Serial.println(F("B"));
    Serial.println(F("  Visible to the RC522, but not a reliable UID source."));

    if (randomUid) {
      Serial.println(F("  Reason: randomized UID (0x08 prefix)."));
      Serial.println(F("  This is by design, not a fault: the card issues a"));
      Serial.println(F("  fresh ID per activation to prevent tracking."));
      Serial.println(F("  No reader can turn this into a stable credential."));
    }

    if (!uidConsistent) {
      Serial.println(F("  Reason: UID differed between reads."));
      // The 0x08 prefix already proves the changing UID came from the
      // card itself, so the mixed-session warning would only mislead.
      if (!randomUid) {
        Serial.println(F("  CHECK: was more than one object tested in this"));
        Serial.println(F("  session? If so this verdict is invalid - press"));
        Serial.println(F("  'n <label>' and test the object on its own."));
      }
    }

    if (g_uidFailures) {
      Serial.print  (F("  Reason: activation failed on "));
      Serial.print(g_uidFailures);
      Serial.print(F(" of "));
      Serial.print(g_detections);
      Serial.println(F(" detections."));
      if (uidConsistent && g_uidReads >= 2) {
        // Worth separating: every UID that came through agreed, so the
        // card is not handing out changing identifiers. Intermittent
        // activation on this clone reader is at least as likely to be a
        // coupling problem as a property of the card.
        Serial.println(F("  NOTE: every UID that WAS read agreed. This looks"));
        Serial.println(F("  like intermittent coupling, not a changing UID."));
        Serial.println(F("  Retest holding the target still and centred"));
        Serial.println(F("  before recording a class B result."));
      }
    }
  }
  Serial.println(F("========================================"));
  Serial.println();
}

static void printHelp() {
  Serial.println(F("========================================"));
  Serial.println(F("SMARTSAFE RFID/NFC COMPATIBILITY PROBE"));
  Serial.println(F("Discovery / UID level only. No card data"));
  Serial.println(F("is read, no keys are tried, nothing is"));
  Serial.println(F("written to any card."));
  Serial.println(F("----------------------------------------"));
  Serial.println(F("Serial commands:"));
  Serial.println(F("  n<label>  new test session, e.g. n izmirim"));
  Serial.println(F("  r         stability report + A/B/C verdict"));
  Serial.println(F("  v         reader version / self test"));
  Serial.println(F("  h         this help"));
  Serial.println(F("----------------------------------------"));
  Serial.print  (F("Present a target "));
  Serial.print(TARGET_SAMPLES);
  Serial.println(F(" times, lifting it away"));
  Serial.println(F("between reads, then press 'r'."));
  Serial.println(F("========================================"));
}

// ---------------------------------------------------------------
// Serial command handling
// ---------------------------------------------------------------
static void handleSerial() {
  static char line[48];
  static uint8_t idx = 0;

  while (Serial.available()) {
    char c = (char)Serial.read();

    // Accept CR, LF or CRLF as the line terminator: terminal programs
    // disagree about which one the Enter key sends, and a command that
    // silently never runs is worse than a stray blank line.
    if (c != '\r' && c != '\n') {
      if (idx < sizeof(line) - 1) line[idx++] = c;
      continue;
    }
    line[idx] = '\0';
    uint8_t used = idx;
    idx = 0;
    if (used == 0) continue;   // also swallows the LF of a CRLF pair

    switch (line[0]) {
      case 'n': case 'N': {
        const char *label = line + 1;
        while (*label == ' ') label++;
        resetSession(*label ? label : "unnamed target");
        Serial.println();
        Serial.println(F("========================================"));
        Serial.print  (F("NEW TEST SESSION : ")); Serial.println(g_label);
        Serial.print  (F("Present the target "));
        Serial.print(TARGET_SAMPLES);
        Serial.println(F(" times."));
        Serial.println(F("========================================"));
        break;
      }
      case 'r': case 'R':
        reportSession();
        break;
      case 'v': case 'V':
        printReaderInfo();
        break;
      default:
        printHelp();
        break;
    }
  }
}

// ---------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(400);

  SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI, PIN_SS);
  mfrc522.PCD_Init();
  delay(50);
  mfrc522.PCD_SetAntennaGain(MFRC522::RxGain_max);
  mfrc522.PCD_AntennaOn();

  Serial.println();
  printHelp();
  printReaderInfo();
  Serial.print(F("Pins             : SCK "));  Serial.print(PIN_SCK);
  Serial.print(F("  MISO "));                  Serial.print(PIN_MISO);
  Serial.print(F("  MOSI "));                  Serial.print(PIN_MOSI);
  Serial.print(F("  SS "));                    Serial.print(PIN_SS);
  Serial.print(F("  RST "));                   Serial.println(PIN_RST);
  Serial.println(F("Waiting for a target..."));

  resetSession("unnamed target");
  g_lastHeartbeat = millis();
}

void loop() {
  handleSerial();

  uint32_t now = millis();
  if (now - g_lastPoll < POLL_INTERVAL_MS) return;
  g_lastPoll = now;

  Sample s;
  bool present = probeOnce(s);

  if (present) {
    g_absentPolls = 0;

    // Anti-spam: a target held on the antenna is one measurement.
    // A new measurement starts only after the target has been removed.
    if (g_targetPresent) return;
    g_targetPresent = true;

    const Sample *prev = (g_sampleCount > 0) ? &g_samples[g_sampleCount - 1] : nullptr;

    g_detections++;
    if (s.uidValid) g_uidReads++; else g_uidFailures++;
    if (g_sampleCount < MAX_SAMPLES) g_samples[g_sampleCount++] = s;

    reportMeasurement(s, prev);
    g_lastHeartbeat = now;

    if (g_detections == TARGET_SAMPLES) {
      Serial.println(F(">> 5 reads collected. Press 'r' for the verdict."));
    }
    return;
  }

  if (g_targetPresent) {
    if (++g_absentPolls >= ABSENT_POLLS_GONE) {
      g_targetPresent = false;
      g_absentPolls = 0;
      Serial.println(F("[target removed]"));
      g_lastHeartbeat = now;
    }
    return;
  }

  if (now - g_lastHeartbeat >= HEARTBEAT_MS) {
    g_lastHeartbeat = now;
    Serial.print(F("[idle] waiting for a target - session: "));
    Serial.print(g_label);
    Serial.print(F(", reads so far: "));
    Serial.println(g_detections);
  }
}
