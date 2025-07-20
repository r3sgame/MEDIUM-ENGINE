#ifndef STRUCTURES_H
#define STRUCTURES_H

#include "raylib.h"
#include <box2d/box2d.h> // Include Box2D header (this pulls in b2_api.h which defines IDs)
#include <vector>
#include <memory>

// Define a conversion factor from pixels to Box2D units (meters)
#define PIXELS_PER_METER 30.0f
#define METERS_PER_PIXEL (1.0f / PIXELS_PER_METER)

#define PI 3.14159265358979323846f

// Box2D simulation parameters
#define dt (1.0f / 60.0f)
#define SUB_STEPS 8

#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080

#define CAM_X_INTERPOLATION 0.2f
#define CAM_Y_INTERPOLATION 0.2f

#define COLLISION_MARGIN 0.001f
#define JITTER_TOLERANCE 0.1f
#define MAX_SHAPES_PER_BODY 10000
#define COYOTE_TIME 0.15f
#define BASE_FRICTION 1.0f

#define DEFAULT_FRICTION 0.5f
#define DEFAULT_RESTITUTION 0.0f

// CHARACTER
class Character {
public:
    Vector2 position;
    Rectangle rect;

    b2BodyId body;

    Character(b2WorldId worldId, float width, float height, float moveSpeed, float jumpForce, int maxHealth, int maxMedium, bool isPlayer);
    ~Character(); // Destructor to destroy Box2D body

    void Update();
    void Draw();

    // Movement functions (apply forces/impulses to Box2D body)
    void MoveLeft();
    void MoveRight();
    void Jump();
    void ApplyDownwardForce();

    bool IsGrounded(); // Check if the character is grounded

    int mxHealth;
    int mxMedium;
    int health;
    int medium;

private:
    b2WorldId world; // Store a reference to the world to destroy the body
    float moveSpeedPixelsPerSec; // Store speed in pixels/sec
    float jumpForcePixels;       // Store jump force in pixels
    bool player;
    bool isGroundedLastFrame; // To detect when the player just left the ground
    float coyoteTimeCounter;  // Counts down from coyoteTimeDuration
};

// WEAPON
class MeleeWeapon {
public:
    float range;
    float damage;
    float energy;
};

// GUN
class Gun {
public:
    float fireRate;
    float ammo;
    float damage;
    float reloadTime;
    float range;
    float energy;
};

// DESTRUCTIBLE BODY

class Debris {
public:
    Debris(b2WorldId worldId, Vector2 initialPosition, float initialRotation, float energyCapacity, b2Polygon polygonShape);
    ~Debris();

    void Update();

    b2WorldId world;
    b2BodyId body;

    float vertexMagnitudes[8];
    float vertexAngles[8];
    int numVertices;

    float energyCap;
    bool destroyed = false;
    Vector2 position;
};

class DestructibleBody {
public:
    DestructibleBody(b2WorldId worldId, Vector2 initialPosition, float initialRotation, bool dynamic, float width, float height, float energyCapacity, float debrisEnergyCapacity, float squareSize);
    ~DestructibleBody();

    b2WorldId world;
    b2BodyId body;
    std::vector<std::unique_ptr<Debris>> debrisInstances;

    float energyCap;
    float debrisEnergyCap;
    int triangles;

    Vector2 position;
    Rectangle rect;

    void Update();
};

// LEVEL

class Level {
public:
    Level(b2WorldId worldId, Character& playerToLoad);

    b2WorldId world;
    Character& player;
    std::vector<std::unique_ptr<DestructibleBody>> destructibleBodies;
    std::vector<std::unique_ptr<Debris>> freeDebrisInstances;

    void LoadLevel(Vector2 spawnPoint);
    void UnloadLevel();
    void Update();

    bool loaded = false;
};

#endif // STRUCTURES_H