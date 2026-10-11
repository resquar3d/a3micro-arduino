/*
 * UNO_R4_WIFI_FACES_AND_TUNES.ino - a robot face on a 1.3" OLED and tunes on a buzzer, from the A3 Micro app
 *
 * Faces (emojis) on an SH1106 1.3" OLED, 128 x 64, drawn by the sketch (no image files), blinking now and then:
 *   happy  sad  love  angry  surprised  sleepy  wink  cool  laugh  neutral
 * Tunes on a piezo buzzer, played in the background (the robot keeps listening while it plays):
 *   twinkle  birthday  joy  jingle  charge  beep               traditional melodies and a start-up beep
 *   droid  voyage  warp  laser  scan                           the robot's own sci-fi sounds: droid chatter
 *                                                              (new every time), a space fanfare, a warp jump,
 *                                                              laser shots, a scanner
 *
 * From the app (any of these):
 *   face:<name> or face:<0-9>     a button with Message ID "face" and On value "love", or a slider "face" 0-9
 *   tune:<name> or tune:<1-11>    a button "tune" / "droid"; tune:next, tune:stop
 *   voice commands (A3 Micro Pro) arrive as vc:<the words>: "happy face", "show love", "play music",
 *                                 "next song", "play birthday", "stop music", "go to sleep", "wake up",
 *                                 "robot talk", "beep beep", "space", "warp", "jump", "laser", "fire", "scan"
 * Your own recordings (MP3) on a DFPlayer Mini, the face talking while they play:
 *   sound:<1-255>, sound:stop      a button "sound" / "1"; vol:<0-30> sets its volume
 *   voice: "play my sound", "my voice", "play sound 2", "louder", "quieter"
 *   On the microSD card (FAT32): a folder named mp3 with your files named 0001.mp3, 0002.mp3, ...
 *   (0001.mp3 is sound 1). Without a DFPlayer the sketch still works: sound:<n> beeps instead.
 * The board tells the app what it shows and plays: face:<name>, tune:<name> and tune:off when it ends,
 * sound:<n> and sound:off.
 * With the app gone it falls asleep (sleepy face); when the app connects it wakes up happy.
 *
 * Board: UNO R4 WiFi, using its own Bluetooth LE radio (with an HM-10 on any board: A3Micro a3(Serial1);).
 * Wiring:  OLED  VCC 3.3V or 5V · GND · SCL to SCL · SDA to SDA   (I2C address 0x3C; see OLED_ADDRESS)
 *          buzzer  + to pin 2 (the UNO_R4_WIFI_HUD_DRIVE robot's horn pin, so the two sketches combine) · - to GND
 *          DFPlayer Mini  VCC 5V · GND · TX to pin 0 (RX) · RX to pin 1 (TX) through a 1 kOhm resistor ·
 *                         SPK_1 and SPK_2 to a small speaker (3 W, 4-8 Ohm)
 * Needs the U8g2 library (Library Manager: "U8g2" by oliver) and DFRobotDFPlayerMini ("DFPlayer Mini" by DFRobot).
 *
 * Copyright (c) 2026 R.E Espino, TwinSparks Development (www.twinsparksdevelopment.com)
 * Released under the MIT License (see LICENSE).
 */

#include <A3Micro.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <DFRobotDFPlayerMini.h>

A3Micro a3;                                     // the UNO R4 WiFi's own Bluetooth LE radio
U8G2_SH1106_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);

const int BUZZER_PIN = 2;
const uint8_t OLED_ADDRESS = 0x3C;              // some modules use 0x3D

// ---------- Faces ----------
const char *FACES[] = { "happy", "sad", "love", "angry", "surprised", "sleepy", "wink", "cool", "laugh", "neutral" };
const int FACE_COUNT = sizeof(FACES) / sizeof(FACES[0]);
int face = 0;                                   // index into FACES
unsigned long nextBlink = 0, blinkUntil = 0;

const int CX = 64, CY = 32;                     // the middle of the screen
bool tunePlaying();                             // (below, with the tunes)
int tuneIndex(String name);

// ---------- Your recordings: a DFPlayer Mini on Serial1 (pins 0 and 1) ----------
DFRobotDFPlayerMini mp3;
bool hasMp3 = false;                            // a DFPlayer answered at start-up
int sound = 0;                                  // the recording playing (0: none)
int mp3Volume = 22;                             // 0-30
unsigned long soundSafety = 0;                  // gives up waiting for "finished" after this time

