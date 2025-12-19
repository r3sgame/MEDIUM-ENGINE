#ifndef STRUCTURES_H
#define STRUCTURES_H

#include "box2d/id.h"
#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"
#include <box2d/box2d.h>
#include <unordered_map>
#include <vector>
#include <memory>
#include <iostream>
#include <cmath>
#include <cstring>
#include <numeric>
#include <fstream>
#include "json.hpp"
#include <thread>

// for convenience
using json = nlohmann::json;

// Define a conversion factor from pixels to Box2D units (meters)
#define PIXELS_PER_METER 120.0f
#define METERS_PER_PIXEL (1.0f / PIXELS_PER_METER)

#define PI 3.14159265358979323846f

#define SUB_STEPS 8
#define FPS 60
#define GRAVITY 9.81f
#define CHARACTER_GRAVITY_ADDON 30.0f

#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080

#define CAM_X_INTERPOLATION 0.2f
#define CAM_Y_INTERPOLATION 0.2f
#define CAM_MAX_ZOOM 1.6f
#define CAM_ZOOM_RATE 1500.0f
#define CAM_ZOOM_INTERPOLATION 0.2f
#define CAM_SHAKE_SMALL 2.0f
#define CAM_SHAKE_LARGE 5.0f

#define COLLISION_MARGIN 0.001f
#define JITTER_TOLERANCE 0.2f
#define VELOCITY_TOLERANCE 1.0f
#define COYOTE_TIME 0.15f
#define CHARACTER_FRICTION 0.5f

#define DEFAULT_FRICTION 0.5f
#define DEFAULT_RESTITUTION 0.0f

#define MENU_BG_COLOR Color{ 0, 0, 0, 255 }
#define PAUSE_BG_COLOR Color{ 0, 0, 0, 128 }
#define TITLE_COLOR Color{ 255, 255, 255, 255 }
#define TEXT_COLOR Color{ 255, 255, 255, 255 }
#define SUBTEXT_COLOR Color{ 128, 128, 128, 255 }
#define HOVER_COLOR Color{ 255, 0, 0, 255 }
#define HEALTH_COLOR Color{ 0, 255, 0, 122 }
#define MEDIUM_COLOR Color{ 0, 0, 255, 122 }

#define HOVER_ALPHA_SHIFT 0.5f
#define SUBTEXT_SIZE 32
#define PAUSE_TITLE_SIZE 64
#define PAUSE_OPTION_SPACING 60

#define DEFAULT_FONT "resources/fonts/ScienceGothic-Light.ttf"

#define DEBUG 1

#define DEFAULT_CHARACTER_MASS 1.0f
#define DEFAULT_MATERIAL_MASS 1.0f

#define GL_TEXTURE_2D 0x0DE1
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_LINEAR 0x2601

#define GLSL_VERSION "330"

// Forward declarations
class Debris;
class Level;
struct Option;
class GameInstance;

enum struct OptionType {
    MENU,
    LEVEL
};

struct TextElement {
public:
    TextElement(std::string text, Vector2 position, float scale, Color color, bool centered) : text(text), position(position), scale(scale), color(color), centered(centered) {};

    std::string text;
    Vector2 position;
    float scale;
    Color color;
    bool centered;
};

struct ImageElement {
public:
    ImageElement(Texture2D texture, Vector2 position, float scale, bool centered) : texture(texture), position(position), scale(scale), centered(centered) {};

    Texture2D texture;
    Vector2 position;
    float scale;
    bool centered;
};


struct Option {
public:
    Option(Vector2 position, float scale, std::string text, std::string subtext, Texture2D image, float imageScale, std::string screenToLoad, bool isLevel, bool centered, Color color)
     : position(position), scale(scale), text(text), subtext(subtext), image(image), isLevel(isLevel), screenToLoad(screenToLoad), centered(centered), color(color) {};

    Vector2 position;
    float scale;

    bool hovered = false;
    
