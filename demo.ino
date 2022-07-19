#include <math.h>

#include <Rainbowduino.h>

const int totalSentences = 2;
const char *sentences[totalSentences] = {
  "you are wonderful <3 <3 <3  ",
  "<3 <3 <3    "
};

const unsigned char minFrameTime = 16;
const unsigned char scrollTime = 80;

const unsigned char totalPrograms = 8;
const unsigned int transitionTime = 6000;

float maxBrightness = 1;
float minBrightness = 0.2;

long uptime;
float brightness;

int clamp(int mn, int mx, int val) {
  return max(mn, min(mx, val));
}

float fclamp(float mn, float mx, float val) {
  return fmax(mn, fmin(mx, val));
}

int absColor(float multi) {
  return 255 * multi * brightness;
}

int scaledColor(float multi) {
  return (128 + 127 * multi) * brightness;
}

template<typename T>
inline T fastCos(T x) noexcept {
  constexpr T tp = 1. / (2.*M_PI);
  x *= tp;
  x -= T(.25) + floor(x + T(.25));
  x *= T(16.) * (abs(x) - T(.5));
  return x;
}

float cosAlgo(int seed, int divisor, float constant) {
  return fastCos(seed % divisor / (float)divisor * 2 * PI + constant);
}

float sinBetween(float min, float max, float t) {
  return ((max - min) * sin(t) + max + min) / 2;
}

int cosColor(int seed, int divisor, float constant) {
  return scaledColor(cosAlgo(seed, divisor, constant));
}

void clearBuffer(unsigned char r[][8], unsigned char g[][8], unsigned char b[][8]) {
  for (int x = 0; x < 8; x++) {
    for (int y = 0; y < 8; y++) {
      r[x][y] = 0;
      g[x][y] = 0;
      b[x][y] = 0;
    }
  }
}

void fillMask(unsigned char mask[][8], unsigned char value) {
  for (int x = 0; x < 8; x++) {
    for (int y = 0; y < 8; y++) {
      mask[x][y] = value;
    }
  }
}

void setCharMask(unsigned char ascii, int poX, int poY, unsigned char mask[][8]) {
  if ((ascii < 0x20) || (ascii > 0x7e)) {
    // Unsupported char.
    ascii = '?';
  }
  for (int i = 0; i < 8; i++) {
    int line = pgm_read_byte(&simpleFont[ascii - 0x20][i]);
    for (int f = 0; f < 8; f++) {
      int y = poY + f;
      int x = poX + i;
      if (x >= 0 && x < 8 && y >= 0 && y < 8) {
        if ((line >> f) & 0x01) {
          mask[y][x] = 0xFF;
        }
      }
    }
  }
}

boolean scrollText(unsigned long tElapsed, unsigned char mask[][8], char const *str) {
  fillMask(mask, 0);

  unsigned char len = strlen(str);
  unsigned int scrollOffset = (tElapsed / scrollTime) % (len * 5); // Yeah char lengths are not counted properly here, doesn't matter

  unsigned char i;
  int charX = -scrollOffset + 8;
  for (i = 0; i < len; i++) {
    if (charX > 8) {
      break;
    }
    if (charX > -8) {
      setCharMask(str[i], charX, 0, mask);
    }

    unsigned char charLength;
    switch (str[i]) {
      case ' ':
        charLength = 4;
        break;
      case 'd':
      case 'w':
        charLength = 6;
        break;
      default:
        charLength = 5;
    }
    charX = charX + charLength;
  }

  return i == len; // Means scroll was finished
}

void setColorGradient(int seed, unsigned char r[][8], unsigned char g[][8], unsigned char b[][8]) {
  r[0][0] = cosColor(seed, 29, 0);
  g[0][0] = cosColor(seed, 53, 2);
  b[0][0] = cosColor(seed, 37, 4);

  for (int x = 7; x >= 0; x--) {
    for (int y = 7; y > 0; y--) {
      r[x][y] = r[x][y - 1];
      g[x][y] = g[x][y - 1];
      b[x][y] = b[x][y - 1];
    }
  }
  for (int x = 7; x > 0; x--) {
    r[x][0] = r[x - 1][0];
    g[x][0] = g[x - 1][0];
    b[x][0] = b[x - 1][0];
  }
}

void setFullScreenColor(int seed, unsigned char r[][8], unsigned char g[][8], unsigned char b[][8]) {
  int vr = cosColor(seed, 29, 0);
  int vg = cosColor(seed, 53, 2);
  int vb = cosColor(seed, 37, 4);
  for (int x = 0; x < 8; x++) {
    for (int y = 0; y < 8; y++) {
      r[x][y] = vr;
      g[x][y] = vg;
      b[x][y] = vb;
    }
  }
}

void setColorGradient2(int seed, unsigned char r[][8], unsigned char g[][8], unsigned char b[][8]) {
  for (int x = 0; x < 8; x++) {
    for (int y = 0; y < 8; y++) {
      r[x][y] = cosColor(seed, 29, x + y);
      g[x][y] = cosColor(seed, 13, x + y + 2);
      b[x][y] = cosColor(seed, 19, x + y + 4);
    }
  }
}