void eyesOpen(int r = 4) {
  oled.drawFilledEllipse(CX - 11, CY - 8, r - 1, r + 1);
  oled.drawFilledEllipse(CX + 11, CY - 8, r - 1, r + 1);
}
void eyesClosed() {
  oled.drawHLine(CX - 15, CY - 8, 9);
  oled.drawHLine(CX + 7, CY - 8, 9);
}
void smile(int r = 13) {
  oled.drawCircle(CX, CY + 1, r, U8G2_DRAW_LOWER_LEFT | U8G2_DRAW_LOWER_RIGHT);
  oled.drawCircle(CX, CY + 1, r - 1, U8G2_DRAW_LOWER_LEFT | U8G2_DRAW_LOWER_RIGHT);
}
void frown() {
  oled.drawCircle(CX, CY + 21, 10, U8G2_DRAW_UPPER_LEFT | U8G2_DRAW_UPPER_RIGHT);
  oled.drawCircle(CX, CY + 21, 9, U8G2_DRAW_UPPER_LEFT | U8G2_DRAW_UPPER_RIGHT);
}
void heart(int x, int y, int s) {
  oled.drawDisc(x - s / 2, y, s / 2 + 1);
  oled.drawDisc(x + s / 2, y, s / 2 + 1);
  oled.drawTriangle(x - s - 1, y + 1, x + s + 1, y + 1, x, y + s + 3);
}

void drawFace(bool blink) {
  oled.clearBuffer();
  oled.drawCircle(CX, CY, 30);                  // the head
  oled.drawCircle(CX, CY, 29);
  const char *name = FACES[face];
  if (sound) {                                  // a recording playing: the face talks
    eyesOpen();
    oled.drawFilledEllipse(CX, CY + 12, 8, 1 + (millis() / 130) % 3 * 3);
  }
  else if (!strcmp(name, "happy"))      { blink ? eyesClosed() : eyesOpen(); smile(); }
  else if (!strcmp(name, "sad"))   { blink ? eyesClosed() : eyesOpen(); frown();
                                     oled.drawLine(CX - 17, CY - 13, CX - 7, CY - 16); oled.drawLine(CX + 17, CY - 13, CX + 7, CY - 16); }
  else if (!strcmp(name, "love"))  { heart(CX - 11, CY - 9, 5); heart(CX + 11, CY - 9, 5); smile(); }
  else if (!strcmp(name, "angry")) { blink ? eyesClosed() : eyesOpen(3);
                                     oled.drawLine(CX - 18, CY - 17, CX - 6, CY - 12); oled.drawLine(CX + 18, CY - 17, CX + 6, CY - 12);
                                     oled.drawLine(CX - 18, CY - 16, CX - 6, CY - 11); oled.drawLine(CX + 18, CY - 16, CX + 6, CY - 11);
                                     frown(); }
  else if (!strcmp(name, "surprised")) { blink ? eyesClosed() : eyesOpen(5); oled.drawCircle(CX, CY + 13, 6); oled.drawCircle(CX, CY + 13, 5); }
  else if (!strcmp(name, "sleepy")) { eyesClosed(); oled.drawCircle(CX, CY + 13, 3);
                                     oled.setFont(u8g2_font_6x10_tf); oled.drawStr(CX + 26, CY - 18, "Z"); oled.drawStr(CX + 33, CY - 25, "z"); }
  else if (!strcmp(name, "wink"))  { oled.drawFilledEllipse(CX - 11, CY - 8, 3, 5); oled.drawHLine(CX + 7, CY - 8, 9); oled.drawLine(CX + 7, CY - 8, CX + 4, CY - 10); smile(); }
  else if (!strcmp(name, "cool"))  { oled.drawRBox(CX - 23, CY - 13, 19, 10, 3); oled.drawRBox(CX + 4, CY - 13, 19, 10, 3);
                                     oled.drawHLine(CX - 4, CY - 11, 8); oled.drawLine(CX - 23, CY - 12, CX - 29, CY - 15); oled.drawLine(CX + 23, CY - 12, CX + 29, CY - 15);
                                     oled.drawCircle(CX + 2, CY + 3, 12, U8G2_DRAW_LOWER_RIGHT); oled.drawCircle(CX + 2, CY + 3, 11, U8G2_DRAW_LOWER_RIGHT); }
  else if (!strcmp(name, "laugh")) { oled.drawLine(CX - 15, CY - 6, CX - 11, CY - 11); oled.drawLine(CX - 11, CY - 11, CX - 7, CY - 6);
                                     oled.drawLine(CX + 7, CY - 6, CX + 11, CY - 11); oled.drawLine(CX + 11, CY - 11, CX + 15, CY - 6);
                                     oled.drawDisc(CX, CY + 4, 13, U8G2_DRAW_LOWER_LEFT | U8G2_DRAW_LOWER_RIGHT); }
  else                             { blink ? eyesClosed() : eyesOpen(); oled.drawHLine(CX - 9, CY + 12, 19); oled.drawHLine(CX - 9, CY + 13, 19); }
  if (tunePlaying()) {                          // a little note in the corner while a tune plays
    oled.drawDisc(112, 56, 3); oled.drawVLine(115, 44, 12); oled.drawLine(115, 44, 121, 47);
  }
  oled.sendBuffer();
}