    std::string text;
    std::string subtext;
    Texture2D image;
    
    std::string screenToLoad;
    bool isLevel;

    bool centered;
    Color color;
};

class Menu {
public:
    Menu(GameInstance* gameInstance, std::vector<Option> options, std::vector<TextElement> textElements,
        std::vector<ImageElement> imageElements, Vector2 optionSubtextPosition, Vector2 optionImagePosition) :
        gameInstance(gameInstance), options(options), textElements(textElements), imageElements(imageElements), camera({ 0 }),
        optionSubtextPosition(optionSubtextPosition), optionImagePosition(optionImagePosition), loaded(false), font(LoadFont(DEFAULT_FONT)) {};

    void Update();
    void EnterMenu();
    void ExitMenu();

    bool IsLoaded() { return loaded; }

private:
    GameInstance* gameInstance;

    std::vector<Option> options;
    std::vector<TextElement> textElements;
    std::vector<ImageElement> imageElements;

    Vector2 optionSubtextPosition;
    Vector2 optionImagePosition;

    bool loaded;

    Camera2D camera;

    Font font;
};

enum struct AnimationStates {
    IDLE,
    JUMP,
    RUN,
    WINDUP,
    WINDUP_JUMP,
    WINDUP_RUN,
    ACTIVE,
    ACTIVE_JUMP,
    ACTIVE_RUN,
    RECOVERY,
    RECOVERY_JUMP,
    RECOVERY_RUN,
    GUN,
    GUN_JUMP,
    GUN_RUN,
    CHARGE,
    STUN,
    MISC_1,
    MISC_2,
    MISC_3
};

struct SpriteSettings {
public:
    SpriteSettings(Vector2 spriteOffset, std::string folderName, float spriteScale, float framesPerSecond, std::unordered_map<AnimationStates, int> frameCounts,
         std::unordered_map<AnimationStates, Shader> shaders);
    
    std::vector<Texture2D> sprites;
    Vector2 offset;
    float scale;
    float framesPerSecond;
    int totalFrames;

    std::unordered_map<AnimationStates, int> frameCounts;
    std::unordered_map<AnimationStates, Shader> shaders;
};

struct MeleeWeapon {
    MeleeWeapon(Vector2 hitBox, float damage, float mediumAffinity, float energy, float knockback, float windupTime, float activeTime, float recoveryTime, float stunTime, float slowdownFactor) :
    hitBox(hitBox), damage(damage), mediumAffinity(mediumAffinity), energy(energy), knockback(knockback), windupTime(windupTime), activeTime(activeTime), stunTime(activeTime), 
    recoveryTime(recoveryTime), slowdownFactor(slowdownFactor) {}
    
    Vector2 hitBox;

    float damage;
    float mediumAffinity;
    float energy;
    float knockback;

    float windupTime;
    float activeTime;
    float recoveryTime;
    
    float stunTime;
    float slowdownFactor;
};

struct SpriteElement {
    SpriteElement(Vector2 offset, float scale, Texture2D texture, float parallax, bool loopX, bool loopY) : offset(offset), scale(scale), texture(texture), parallax(parallax),
     loopX(loopX), loopY(loopY) {}
    Vector2 offset;
    float scale;
    Texture2D texture;
    float parallax;
    bool loopX;
    bool loopY;
};

// This struct captures the closest hit shape
struct RayCastContext
{
    b2ShapeId shape;
    b2Vec2 point;
    b2Vec2 normal;
    float fraction;
};

enum struct InputTypes {  
    JUMP,
    RIGHT,
    LEFT,
    ATTACK,
    SHOOT,
    CHARGE,
    SKILL,
    PAUSE
};