void setTwisterGradient(int seed, unsigned char r[][8], unsigned char g[][8], unsigned char b[][8]) {
  int dy = clamp(0, 7, round((cosAlgo(seed, 50, 0) + 1) * 3.5));
  for (int y = 0; y < 8; y++) {
    float d = (sin(y / 7.0) + 1) * 4;
    int yc = clamp(0, 7, (y + dy) % 8);
    r[0][yc] = cosColor(seed, 29, d / 1.53 + 2);
    g[0][yc] = cosColor(seed, 53, d / 1.19 + 4);
    b[0][yc] = cosColor(seed, 37, d / 1.79 + 0);
  }

  const int dmul = 2;
  int yd = clamp(-dmul, dmul, round(cosAlgo(seed, 79, 0) * dmul));
  for (int x = 7; x > 0; x--) {
    for (int y = 0; y < 8; y++) {
      int yc = clamp(0, 7, (y + yd) % 8);
      r[x][y] = r[x - 1][yc];
      g[x][y] = g[x - 1][yc];
      b[x][y] = b[x - 1][yc];
    }
  }
}

unsigned char setBgProgram(int step, unsigned char r[][8], unsigned char g[][8], unsigned char b[][8]) {
  int program = (uptime / transitionTime % totalPrograms);
  switch (program) {
    case 0:
      setTwisterGradient(step, r, g, b);
      break;
    case 1:
      setTwisterGradient(step, r, g, b);
      break;
    case 2:
      setColorGradient2(step, r, g, b);
      break;
    case 3:
      setColorGradient(step, r, g, b);
      break;
    case 4:
      setTwisterGradient(step, r, g, b);
      break;
    case 5:
      setTwisterGradient(step, r, g, b);
      break;
    case 6:
      setFullScreenColor(step, r, g, b);
      break;
    case 7:
      setTwisterGradient(step, r, g, b);
      break;
  }
  return program;
}

void setExpander(unsigned char mask[][8]) {
  int state = 3 - (uptime / 80 % 5);
  for (int x = 0; x < 8; x++) {
    for (int y = 0; y < 8; y++) {
      if ((x == state || x == 7 - state) && y >= state && y <= 7 - state) {
        mask[x][y] = 0xFF;
      }
      if ((y == state || y == 7 - state) && x >= state && x <= 7 - state) {
        mask[x][y] = 0xFF;
      }
    }
  }
}

void setExpander2(unsigned char mask[][8]) {
  int state = 3 - (uptime / 80 % 5);
  for (int x = 0; x < 8; x++) {
    for (int y = 0; y < 8; y++) {
      if (x >= state && x <= 7 - state && y >= state && y <= 7 - state) {
        mask[x][y] = 0xFF;
      }
      if (y >= state && y <= 7 - state && x >= state && x <= 7 - state) {
        mask[x][y] = 0xFF;
      }
    }
  }
}

void setExpanderY(unsigned char mask[][8]) {
  int state = 3 - (uptime / 80 % 5);
  for (int x = 0; x < 8; x++) {
    for (int y = 0; y < 8; y++) {
      if (x == state || x == 7 - state) {
        mask[x][y] = 0xFF;
      }
    }
  }
}

void setScanX(unsigned char mask[][8]) {
  int state = abs(8 - uptime / 500 % 16);
  for (int x = 0; x < 8; x++) {
    for (int y = 0; y < 8; y++) {
      if (y == state) {
        mask[x][y] = 0xFF;
      }
    }
  }
}

unsigned char setFgProgram(unsigned char mask[][8]) {
  if (uptime < 300000) {
    fillMask(mask, 0xFF);
    return -1;
  }

  fillMask(mask, 0);
  int program = (uptime / 2000 % 24);
  switch (program) {
    case 8:
      setExpanderY(mask);
      break;
    case 13:
      setExpander(mask);
      break;
    case 14:
      setExpander2(mask);
      break;
    case 18:
    case 19:
    case 20:
    case 21:
      setScanX(mask);
      break;
    default:
      fillMask(mask, 0xFF);
      break;
  }
  return program;
}

void loop() {
  unsigned char r[8][8], g[8][8], b[8][8];
  unsigned char mask[8][8];

  long programStart = millis();
  clearBuffer(r, g, b);

  int previousProgram = 0;
  long scrollStart = -1;

  for (unsigned int i = 0; i < 56869; i++) {
    unsigned long frameStart = millis();
    uptime = frameStart - programStart;

    if (uptime > 900000) {
      minBrightness = 0.1;
    }
    maxBrightness = fclamp(minBrightness, 1, 1 - uptime / 1800000.0);
    brightness = minBrightness + (maxBrightness - minBrightness) * pow(1 - pow(((uptime + 1500) % 3000 / 1500.0) - 1, 2), 4);

    if (scrollStart < 0) {
      unsigned char currentProgram = setBgProgram(i, r, g, b);
      if (currentProgram == 0 && previousProgram != 0) {
        scrollStart = frameStart;
      }
      previousProgram = currentProgram;

      setFgProgram(mask);
    } else {
      setFullScreenColor(i, r, g, b);
      int randSentence = scrollStart % totalSentences;
      boolean done = scrollText(frameStart - scrollStart, mask, sentences[randSentence]);
      if (done) {
        scrollStart = -1;
      }
    }

    for (int x = 0; x < 8; x++) {
      for (int y = 0; y < 8; y++) {
        Rb.setPixelXY(
          y,
          7 - x,
          r[x][y] & mask[x][y],
          g[x][y] & mask[x][y],
          b[x][y] & mask[x][y]
        );
      }
    }

    unsigned long frameTime = millis() - frameStart;
    delay(clamp(0, minFrameTime, minFrameTime - frameTime));
  }
}

void setup() {
  Rb.init();
  randomSeed(analogRead(0));
}