void showFace(int i) {
  face = constrain(i, 0, FACE_COUNT - 1);
  drawFace(false);
  nextBlink = millis() + random(2500, 5000);
  a3.send("face", FACES[face]);
}
int faceIndex(String name) {
  name.trim(); name.toLowerCase();
  if (name.length() && isDigit(name[0])) return constrain(name.toInt(), 0, FACE_COUNT - 1);
  for (int i = 0; i < FACE_COUNT; i++) if (name == FACES[i]) return i;
  return -1;
}

// ---------- Tunes, played without stopping the rest of the sketch ----------
// Traditional melodies, then the robot's own sci-fi sounds (original, written for A3Micro): droid chatter (new
// every time), a space fanfare, a warp jump, laser shots and a scanner
struct Note { uint16_t hz, ms, to; };          // hz 0 = a rest · to: slides from hz to this pitch (0 = a plain note)
// Note frequencies (N_ in front: names like D4 and A4 are pins on these boards)
const uint16_t N_C4 = 262, N_D4 = 294, N_E4 = 330, N_F4 = 349, N_G4 = 392, N_A4 = 440, N_B4 = 494, N_C5 = 523, N_D5 = 587, N_E5 = 659, N_G5 = 784;
const Note TWINKLE[] = { {N_C4,300},{N_C4,300},{N_G4,300},{N_G4,300},{N_A4,300},{N_A4,300},{N_G4,600},{N_F4,300},{N_F4,300},{N_E4,300},{N_E4,300},{N_D4,300},{N_D4,300},{N_C4,600} };
const Note BIRTHDAY[] = { {N_G4,225},{N_G4,75},{N_A4,300},{N_G4,300},{N_C5,300},{N_B4,600},{N_G4,225},{N_G4,75},{N_A4,300},{N_G4,300},{N_D5,300},{N_C5,600} };
const Note JOY[] = { {N_E4,300},{N_E4,300},{N_F4,300},{N_G4,300},{N_G4,300},{N_F4,300},{N_E4,300},{N_D4,300},{N_C4,300},{N_C4,300},{N_D4,300},{N_E4,300},{N_E4,450},{N_D4,150},{N_D4,600} };
const Note JINGLE[] = { {N_E4,250},{N_E4,250},{N_E4,500},{N_E4,250},{N_E4,250},{N_E4,500},{N_E4,250},{N_G4,250},{N_C4,250},{N_D4,250},{N_E4,1000} };
const Note CHARGE[] = { {N_G4,150},{N_C5,150},{N_E5,150},{N_G5,300},{0,80},{N_E5,150},{N_G5,600} };
const Note BEEP[] = { {1200,80},{0,60},{1600,80},{0,60},{2000,140},{0,80},{1500,70},{1900,160} };
const uint16_t N_C6 = 1047;
const Note VOYAGE[] = { {N_C5,180},{N_G5,420},{N_E5,180},{N_C5,180},{N_D5,600},{0,120},{N_G4,180},{N_C5,180},{N_E5,240},{N_G5,240},{N_C6,900} };
const Note WARP[] = { {120,900,900},{900,500,2600},{0,60},{2600,90},{0,40},{2600,260} };
const Note LASER[] = { {2800,140,500},{0,60},{2800,140,500},{0,60},{3200,220,350} };
const Note SCAN[] = { {500,700,1600},{1600,700,500},{500,700,1600},{0,80},{1800,90},{0,50},{1800,90} };
Note DROID[16];                                 // filled with new chatter each time it plays (makeDroid)
struct Tune { const char *name; const Note *notes; uint8_t count; };
#define TUNE(n, a) { n, a, sizeof(a) / sizeof(a[0]) }
const Tune TUNES[] = { TUNE("twinkle", TWINKLE), TUNE("birthday", BIRTHDAY), TUNE("joy", JOY), TUNE("jingle", JINGLE),
                       TUNE("charge", CHARGE), TUNE("beep", BEEP), TUNE("droid", DROID), TUNE("voyage", VOYAGE),
                       TUNE("warp", WARP), TUNE("laser", LASER), TUNE("scan", SCAN) };