class Emitter {
    public:
    Emitter(Shader shader, Vector2 velocityMin, Vector2 velocityMax, Vector2 acceleration, float scaleMin, float scaleMax, float lifetimeMin,
         float lifetimeMax, int numParticles, Color colorBound1, Color colorBound2)
          : shader(shader), velocityMax(velocityMax), velocityMin(velocityMin), acceleration(acceleration), scaleMin(scaleMin), scaleMax(scaleMax),
           lifetimeMin(lifetimeMin), lifetimeMax(lifetimeMax), numParticles(numParticles), colorBound1(colorBound1), colorBound2(colorBound2) {};

    void Start(Vector2 initialPosition, bool flipX, bool flipY);
    void Update();
    void Draw();

    private:
    Shader shader;

    Vector2 velocityMax;
    Vector2 velocityMin;
    Vector2 acceleration;

    float scaleMin;
    float scaleMax;

    float lifetimeMin;
    float lifetimeMax;

    std::vector<Vector2> positions = {};
    std::vector<Vector2> velocities = {};
    std::vector<float> scales = {};
    std::vector<float> lifetimes = {};
    std::vector<Color> colors = {};

    int numParticles;

    Color colorBound1;
    Color colorBound2;

    int flipXMultiplier;
    int flipYMuliplier;
};

struct Skill {
    Skill(Vector2 force, Vector2 upHitBox, Vector2 downHitBox, Vector2 forwardHitBox, Vector2 backwardHitBox, float energy, float damage, float mediumCost, float knockback,
        float windupTime, float activeTime, float recoveryTime, float stunTime, float slowdownFactor, AnimationStates animationState, InputTypes input, Emitter particles) 
    : force(force), upHitBox(upHitBox), downHitBox(downHitBox), forwardHitBox(forwardHitBox), backwardHitBox(backwardHitBox), energy(energy), damage(damage), mediumCost(mediumCost), knockback(knockback), 
    windupTime(windupTime), activeTime(activeTime), recoveryTime(recoveryTime), stunTime(stunTime), slowdownFactor(slowdownFactor), animationState(animationState), input(input), particles(particles) {}
    
    Vector2 force;

    Vector2 upHitBox;
    Vector2 downHitBox;
    Vector2 forwardHitBox;
    Vector2 backwardHitBox;

    float energy;
    float damage;
    float mediumCost;
    float knockback;

    float windupTime;
    float activeTime;
    float recoveryTime;
    
    float stunTime;
    float slowdownFactor;

    AnimationStates animationState;
    InputTypes input;
    Emitter particles;
};

struct Gun {
    Gun(float fireTime, int ammo, float damage, float mediumAffinity, float reloadTime, float range, float energy, float knockback,
         float stunTime, float slowdownFactor, Emitter muzzleFlash, Vector2 muzzleFlashPosition) :
    fireTime(fireTime), maxAmmo(ammo), damage(damage), mediumAffinity(mediumAffinity), reloadTime(reloadTime), range(range), energy(energy),
     knockback(knockback), stunTime(stunTime), slowdownFactor(slowdownFactor), muzzleFlash(muzzleFlash), muzzleFlashPosition(muzzleFlashPosition) {}

    float fireTime;
    int maxAmmo;
    float damage;
    float mediumAffinity;
    float reloadTime;
    float range;
    float energy;
    float knockback;
    float stunTime;
    float slowdownFactor;

    Emitter muzzleFlash;
    Vector2 muzzleFlashPosition;
};

enum struct BodyTypes {
    CHARACTER,
    DESTRUCTIBLE_BODY,
    DEBRIS
};

enum struct CharacterTypes {
    PLAYER_1,
    PLAYER_2,
    CPU
};

enum struct CharacterStates {
    RELOAD,
    GUN,
    WEAPON,
    SKILL,
    STUN,
    CHARGE,
    Size // Cast to int to return number of states, useful for hashmaps
};

class Character {
public:
    Character(b2WorldId worldId, Vector2 initialPosition, Vector2 bodySize, Vector2 hurtBoxSize, float moveSpeed, float jumpForce, float maxHealth, float maxMedium, float mediumChargeRate,
        CharacterTypes characterType, MeleeWeapon equippedWeapon, Gun equippedGun, SpriteSettings spriteSettings, std::vector<Skill> availableSkills);
    ~Character();

