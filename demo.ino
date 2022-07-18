#include <Rainbowduino.h>

void setCharMask(unsigned char ascii, int poX, int poY, int mask[][8]) {
  if ((ascii < 0x20) || (ascii > 0x7e)) {
    // Unsupported char.
    ascii = '?';
  }
  for (int i = 0; i < 8; i++) {
    int temp = pgm_read_byte(&simpleFont[ascii - 0x20][i]);
    for (int f = 0; f < 8; f++)
    {
      if ((temp >> f) & 0x01)
      {
        mask[poY + f][poX + i] = 0xFF;
      }
      else {
        mask[poY + f][poX + i] = 0;
      }
    }
  }
}

void setEmptyMask(int mask[][8]) {
  for (int x = 0; x < 8; x++) {
    for (int y = 0; y < 8; y++) {
      mask[x][y] = 0xFF;
    }
  }
}

void setColorGradient(int seed, int r[][8], int g[][8], int b[][8]) {
  r[0][0] = 128 + 127 * sin(seed % 29 / 29.0 * 2 * 3.14159);
  g[0][0] = 128 + 127 * sin(seed % 53 / 53.0 * 2 * 3.14159 + 2);
  b[0][0] = 128 + 127 * sin(seed % 37 / 37.0 * 2 * 3.14159 + 4);

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

void loop() {
  int r1[8][8], g1[8][8], b1[8][8];
  int r2[8][8], g2[8][8], b2[8][8];
  int (*r)[8], (*g)[8], (*b)[8];
  int (*rp)[8], (*gp)[8], (*bp)[8];

  int mask[8][8];
  setEmptyMask(mask);

  for (unsigned int i = 0; i < 56869; i++) {
    if (i % 2 == 0) {
      r = r1; g = g1; b = b1;
      rp = r2; gp = g2; bp = b2;
    } else {
      r = r2; g = g2; b = b2;
      rp = r1; gp = g1; bp = b1;
    }

    setColorGradient(i, r, g, b);

    //setCharMask('m', 0, 0, mask);

    // Apply mask to buffer
    for (int x = 0; x < 8; x++) {
      for (int y = 0; y < 8; y++) {
        r[x][y] = r[x][y] & mask[x][y];
        g[x][y] = g[x][y] & mask[x][y];
        b[x][y] = b[x][y] & mask[x][y];
      }
    }

    // Draw buffer
    for (int x = 0; x < 8; x++) {
      for (int y = 0; y < 8; y++) {
        if (r[x][y] != rp[x][y] || g[x][y] != gp[x][y] || b[x][y] != bp[x][y]) {
          Rb.setPixelXY(y, 7 - x, r[x][y], g[x][y], b[x][y]);
        }
      }
    }

    delay(15);
  }
}

void setup() {
  Rb.init();
  randomSeed(analogRead(0));
}
