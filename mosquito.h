#ifndef MOSQUITO_H
#define MOSQUITO_H

// ---------------- Mosquito struct ----------------
struct Mosquito {
    float x, y;
    float dx, dy;
    float size;
    bool alive;
    int deadTimer;
    bool attractedToPond;
    int pondTime; // For larva spawning
};

extern Mosquito mosquitoes[];

void initializeMosquitoes();
void updateMosquitoesLogic();
void spawnOneMosquito(bool pondBoost);
void checkSprayCollisions();
void updateHistogram(int killedThisFrame);
bool isNearPondArea(float x, float y);

#endif // MOSQUITO_H