    void Update();
    void Draw();

    // Public getters for character state if needed by external classes
    Vector2 GetPosition() const { return position; }
    Rectangle GetRect() const { return rect; }
    b2BodyId GetBodyId() const { return body; }

    float GetMaxHealth() const { return maxHealth; }
    float GetMaxMedium() const { return maxMedium; }
    int GetAmmo() const { return ammo; }

    CharacterTypes GetCharacterType() const { return characterType; }

    bool IsUsingSkill() const { return states.at(CharacterStates::SKILL); }
    bool IsAttacking() const { return states.at(CharacterStates::WEAPON); }
    bool IsShooting() const { return states.at(CharacterStates::GUN); }
    bool IsReloading() const { return states.at(CharacterStates::RELOAD); }
    bool IsStunned() const { return states.at(CharacterStates::STUN); }

    Skill GetActiveSkill() const { return activeSkill; }

    MeleeWeapon weapon;
    Gun gun;
    std::vector<Skill> skills;

    float health;
    float medium;

    // Setters
    void SetPosition(Vector2 newPosition);
    void SetStun(float stunTime);

private:
    b2WorldId world;
    b2BodyId body;
    b2ShapeId shape;

    Vector2 size;
    Vector2 hurtBox;

    Vector2 position;
    Rectangle rect;

    float moveSpeedPixelsPerSec;
    float jumpForcePixels;
    CharacterTypes characterType;
    float coyoteTimeCounter;

    float maxHealth;
    float maxMedium;
    float chargeRate;
    Skill activeSkill;
    int ammo;
    
    std::unordered_map<CharacterStates, float> timers;
    std::unordered_map<CharacterStates, bool> states;

    bool isFacingRight = true;

    int currentFrame = 0;
    int maxFrame = 1;
    float frameTimer = 0.0f;
    AnimationStates animationState;
    SpriteSettings spriteSet;

    bool IsGrounded();
    
    void MoveLeft();
    void MoveRight();
    void Jump();
    void Shoot();
    void StartMeleeAttack();
    void ProcessMeleeAttack();
    void StartSkill(Skill skill);
    void ProcessSkill();
    void Charge();
};

Vector2 GetCameraPosition(std::vector<Character*> characters, Vector2 initialPosition);

Character* LoadCharacter(b2WorldId world, std::string characterName, Vector2 initialPosition, CharacterTypes characterType);

struct BodyMaterial {
    BodyMaterial(Texture2D texture, Texture2D debrisTexture, int textureNumShapes, int debrisTextureNumShapes,
         float density, float energyCapacity, float debrisEnergyCapacity, Emitter emitter) :
    texture(texture), textureNumShapes(textureNumShapes), density(density), energyCapacity(energyCapacity), debrisTexture(debrisTexture),
     debrisTextureNumShapes(debrisTextureNumShapes), debrisEnergyCapacity(debrisEnergyCapacity), emitter(emitter) {};
    Texture2D texture;
    Texture2D debrisTexture;
    int textureNumShapes;
    int debrisTextureNumShapes;
    float density;
    float energyCapacity;
    float debrisEnergyCapacity;
    Emitter emitter;
};

class DestructibleBody {
public:
    DestructibleBody(b2WorldId worldId, BodyMaterial material, Vector2 initialPosition, float initialRotation, bool dynamic,
         float width, float height, float squareSize);
    ~DestructibleBody();

    void Update();
    void Draw();
    void DestroyShape(b2ShapeId shape);
    float GetEnergyCapacity() { return material.energyCapacity; }

private:
    b2WorldId world;
    b2BodyId body;
    std::vector<Debris*> debrisInstances;

    BodyMaterial material;
    int maxShapesCapacity; // Initial max number of shapes expected, used for array sizing
    int triangles;