const int TUNE_COUNT = sizeof(TUNES) / sizeof(TUNES[0]);

int tune = -1, lastTune = -1;                   // the tune playing (-1: none), the last one played
uint8_t noteAt = 0;
unsigned long noteStart = 0, noteEnds = 0, lastSlide = 0;
const Note *slide = nullptr;                    // the note sliding in pitch now

// Droid chatter: quick chirps, whistles that slide up or down, and short pauses, different every time
void makeDroid() {
  for (Note &n : DROID) {
    long kind = random(10);
    if (kind < 2) n = { 0, (uint16_t)random(30, 90), 0 };                                       // a pause
    else if (kind < 6) n = { (uint16_t)random(1400, 3600), (uint16_t)random(35, 90), 0 };      // a chirp
    else n = { (uint16_t)random(900, 2600), (uint16_t)random(80, 220), (uint16_t)random(1200, 3800) };   // a whistle
  }
}

bool tunePlaying() { return tune >= 0; }
void playTune(int i) {
  tune = ((i % TUNE_COUNT) + TUNE_COUNT) % TUNE_COUNT; lastTune = tune;
  noteAt = 0; noteEnds = 0; slide = nullptr;
  if (TUNES[tune].notes == DROID) makeDroid();
  a3.send("tune", TUNES[tune].name);
  drawFace(false);
}
void stopTune() {
  if (!tunePlaying()) return;
  noTone(BUZZER_PIN); tune = -1; slide = nullptr;
  a3.send("tune", "off");
  drawFace(false);
}
void updateTune() {
  if (!tunePlaying()) return;
  unsigned long now = millis();
  if (now < noteEnds) {
    // A sliding note: a new pitch every 8 ms on the way from hz to to
    if (slide && now - lastSlide >= 8) {
      long hz = slide->hz + ((long)slide->to - slide->hz) * (long)(now - noteStart) / slide->ms;
      tone(BUZZER_PIN, (unsigned int)max(31L, hz));
      lastSlide = now;
    }
    return;
  }
  const Tune &t = TUNES[tune];
  if (noteAt >= t.count) { stopTune(); return; }
  const Note &n = t.notes[noteAt++];
  slide = (n.hz && n.to) ? &n : nullptr;
  if (!n.hz) noTone(BUZZER_PIN);
  else if (slide) tone(BUZZER_PIN, n.hz);                                    // held; updateTune slides it
  else tone(BUZZER_PIN, n.hz, n.ms * 9 / 10);
  noteStart = lastSlide = now;
  noteEnds = now + n.ms;
}
void stopSound() {
  if (!sound) return;
  if (hasMp3) mp3.stop();
  sound = 0;
  a3.send("sound", "off");
  drawFace(false);
}
void playSound(int n) {
  n = constrain(n, 1, 255);
  stopTune();
  if (!hasMp3) { a3.send("sound", "no DFPlayer"); playTune(tuneIndex("beep")); return; }
  mp3.playMp3Folder(n);                         // mp3/0001.mp3 is 1
  sound = n; soundSafety = millis() + 5UL * 60 * 1000;
  a3.send("sound", n);
  drawFace(false);
}
void setVolume(int v) {
  mp3Volume = constrain(v, 0, 30);
  if (hasMp3) mp3.volume(mp3Volume);
  a3.send("vol", mp3Volume);
}
// The DFPlayer says when a recording ends; the face stops talking
void updateSound() {
  if (hasMp3 && mp3.available() && mp3.readType() == DFPlayerPlayFinished) { if (sound) { sound = 0; a3.send("sound", "off"); drawFace(false); } }
  if (!sound) return;
  if ((long)(millis() - soundSafety) > 0) { stopSound(); return; }
  static unsigned long lastTalk = 0;
  if (millis() - lastTalk >= 130) { lastTalk = millis(); drawFace(false); }
}

int tuneIndex(String name) {
  name.trim(); name.toLowerCase();
  if (name.length() && isDigit(name[0])) return constrain(name.toInt(), 1, TUNE_COUNT) - 1;
  for (int i = 0; i < TUNE_COUNT; i++) if (name == TUNES[i].name) return i;
  return -1;
}

