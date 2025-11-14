#ifndef DRAWING_H
#define DRAWING_H

void drawCircle(float cx, float cy, float rx, float ry, int segments = 48);
void drawMosquito(float x, float y, float size);
void drawLarva(float x, float y);
void drawHouse(float x, float y, float w, float h);
void drawTree(float x, float y);
void drawGrass(float x, float y);
void drawPond();
void drawWaterBowl();
void drawRain();
void drawHistogram();
void display();

#endif // DRAWING_H