    Vector2 position;
    float rotation;
    Rectangle rect;
};

class Debris {
public:
    Debris(b2WorldId worldId, Vector2 initialPosition, float initialRotation, float energyCapacity, b2Polygon polygonShape);
    ~Debris();

    void Update();
    virtual void DestroyShape(b2ShapeId shape);
    
    float GetEnergyCapacity() { return energyCapacity; }
    bool IsDestroyed() const { return destroyed; } // Getter for destroyed state
    b2BodyId GetBodyId() const { return body; }
    float* GetVertexMagnitudes() { return vertexMagnitudes; }
    float* GetVertexAngles() { return vertexAngles; }

protected:
    b2WorldId world;
    b2BodyId body;

    float vertexMagnitudes[8];
    float vertexAngles[8];
    int numVertices;

    float energyCapacity;
    bool destroyed; // Made private, with a public getter
    Vector2 position;
    float rotation;

    void BuildMeshFromBody();
};

class FreeDebris : public Debris {
public:
    FreeDebris(b2WorldId worldId, Vector2 initialPosition, float initialRotation, float energyCapacity, b2Polygon polygonShape, Texture2D texture,
         int textureNumShapes, Emitter emitter) :
        Debris(worldId, initialPosition, initialRotation, energyCapacity, polygonShape), texture(texture), textureNumShapes(textureNumShapes),
         emitter(emitter) {};
    void Draw();

private:
    Texture2D texture;
    int textureNumShapes;
    Emitter emitter;

    void DestroyShape(b2ShapeId shape) override;
};

class Level {
public:
    Level(GameInstance* gameInstance, b2WorldId worldId, Shader postProcessingShader, std::vector<Character*> characters, std::vector<DestructibleBody*> destructibleBodies, 
        std::vector<FreeDebris*> freeDebrisInstances, std::vector<SpriteElement> backgroundElements, std::vector<SpriteElement> foregroundElements)
     : gameInstance(gameInstance), world(worldId), postProcessingShader(postProcessingShader), camera({ 0 }), characters(characters), destructibleBodies(destructibleBodies), freeDebrisInstances(freeDebrisInstances), 
     backgroundElements(backgroundElements), foregroundElements(foregroundElements), font(LoadFont(DEFAULT_FONT)) {
        camera.target = { 0.0f, 0.0f };
        camera.offset = { (float)SCREEN_WIDTH / 2, (float)SCREEN_HEIGHT / 2 };
        camera.rotation = 0.0f;
        camera.zoom = 1.5f;

        renderTexture = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
     }
    ~Level() = default; // Use default destructor as unique_ptrs handle memory

    void LoadLevel();
    void UnloadLevel();
    void Update();

    bool IsLoaded() const { return loaded; } // Getter for loaded state
    bool IsPaused() const { return paused; }

private:
    GameInstance* gameInstance;
    b2WorldId world;
    Camera2D camera;

    Shader postProcessingShader;
    RenderTexture2D renderTexture;

    std::vector<Character*> characters;
    std::vector<DestructibleBody*> destructibleBodies;
    std::vector<FreeDebris*> freeDebrisInstances;

    std::vector<SpriteElement> backgroundElements;
    std::vector<SpriteElement> foregroundElements;

    Font font;

    bool loaded = true;
    bool paused = false;
};

class GameInstance {
public:
    GameInstance(std::string startingMenu);
    Menu* currentMenu;
    Level* currentLevel;
    std::string startingMenu;

    Font font;

    void Update();
    void SetMenu(std::string name);
    void SetLevel(std::string name, std::vector<std::string> characterLoadIds);
};

// JSON Loaders
Level* LoadLevel(std::string name, GameInstance* gameInstance, std::vector<std::string> characterLoadIds);
Emitter LoadParticleEmitter(std::string name);
Menu* LoadMenu(std::string name, GameInstance* gameInstance);

#endif // STRUCTURES_H