// ---------- Voice commands: the words themselves ----------
bool says(const String &said, const char *word) { return said.indexOf(word) >= 0; }
void hear(String said) {
  said.toLowerCase();
  if (says(said, "stop")) { stopTune(); stopSound(); return; }           // "stop" / "stop music"
  if (says(said, "louder")) { setVolume(mp3Volume + 4); return; }
  if (says(said, "quieter") || says(said, "softer")) { setVolume(mp3Volume - 4); return; }
  if (says(said, "my sound") || says(said, "my voice") || says(said, "recording") || says(said, "sound")) {
    int n = 1;
    for (unsigned int i = 0; i < said.length(); i++) if (isDigit(said[i])) { n = said.substring(i).toInt(); break; }
    if (says(said, "two")) n = 2; else if (says(said, "three")) n = 3;
    playSound(n); return;
  }
  if (says(said, "robot talk") || says(said, "talk") || says(said, "chatter") || says(said, "beep beep")) { playTune(tuneIndex("droid")); return; }
  if (says(said, "space") || says(said, "fanfare")) { playTune(tuneIndex("voyage")); return; }
  if (says(said, "jump") || says(said, "hyper")) { playTune(tuneIndex("warp")); return; }
  if (says(said, "shoot") || says(said, "fire") || says(said, "pew")) { playTune(tuneIndex("laser")); return; }
  for (int i = 0; i < TUNE_COUNT; i++) if (says(said, TUNES[i].name)) { playTune(i); return; }
  if (says(said, "next") || says(said, "another")) { playTune(lastTune + 1); return; }
  if (says(said, "music") || says(said, "song") || says(said, "play") || says(said, "sing")) { playTune(lastTune < 0 ? 0 : lastTune); return; }
  if (says(said, "sleep") || says(said, "night")) { showFace(faceIndex("sleepy")); return; }
  if (says(said, "wake")) { showFace(faceIndex("happy")); return; }
  for (int i = 0; i < FACE_COUNT; i++) if (says(said, FACES[i])) { showFace(i); return; }
  if (says(said, "smile")) showFace(faceIndex("happy"));
  else if (says(said, "heart") || says(said, "kiss")) showFace(faceIndex("love"));
  else if (says(said, "mad")) showFace(faceIndex("angry"));
  else if (says(said, "wow") || says(said, "surprise")) showFace(faceIndex("surprised"));
  else if (says(said, "funny") || says(said, "haha")) showFace(faceIndex("laugh"));
  else a3.send("heard", "not understood: " + said);
}

void setup() {
  Serial.begin(9600);
  oled.setI2CAddress(OLED_ADDRESS * 2);
  oled.begin();
  oled.clearBuffer();
  oled.setFont(u8g2_font_7x14B_tf);
  oled.drawStr(30, 28, "A3 MICRO");
  oled.setFont(u8g2_font_6x10_tf);
  oled.drawStr(22, 46, "waiting for app");
  oled.sendBuffer();
  if (!a3.begin("Robot Face")) Serial.println("Bluetooth LE did not start");
  Serial1.begin(9600);                          // the DFPlayer Mini
  hasMp3 = mp3.begin(Serial1, true, true);
  if (hasMp3) mp3.volume(mp3Volume); else Serial.println("No DFPlayer Mini (check its wiring and microSD card)");
  randomSeed(analogRead(A3));
  face = faceIndex("sleepy");
}

void loop() {
  // Awake while the app is connected, asleep without it
  static bool wasConnected = false;
  bool connected = a3.connected();
  if (connected != wasConnected) {
    wasConnected = connected;
    if (connected) { showFace(faceIndex("happy")); playTune(tuneIndex("beep")); }
    else { stopTune(); stopSound(); face = faceIndex("sleepy"); drawFace(false); }
  }

  updateTune();
  updateSound();

  // Blink now and then (a short closed-eyes frame)
  unsigned long now = millis();
  if (now >= nextBlink && !blinkUntil) { drawFace(true); blinkUntil = now + 140; }
  if (blinkUntil && now >= blinkUntil) { drawFace(false); blinkUntil = 0; nextBlink = now + random(2500, 5000); }

  A3MicroPacket in;
  if (!a3.receive(in)) return;                 // nothing new yet
  Serial.println(in.toMessage());

  if (in.has("face")) { int i = faceIndex(in.get("face")); if (i >= 0) showFace(i); }
  if (in.has("tune")) {
    String t = in.get("tune"); t.toLowerCase();
    if (t == "stop" || t == "off" || t == "0") stopTune();
    else if (t == "next") playTune(lastTune + 1);
    else { int i = tuneIndex(t); if (i >= 0) playTune(i); }
  }
  if (in.has("sound")) {
    String t = in.get("sound"); t.toLowerCase();
    if (t == "stop" || t == "off" || t == "0") stopSound(); else playSound(t.toInt());
  }
  if (in.has("vol")) setVolume(in.getInt("vol"));
  if (in.has("vc")) hear(in.get("vc"));
}
