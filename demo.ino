#include <math.h>

#include <Rainbowduino.h>

const int totalSentences = 2;
const char *sentences[totalSentences] = {
  "you are wonderful <3 <3 <3  ",
  "<3 <3 <3    "
};

const unsigned int minFrameTime = 16;
const unsigned int scrollTime = 80;

const unsigned int programCount = 6;
const unsigned int transitionTime = 10000;

float maxBrightness = 1;
float minBrightness = 0.15;

long uptime;
float brightness;

int clamp(int mn, int mx, int val) {
  return max(mn, min(mx, val));
}

float fclamp(float mn, float mx, float val) {
  return fmax(mn, fmin(mx, val));
}

float sinAlgo(int seed, int divisor, float constant) {
  return sin(seed % divisor / (float)divisor * 2 * 3.14159 + constant);
}

float sinBetween(float min, float max, float t) {
  return ((max - min) * sin(t) + max + min) / 2;
}

int scaleColor(float multi) {
  return (128 + 127 * multi) * brightness;
}

int sinColor(int seed, int divisor, float constant) {
  return scaleColor(sinAlgo(seed, divisor, constant));
}

void clearBuffer(unsigned int r[][8], unsigned int g[][8], unsigned int b[][8]) {
  for (int x = 0; x < 8; x++) {
    for (int y = 0; y < 8; y++) {
      r[x][y] = 0;
      g[x][y] = 0;
      b[x][y] = 0;
    }
  }
}

void fillMask(unsigned int mask[][8], unsigned int value) {
  for (int x = 0; x < 8; x++) {
    for (int y = 0; y < 8; y++) {
      mask[x][y] = value;
    }
  }
}

void setCharMask(unsigned char ascii, int poX, int poY, unsigned int mask[][8]) {
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
        } else {
          //mask[y][x] = 0x00;
        }
      }
    }
  }
}

boolean scrollText(unsigned int tElapsed, unsigned int mask[][8], char const *str) {
  fillMask(mask, 0);

  unsigned int len = strlen(str);
  unsigned int scrollOffset = (tElapsed / scrollTime) % (len * 5); // Yeah char lengths are not counted properly here, doesn't matter

  unsigned int i;
  int charX = -scrollOffset + 8;
  for (i = 0; i < len; i++) {
    if (charX > 8) {
      break;
    }
    if (charX > -8) {
      setCharMask(str[i], charX, 0, mask);
    }

    unsigned int charLength;
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

void setColorGradient(int seed, unsigned int r[][8], unsigned int g[][8], unsigned int b[][8]) {
  r[0][0] = sinColor(seed, 29, 0);
  g[0][0] = sinColor(seed, 53, 2);
  b[0][0] = sinColor(seed, 37, 4);

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

void setFullScreenColor(int seed, unsigned int r[][8], unsigned int g[][8], unsigned int b[][8]) {
  int vr = sinColor(seed, 29, 0);
  int vg = sinColor(seed, 53, 2);
  int vb = sinColor(seed, 37, 4);
  for (int x = 0; x < 8; x++) {
    for (int y = 0; y < 8; y++) {
      r[x][y] = vr;
      g[x][y] = vg;
      b[x][y] = vb;
    }
  }
}

void setColorGradient2(int seed, unsigned int r[][8], unsigned int g[][8], unsigned int b[][8]) {
  for (int x = 0; x < 8; x++) {
    for (int y = 0; y < 8; y++) {
      r[x][y] = sinColor(seed, 29, x + y);
      g[x][y] = sinColor(seed, 13, x + y + 2);
      b[x][y] = sinColor(seed, 19, x + y + 4);
    }
  }
}

void setTwisterGradient(int seed, unsigned int r[][8], unsigned int g[][8], unsigned int b[][8]) {
  int dy = clamp(0, 7, round((sinAlgo(seed, 50, 0) + 1) * 3.5));
  for (int y = 0; y < 8; y++) {
    float d = (sin(y / 7.0) + 1) * 4;
    int yc = clamp(0, 7, (y + dy) % 8);
    r[0][yc] = sinColor(seed, 29, d / 1.53 + 2);
    g[0][yc] = sinColor(seed, 53, d / 1.19 + 4);
    b[0][yc] = sinColor(seed, 37, d / 1.79 + 0);
  }

  const int dmul = 2;
  int yd = clamp(-dmul, dmul, round(sinAlgo(seed, 79, 0) * dmul));
  for (int x = 7; x > 0; x--) {
    for (int y = 0; y < 8; y++) {
      int yc = clamp(0, 7, (y + yd) % 8);
      r[x][y] = r[x - 1][yc];
      g[x][y] = g[x - 1][yc];
      b[x][y] = b[x - 1][yc];
    }
  }
}

unsigned int setBgProgram(int step, unsigned int r[][8], unsigned int g[][8], unsigned int b[][8]) {
  int program = (uptime / transitionTime % programCount);
  switch (program) {
    case 0:
      setTwisterGradient(step, r, g, b);
      break;
    case 1:
      setColorGradient2(step, r, g, b);
      break;
    case 2:
      setTwisterGradient(step, r, g, b);
      break;
    case 3:
      setColorGradient(step, r, g, b);
      break;
    case 4:
      setTwisterGradient(step, r, g, b);
      break;
    case 5:
      setFullScreenColor(step, r, g, b);
      break;
  }
  return program;
}

unsigned int r1[8][8], g1[8][8], b1[8][8];
unsigned int r2[8][8], g2[8][8], b2[8][8];
unsigned int (*r)[8], (*g)[8], (*b)[8];
unsigned int (*rp)[8], (*gp)[8], (*bp)[8];
unsigned int mask[8][8];

void loop() {
  long programStart = millis();
  clearBuffer(r1, g1, b1);
  clearBuffer(r2, g2, b2);

  int previousProgram = 0;
  long scrollStart = -1;

  for (unsigned int i = 0; i < 56869; i++) {
    fillMask(mask, 0xFF);

    unsigned long frameStart = millis();
    uptime = frameStart - programStart;

    if (uptime > 1800000) {
      minBrightness = 0.02;
    }
    maxBrightness = fclamp(minBrightness, 1, 1 - uptime / 3600000.0);
    brightness = minBrightness + (maxBrightness - minBrightness) * pow(1 - pow(((uptime + 1500) % 3000 / 1500.0) - 1, 2), 4);

    // Switch active frame buffer every other frame
    if (i % 2 == 0) {
      r = r1; g = g1; b = b1;
      rp = r2; gp = g2; bp = b2;
    } else {
      r = r2; g = g2; b = b2;
      rp = r1; gp = g1; bp = b1;
    }

    if (scrollStart < 0) {
      unsigned int currentProgram = setBgProgram(i, r, g, b);
      if (currentProgram == 0 && previousProgram != 0) {
        scrollStart = frameStart;
      }
      previousProgram = currentProgram;
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
