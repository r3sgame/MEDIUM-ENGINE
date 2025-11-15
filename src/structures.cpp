#include "structures.h"
#include "box2d/box2d.h"
#include "box2d/id.h"
#include <iostream>

//HELPER FUNCTIONS
bool CheckInput(InputTypes input,  CharacterTypes characterType) {
    if (characterType == CharacterTypes::PLAYER_1) {
        switch (input) {
            case InputTypes::RIGHT:
                return IsKeyDown(KEY_RIGHT);
            case InputTypes::LEFT:
                return IsKeyDown(KEY_LEFT);
            case InputTypes::JUMP:
                return IsKeyPressed(KEY_UP);
            case InputTypes::CHARGE:
                return IsKeyDown(KEY_DOWN);
            case InputTypes::ATTACK:
                return IsKeyPressed(KEY_B);
            case InputTypes::SKILL:
                return IsKeyDown(KEY_N);
            case InputTypes::SHOOT:
                return IsKeyDown(KEY_M);
            default:
                return false;
        }
    } else if (characterType == CharacterTypes::PLAYER_2) {
        switch (input) {
            case InputTypes::RIGHT:
                return IsKeyDown(KEY_D);
            case InputTypes::LEFT:
                return IsKeyDown(KEY_A);
            case InputTypes::JUMP:
                return IsKeyPressed(KEY_W);
            case InputTypes::CHARGE:
                return IsKeyDown(KEY_S);
            case InputTypes::ATTACK:
                return IsKeyPressed(KEY_ONE);
            case InputTypes::SKILL:
                return IsKeyDown(KEY_TWO);
            case InputTypes::SHOOT:
                return IsKeyDown(KEY_THREE);
            default:
                return false;
        }
    }

    return false;
}

void DrawTextureLoopX(Texture2D texture, float width, float height, Vector2 elementPosition, Vector2 cameraPosition, float scale, float zoom) {
    Vector2 textureCameraOrigin = {elementPosition.x + width * floorf((cameraPosition.x - elementPosition.x) / (width)), elementPosition.y};
    DrawTextureEx(texture, textureCameraOrigin, 0, scale, WHITE);

    Vector2 rightTextureCameraOrigin = {textureCameraOrigin.x + width, textureCameraOrigin.y};
    while (rightTextureCameraOrigin.x < cameraPosition.x + SCREEN_WIDTH * (1 / zoom)) {
        DrawTextureEx(texture, rightTextureCameraOrigin, 0, scale, WHITE);
        rightTextureCameraOrigin.x += width;
    }

    Vector2 leftTextureCameraOrigin = {textureCameraOrigin.x - width, textureCameraOrigin.y};
    while (leftTextureCameraOrigin.x + width > cameraPosition.x - SCREEN_WIDTH * (1 / zoom)) {
        DrawTextureEx(texture, leftTextureCameraOrigin, 0, scale, WHITE);
        leftTextureCameraOrigin.x -= width;
    }
}

void DrawTextureLoopY(Texture2D texture, float width, float height, Vector2 elementPosition, Vector2 cameraPosition, float scale, float zoom) {
    Vector2 textureCameraOrigin = {elementPosition.x, elementPosition.y + height * floorf((cameraPosition.y - elementPosition.y) / (height))};
    DrawTextureEx(texture, textureCameraOrigin, 0, scale, WHITE);

    Vector2 bottomTextureCameraOrigin = {textureCameraOrigin.x, textureCameraOrigin.y + height};
    while (bottomTextureCameraOrigin.y < cameraPosition.y + SCREEN_HEIGHT * (1 / zoom)) {
        DrawTextureEx(texture, bottomTextureCameraOrigin, 0, scale, WHITE);
        bottomTextureCameraOrigin.y += height;
    }

    Vector2 topTextureCameraOrigin = {textureCameraOrigin.x, textureCameraOrigin.y - height};
    while (topTextureCameraOrigin.y + height > cameraPosition.y - SCREEN_HEIGHT * (1 / zoom)) {
        DrawTextureEx(texture, topTextureCameraOrigin, 0, scale, WHITE);
        topTextureCameraOrigin.y -= height;
    }
}

float RayCastCallback(b2ShapeId shapeId, b2Vec2 point, b2Vec2 normal, float fraction, void* context)
{
    RayCastContext* rayContext = (RayCastContext*) context;
    rayContext->shape = shapeId;
    rayContext->point = point;
    rayContext->normal = normal;
    rayContext->fraction = fraction;
    return fraction;
}

void Knockback(float knockback, b2Vec2 b2Position, b2BodyId body) {
    b2Vec2 direction = {b2Body_GetPosition(body).x - b2Position.x, b2Body_GetPosition(body).y -  b2Position.y};
    b2Vec2 normalized = b2Normalize(direction);
    float mass = b2Body_GetMass(body);
    b2Vec2 impulse = {normalized.x * knockback * mass, normalized.y * knockback * mass};
    b2Body_ApplyLinearImpulse(body, impulse, {0, 0}, true);
}

bool HandleAttack(int damage, float energy, float knockback, float stunTime, b2Vec2 b2Position, b2ShapeId shape, bool continuousAttack) {
    if (b2Shape_IsValid(shape)) {
        b2BodyId target = b2Shape_GetBody(shape);
        const char *targetName = b2Body_GetName(target);
        
        if (strcmp(targetName, "Character") == 0) {
            Character *targetCharacter = (Character *) b2Body_GetUserData(target);
            if (!targetCharacter->IsStunned()) {
                targetCharacter->health -= damage;
                targetCharacter->SetStun(stunTime);
            }
        
            if (continuousAttack) {
                Knockback(knockback * GetFrameTime(), b2Position, target);
            } else {
                Knockback(knockback, b2Position, target);
            }

            return true;
        }

        if (strcmp(targetName, "DestructibleBody") == 0) {
            DestructibleBody *targetBody = (DestructibleBody *) b2Body_GetUserData(target);

            if (energy > targetBody->GetEnergyCapacity()) {
                targetBody->DestroyShape(shape);
            }
        }

        if (strcmp(targetName, "Debris") == 0) {
            Debris *targetDebris = (Debris *) b2Body_GetUserData(target);

            if (energy > targetDebris->GetEnergyCapacity()) {
                targetDebris->DestroyShape(shape);
            } else {
                if (continuousAttack) {
                    Knockback(knockback * GetFrameTime(), b2Position, target);
                } else {
                    Knockback(knockback, b2Position, target);
                }
            }
        }
    }

    return false;
}


bool OverlapCallback(b2ShapeId shapeId, void* context)
{
    Character* character = (Character*)context;
    b2Vec2 position = b2Body_GetPosition(character->GetBodyId());

    if (character->IsUsingSkill()) {
        Skill activeSkill = character->GetActiveSkill();
        bool hitCharacter = HandleAttack(activeSkill.damage, activeSkill.energy, activeSkill.knockback, activeSkill.stunTime, position, shapeId, true);
    } else {
        bool hitCharacter = HandleAttack(character->weapon.damage, character->weapon.energy, character->weapon.knockback, character->weapon.stunTime, position, shapeId, true);

        if (hitCharacter) {
            character->medium = std::min(character->GetMaxMedium(), character->medium + character->weapon.mediumAffinity);
        }
    }
 
    // Return true to continue the query.
    return true;
}

// Helper function to create a circle texture for particles
Texture2D CreateCircleParticleTexture(int size, Color color) {
    Image image = GenImageColor(size, size, BLANK); // Start with transparent
    ImageDrawCircle(&image, size / 2, size / 2, size / 2, color);
    Texture2D texture = LoadTextureFromImage(image);
    UnloadImage(image);
    return texture;
}

// Function to find an object by a name attribute
json FindObjectByName(const json& jsonArray, const std::string& name) {
    // Check if the JSON value is an array
    if (!jsonArray.is_array()) {
        std::cerr << "Error: Provided JSON is not an array." << std::endl;
        return nullptr; // Or throw an exception
    }

    for (const auto& obj : jsonArray) {
        // Check if the object has a "name" key and if its value matches
        if (obj.contains("name") && obj["name"] == name) {
            return obj; // Return the found object
        }
    }
    
    // Return a null JSON object if no match is found
    return nullptr;
}

Emitter* LoadParticleEmitter(std::string name) {
    
    // Attempt to parse the emitter from JSON
    try {
        std::ifstream f("data.json");
        json data = json::parse(f);
        json particleData = FindObjectByName(data["particles"], name);

        EmitterConfig cfg = {0};
        
        // --- Successful Load Path ---

        // Basic Properties
        std::string name = particleData.value("name", "Default Emitter (Fallback)");
        std::cout << "INFO: Loading emitter: " << name << std::endl;
        
        cfg.capacity = particleData.value("count", 300);
        cfg.emissionRate = (size_t)particleData.value("emissionRate", 100.0f);
        
        // Blending
        bool additive = particleData.value("additiveBlending", false);
        cfg.blendMode = additive ? BLEND_ADDITIVE : BLEND_ALPHA;
        
        // Lifetime
        float lifetime = particleData.value("lifetime", 1.0f);
        cfg.age = (FloatRange){.min = lifetime, .max = lifetime + 0.5f};
        
        // Color
        if (particleData.contains("color") && particleData.at("color").is_object()) {
            const json& colorObj = particleData.at("color");
            float r = colorObj.value("r", 0.0f);
            float g = colorObj.value("g", 0.0f);
            float b = colorObj.value("b", 1.0f);
            float a = colorObj.value("a", 1.0f);

            cfg.startColor = (Color){
                (unsigned char)(r * 255.0f), (unsigned char)(g * 255.0f), 
                (unsigned char)(b * 255.0f), (unsigned char)(a * 255.0f)
            };
            cfg.endColor = (Color){cfg.startColor.r, cfg.startColor.g, cfg.startColor.b, 0};
        } else {
            cfg.startColor = BLUE;
            cfg.endColor = (Color){0, 0, 255, 0};
        }
        
        // Texture
        int size = particleData.value("size", 10);
        cfg.texture = CreateCircleParticleTexture(size, WHITE); 
        
        // Origin / Offset
        cfg.origin = (Vector2){particleData.value("offsetX", 0.0f), particleData.value("offsetY", 0.0f)};
        
        // Velocity and Angle Translation
        float velXMin = particleData.value("velocityXMin", -100.0f);
        float velXMax = particleData.value("velocityXMax", 100.0f);
        float velYMin = particleData.value("velocityYMin", -100.0f);
        float velYMax = particleData.value("velocityYMax", 100.0f);
        
        float max_vx_abs = std::max(std::fabs(velXMin), std::fabs(velXMax));
        float max_vy_abs = std::max(std::fabs(velYMin), std::fabs(velYMax));
        float maxSpeed = std::sqrt(max_vx_abs * max_vx_abs + max_vy_abs * max_vy_abs);

        cfg.velocity = (FloatRange){.min = 0.0f, .max = maxSpeed};
        cfg.velocityAngle = (FloatRange){.min = 0.0f, .max = 360.0f};
        cfg.direction = (Vector2){0, -1};

        // External Acceleration
        cfg.externalAcceleration = (Vector2){particleData.value("accelerationX", 0.0f), particleData.value("accelerationY", 0.0f)};

        // Initialize and return the loaded Emitter
        Emitter *emitter = Emitter_New(cfg);
        return emitter;

    } catch (const std::exception& e) {
        // CATCH-ALL BLOCK: Inline implementation of the default emitter logic
        std::cerr << "Load particle failed (" << e.what() << "). Initializing hardcoded DEFAULT emitter." << std::endl; // ⚠️ cerr/cout
        
        // Define and initialize the default configuration directly
        EmitterConfig defaultCfg = {0};
        
        // Default Texture (5x5 white circle)
        Image image = GenImageColor(5, 5, BLANK);
        ImageDrawCircle(&image, 2, 2, 2, WHITE);
        defaultCfg.texture = LoadTextureFromImage(image);
        UnloadImage(image);

        // General Settings
        defaultCfg.capacity = 100;
        defaultCfg.emissionRate = 50;
        defaultCfg.blendMode = BLEND_ALPHA;
        defaultCfg.origin = (Vector2){400, 300}; 

        // Lifetime
        defaultCfg.age = (FloatRange){.min = 1.0f, .max = 2.0f};

        // Movement (Slight upward drift)
        defaultCfg.direction = (Vector2){0, -1};
        defaultCfg.velocity = (FloatRange){.min = 50.0f, .max = 100.0f};
        defaultCfg.velocityAngle = (FloatRange){.min = -15.0f, .max = 15.0f}; 
        defaultCfg.externalAcceleration = (Vector2){0.0f, 0.0f};

        // Color Fade (White fade)
        defaultCfg.startColor = (Color){255, 255, 255, 200};
        defaultCfg.endColor = (Color){255, 255, 255, 0};
        
        // Initialize and return the default Emitter
        Emitter *emitter = Emitter_New(defaultCfg);
        Emitter_Start(emitter);
        return emitter;
    }
}

Character* LoadCharacter(b2WorldId world, std::string characterName, Vector2 initialPosition, CharacterTypes characterType) {
    try {
        std::ifstream f("data.json");
        json data = json::parse(f);
        json characterData = FindObjectByName(data["characters"], characterName);

        // Load Melee Weapon
        json meleeWeaponData = FindObjectByName(data["meleeWeapons"], characterData["meleeWeapon"]);

        Vector2 meleeHitBox = (Vector2){meleeWeaponData["hitBoxWidth"].get<float>(), meleeWeaponData["hitBoxHeight"].get<float>()};
        float meleeDamage = meleeWeaponData["damage"].get<float>();
        float meleeMediumAffinity = meleeWeaponData["mediumAffinity"].get<float>();
        float meleeEnergy = meleeWeaponData["energy"].get<float>();
        float meleeKnockback = meleeWeaponData["knockback"].get<float>();
        float meleeWindupTime = meleeWeaponData["windupTime"].get<float>();
        float meleeActiveTime = meleeWeaponData["activeTime"].get<float>();
        float meleeRecoveryTime = meleeWeaponData["recoveryTime"].get<float>();
        float meleeStunTime = meleeWeaponData["stunTime"].get<float>();
        float meleeSlowdownFactor = meleeWeaponData["slowdownFactor"].get<float>();

        std::cout << "Character: Loaded Melee Weapon" << std::endl;

        MeleeWeapon meleeWeapon(meleeHitBox, meleeDamage, meleeMediumAffinity, meleeEnergy, meleeKnockback, meleeWindupTime,
             meleeActiveTime, meleeRecoveryTime, meleeStunTime, meleeSlowdownFactor);

        // Load Gun
        json gunData = FindObjectByName(data["guns"], characterData["gun"]);

        float fireTime = gunData["fireTime"].get<float>();
        float ammo = gunData["ammo"].get<int>();
        float gunDamage = gunData["damage"].get<float>();
        float gunMediumAffinity = gunData["mediumAffinity"].get<float>();
        float reloadTime = gunData["reloadTime"].get<float>();
        float range = gunData["range"].get<float>();
        float gunEnergy = gunData["energy"].get<float>();
        float gunKnockback = gunData["knockback"].get<float>();
        float gunStunTime = gunData["stunTime"].get<float>();
        float gunSlowdownFactor = gunData["slowdownFactor"].get<float>();

        std::cout << "Character: Loaded Gun" << std::endl;

        Gun gun(fireTime, ammo, gunDamage, gunMediumAffinity, reloadTime, range, gunEnergy, gunKnockback, gunStunTime, gunSlowdownFactor);

        // Load SpriteSettings
        json spriteSettingsData = FindObjectByName(data["spriteSettings"], characterData["spriteSettings"]);
        json frameCountsData = spriteSettingsData["frameCounts"];
        json shaderData = spriteSettingsData["shaders"];

        std::unordered_map< AnimationStates, int> frameCounts;

        frameCounts[AnimationStates::IDLE] = frameCountsData["idle"].get<int>();
        frameCounts[AnimationStates::JUMP] = frameCountsData["jump"].get<int>();
        frameCounts[AnimationStates::RUN] = frameCountsData["run"].get<int>();
        frameCounts[AnimationStates::WINDUP] = frameCountsData["windup"].get<int>();
        frameCounts[AnimationStates::ACTIVE] = frameCountsData["active"].get<int>();
        frameCounts[AnimationStates::RECOVERY] = frameCountsData["recovery"].get<int>();
        frameCounts[AnimationStates::GUN] = frameCountsData["gun"].get<int>();
        frameCounts[AnimationStates::STUN] = frameCountsData["stun"].get<int>();
        frameCounts[AnimationStates::CHARGE] = frameCountsData["charge"].get<int>();

        std::unordered_map< AnimationStates, Shader> shaders;

        std::string shaderFilePath = std::string("resources/shaders/") + std::string(GLSL_VERSION) + std::string("/");

        shaders[AnimationStates::IDLE] = LoadShader(0, (shaderFilePath + shaderData["idle"].get<std::string>()).c_str());
        shaders[AnimationStates::JUMP] = LoadShader(0, (shaderFilePath + shaderData["jump"].get<std::string>()).c_str());
        shaders[AnimationStates::RUN] = LoadShader(0, (shaderFilePath + shaderData["run"].get<std::string>()).c_str());
        shaders[AnimationStates::WINDUP] = LoadShader(0, (shaderFilePath + shaderData["windup"].get<std::string>()).c_str());
        shaders[AnimationStates::ACTIVE] = LoadShader(0, (shaderFilePath + shaderData["active"].get<std::string>()).c_str());
        shaders[AnimationStates::RECOVERY] = LoadShader(0, (shaderFilePath + shaderData["recovery"].get<std::string>()).c_str());
        shaders[AnimationStates::GUN] = LoadShader(0, (shaderFilePath + shaderData["gun"].get<std::string>()).c_str());
        shaders[AnimationStates::STUN] = LoadShader(0, (shaderFilePath + shaderData["stun"].get<std::string>()).c_str());

        std::cout << "Character: Loaded Frame Counts" << std::endl;

        Vector2 spriteOffset = (Vector2){spriteSettingsData["spriteOffsetX"].get<float>(), spriteSettingsData["spriteOffsetY"].get<float>()};
        std::string folderName = spriteSettingsData["name"].get<std::string>();
        float spriteScale = spriteSettingsData["spriteScale"].get<float>();
        float framesPerSecond = spriteSettingsData["framesPerSecond"].get<float>();

        SpriteSettings spriteSettings(spriteOffset, folderName, spriteScale, framesPerSecond, frameCounts, shaders);

        std::cout << "Character: Loaded Sprite Settings" << std::endl;

        // Load Skills
        std::vector<Skill> skills;
        
        for (const auto& skillPointer : characterData["skills"]) {
            if (skillPointer.is_string()) {
                json skillData = FindObjectByName(data["skills"], skillPointer.get<std::string>());

                Vector2 force = (Vector2){skillData["forceX"].get<float>(), skillData["forceY"].get<float>()};

                Vector2 upHitBox = (Vector2){skillData["upHitBoxWidth"].get<float>(), skillData["upHitBoxHeight"].get<float>()};
                Vector2 downHitBox = (Vector2){skillData["downHitBoxWidth"].get<float>(), skillData["downHitBoxHeight"].get<float>()};
                Vector2 forwardHitBox = (Vector2){skillData["forwardHitBoxWidth"].get<float>(), skillData["forwardHitBoxHeight"].get<float>()};
                Vector2 backwardHitBox = (Vector2){skillData["backwardHitBoxWidth"].get<float>(), skillData["backwardHitBoxHeight"].get<float>()};

                float skillEnergy = skillData["energy"].get<float>();
                float skillDamage = skillData["damage"].get<float>();
                float mediumCost = skillData["mediumCost"].get<float>();
                float skillKnockback = skillData["knockback"].get<float>();
                float skillWindupTime = skillData["windupTime"].get<float>();
                float skillActiveTime = skillData["activeTime"].get<float>();
                float skillRecoveryTime = skillData["recoveryTime"].get<float>();
                float skillStunTime = skillData["stunTime"].get<float>();
                float skillSlowdownFactor = skillData["slowdownFactor"].get<float>();
                AnimationStates animationState = ( AnimationStates)skillData["animationState"].get<int>();
                InputTypes input = ( InputTypes)skillData["input"].get<int>();

                //Particles
                Emitter* destroyedParticles = LoadParticleEmitter("material_burst_default"); 

                skills.push_back(Skill(force, upHitBox, downHitBox, forwardHitBox, backwardHitBox, skillEnergy, skillDamage, 
                    mediumCost, skillKnockback, skillWindupTime, skillActiveTime, skillRecoveryTime, skillStunTime, skillSlowdownFactor, animationState, input, destroyedParticles));
            }
        }
        
        std::cout << "Character: Loaded Skills" << std::endl;

        // Load Character
        Vector2 size = (Vector2){characterData["width"], characterData["height"]};
        Vector2 hurtBoxSize = (Vector2){characterData["hurtBoxWidth"], characterData["hurtBoxHeight"]};
        float moveSpeed = characterData["moveSpeed"].get<float>();
        float jumpForce = characterData["jumpForce"].get<float>();
        float health = characterData["health"].get<float>();
        float medium = characterData["medium"].get<float>();
        float mediumChargeRate = characterData["mediumChargeRate"].get<float>();

        Character* character = new Character(world, initialPosition, size, hurtBoxSize, moveSpeed, jumpForce, health, medium, mediumChargeRate,
             characterType, meleeWeapon, gun, spriteSettings, skills);
        std::cout << "Character: Finished Loading" << std::endl;
        return character;

    } catch (json::exception& e) {
        std::cerr << "JSON parsing error (loading default character): " << e.what() << std::endl;

    } catch (...) {
        std::cerr << "Character load error, loading default character." << std::endl;
    }

    // Create default character
    // Sprite Setting parameters
    Vector2 spriteOffset = {-120.0f, -20.0f};
    float spriteScale = 1.25f;
    float framesPerSecond = 12.0f;
    std::unordered_map< AnimationStates, int> frameCounts;

    frameCounts[AnimationStates::IDLE] = 1;
    frameCounts[AnimationStates::JUMP] = 1;
    frameCounts[AnimationStates::RUN] = 4;
    frameCounts[AnimationStates::WINDUP] = 1;
    frameCounts[AnimationStates::ACTIVE] = 1;
    frameCounts[AnimationStates::RECOVERY] = 1;
    frameCounts[AnimationStates::GUN] = 1;
    frameCounts[AnimationStates::STUN] = 1;
    frameCounts[AnimationStates::CHARGE] = 1;

    std::unordered_map<AnimationStates, Shader> shaders;
    shaders[AnimationStates::IDLE] = LoadShader("water", "idle");
    shaders[AnimationStates::JUMP] = LoadShader("water", "jump");
    shaders[AnimationStates::RUN] = LoadShader("water", "run");
    shaders[AnimationStates::WINDUP] = LoadShader("water", "windup");
    shaders[AnimationStates::ACTIVE] = LoadShader("water", "active");
    shaders[AnimationStates::RECOVERY] = LoadShader("water", "recovery");
    shaders[AnimationStates::GUN] = LoadShader("water", "gun");
    shaders[AnimationStates::STUN] = LoadShader("water", "stun");
    shaders[AnimationStates::CHARGE] = LoadShader("water", "charge");

    std::string folderName = "water";
    SpriteSettings defaultSpriteSettings(spriteOffset, folderName, spriteScale, framesPerSecond, frameCounts, shaders);
    
    // Melee Weapon parameters
    Vector2 hitBox = { 100.0f, 70.0f };
    float damage = 5;
    float energy = 5.0f;
    float knockback = 10.0f;
    float windupTime = 0.05f;
    float activeTime = 0.1f;
    float recoveryTime = 0.05f;
    float stunTime = 0.5f;
    float slowdownFactor = 2.0f;
    float mediumAffinity = 10;

    MeleeWeapon defaultMeleeWeapon(hitBox, damage, mediumAffinity, energy, knockback, windupTime, activeTime, stunTime, recoveryTime, slowdownFactor);

    // Gun parameters
    float fireRate = 0.2f;
    int maxAmmo = 10;
    damage = 2;
    float reloadTime = 3.0f;
    float range = 1000.0f;
    energy = 50.0f;
    stunTime = 0.5f;
    slowdownFactor = 2.0f;
    mediumAffinity = 5;

    Gun defaultGun(fireRate, maxAmmo, damage, mediumAffinity, reloadTime, range, energy, knockback, stunTime, slowdownFactor);
    
    // Player parameters
    Vector2 size = {60.0f, 120.0f};
    Vector2 hurtBox = {30.0f, 60.0f};
    float moveSpeed = 190.0f * 4.0f;
    float jumpForce = 450.0f * 4.0f;
    float maxHealth = 100;
    float maxMedium = 100;
    float mediumChargeRate = 100;
    
    std::vector<Skill> defaultSkills;

    Character* defaultCharacter = new Character(world, initialPosition, size, hurtBox, moveSpeed, jumpForce, maxHealth, maxMedium, mediumChargeRate,
        characterType, defaultMeleeWeapon, defaultGun, defaultSpriteSettings, defaultSkills);
    return defaultCharacter;
}

Level* LoadLevel(std::string name, std::vector<std::string> characterLoadIds) {
        
    b2WorldDef worldDef = b2DefaultWorldDef(); // Get default world definition
    worldDef.gravity = {0.0f, GRAVITY};    // Set the gravity in the definition (b2Vec2 can be initialized directly)
    b2WorldId worldId = b2CreateWorld(&worldDef);          // Pass the address of the world definition

    std::vector<Character*> characters;
    std::vector<DestructibleBody*> destructibleBodies;
    std::vector<FreeDebris*> debrisInstances;

    std::vector<SpriteElement> backgroundElements;
    std::vector<SpriteElement> foregroundElements;

    Shader identityShader = LoadShader(0, (std::string("resources/shaders/") + std::string(GLSL_VERSION) + std::string("/identity.fs")).c_str());

    try {
        std::ifstream f("data.json");
        json data = json::parse(f);
        json levelData = FindObjectByName(data["levels"], name);

        for (int i = 0; i < characterLoadIds.size(); i++) {
            if (i == 0) {
                characters.push_back(LoadCharacter(worldId, characterLoadIds[i], Vector2{levelData["player1PositionX"].get<float>(), levelData["player1PositionY"].get<float>()},
                CharacterTypes::PLAYER_1));
            } else {
                characters.push_back(LoadCharacter(worldId, characterLoadIds[i], Vector2{levelData["player2PositionX"].get<float>(), levelData["player2PositionY"].get<float>()},
                CharacterTypes::PLAYER_2));
            }
        }
        
        std::cout << "Level: Loaded Characters" << std::endl;

        for (const auto& destructibleBodyData : levelData["destructibleBodies"]) {
                Vector2 position = {destructibleBodyData["positionX"].get<float>(), destructibleBodyData["positionY"].get<float>()};
                float rotation = destructibleBodyData["rotation"].get<float>();
                float width = destructibleBodyData["width"].get<float>();
                float height = destructibleBodyData["height"].get<float>();
                int squareSize = destructibleBodyData["squareSize"].get<int>();
                bool dynamic = destructibleBodyData["dynamic"].get<bool>();

                 // Load Material
                json materialData = FindObjectByName(data["materials"], destructibleBodyData["material"]);

                Texture2D texture = LoadTexture(("resources/sprites/materials/" + materialData["texture"].get<std::string>()).c_str());
                Texture2D debrisTexture = LoadTexture(("resources/sprites/materials/" + materialData["debrisTexture"].get<std::string>()).c_str());
                int textureNumShapes = materialData["textureNumSquares"].get<int>();
                int debrisTextureNumShapes = materialData["debrisTextureNumSquares"].get<int>();
                float density = materialData["density"].get<float>();
                float energyCapacity = materialData["energyCapacity"].get<float>();
                float debrisEnergyCapacity = materialData["debrisEnergyCapacity"].get<float>();

                BodyMaterial material(texture, debrisTexture, textureNumShapes, debrisTextureNumShapes, density, energyCapacity, debrisEnergyCapacity);

                destructibleBodies.push_back(new DestructibleBody(worldId, material, position, rotation * (PI / 180), dynamic, width, height, squareSize));
        }

        std::cout << "Level: Loaded Destructible Bodies" << std::endl;

        for (const auto& debrisInstanceData : levelData["debrisInstances"]) {

            Vector2 position = (Vector2) {debrisInstanceData["positionX"].get<float>(), debrisInstanceData["positionY"].get<float>()};
            b2Vec2 b2Position = {position.x * METERS_PER_PIXEL, position.y * METERS_PER_PIXEL};

            float energyCapacity = debrisInstanceData["energyCapacity"].get<float>();
            float rotation = 0;

            float vertex1PositionX = debrisInstanceData["vertex1PositionX"].get<float>();
            float vertex1PositionY = debrisInstanceData["vertex1PositionY"].get<float>();
            float vertex2PositionX = debrisInstanceData["vertex2PositionX"].get<float>();
            float vertex2PositionY = debrisInstanceData["vertex2PositionY"].get<float>();
            float vertex3PositionX = debrisInstanceData["vertex3PositionX"].get<float>();
            float vertex3PositionY = debrisInstanceData["vertex3PositionY"].get<float>();

            // Create polygon
            b2Vec2 vertices[3];
            vertices[0] = {vertex1PositionX * METERS_PER_PIXEL + b2Position.x, vertex1PositionY * METERS_PER_PIXEL + b2Position.y};
            vertices[1] = {vertex2PositionX * METERS_PER_PIXEL + b2Position.x, vertex2PositionY * METERS_PER_PIXEL + b2Position.y};
            vertices[2] = {vertex3PositionX * METERS_PER_PIXEL + b2Position.x, vertex3PositionY * METERS_PER_PIXEL + b2Position.y};

            b2Hull hull = b2ComputeHull(vertices, 3);
            b2Polygon polygon = b2MakePolygon(&hull, COLLISION_MARGIN);

            // Load Material Data
            json materialData = FindObjectByName(data["materials"], debrisInstanceData["material"]);
            
            Texture2D debrisTexture = LoadTexture(materialData["debrisTexture"].get<std::string>().c_str());
            int textureNumShapes = materialData["debrisTextureNumShapes"].get<int>();

            debrisInstances.push_back(new FreeDebris(worldId, position, rotation, energyCapacity, polygon, debrisTexture, textureNumShapes));
        }

        std::cout << "Level: Loaded Debris Instances" << std::endl;

        // Load background elements
        for (const auto& backgroundElementData : levelData["backgroundElements"]) {
            Vector2 offset = {backgroundElementData["offsetX"].get<float>(), backgroundElementData["offsetY"].get<float>()};
            float scale = backgroundElementData["scale"].get<float>();

            float parallax = backgroundElementData["parallax"].get<float>();

            bool loopX = backgroundElementData["loopX"].get<bool>();
            bool loopY = backgroundElementData["loopY"].get<bool>();

            std::string textureName = "resources/sprites/layers/" + backgroundElementData["texture"].get<std::string>();
            Texture2D texture = LoadTexture(textureName.c_str());

            backgroundElements.push_back(SpriteElement(offset, scale, texture, parallax, loopX, loopY));
        }

        // Load foreground elements
        for (const auto& foregroundElementData : levelData["foregroundElements"]) {
            Vector2 offset = {foregroundElementData["offsetX"].get<float>(), foregroundElementData["offsetY"].get<float>()};
            float scale = foregroundElementData["scale"].get<float>();

            float parallax = foregroundElementData["parallax"].get<float>();

            bool loopX = foregroundElementData["loopX"].get<bool>();
            bool loopY = foregroundElementData["loopY"].get<bool>();

            std::string textureName = "resources/sprites/layers/" + foregroundElementData["texture"].get<std::string>();
            Texture2D texture = LoadTexture(textureName.c_str());

            foregroundElements.push_back(SpriteElement(offset, scale, texture, parallax, loopX, loopY));
        }

        std::cout << "Level: Loaded Background Elements" << std::endl;

        Shader postProcessingShader = LoadShader(0, (std::string("resources/shaders/") + std::string(GLSL_VERSION) + std::string("/")
         + levelData["postProcessingShader"].get<std::string>()).c_str());

        std::cout << "Level: Loaded Shader" << std::endl;

        Level *level = new Level(worldId, postProcessingShader, characters, destructibleBodies, debrisInstances, backgroundElements, foregroundElements);
        return level;
    } catch (json::exception& e) {
        std::cerr << "JSON parsing error (loading default level): " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Level load error, loading default level." << std::endl;
    }

    // Load default level
    characters.push_back(LoadCharacter(worldId, characterLoadIds[0], Vector2{0, 0}, CharacterTypes::PLAYER_1));
    characters.push_back(LoadCharacter(worldId, characterLoadIds[1], Vector2{0, 0}, CharacterTypes::PLAYER_2));

    //destructibleBodies.push_back(new DestructibleBody(worldId, Vector2{0, 0}, 0, true, 1, 1, 100, 100, 1));
    //Emitter* destroyedParticles = LoadParticleEmitter("material_burst_default");

    //Default material
    BodyMaterial defaultMaterial = BodyMaterial(LoadTexture("resources/sprites/materials/dirt.png"), LoadTexture("resources/sprites/materials/dirt.png"), 1, 1, 1.0f, 1.0f, 1.0f);
    destructibleBodies.push_back(new DestructibleBody(worldId, defaultMaterial, (Vector2){250.0f, 400.0f}, 0.0f, false, 5000.0f, 500.0f, 100.0f));

    Level *defaultLevel = new Level(worldId, identityShader, characters, destructibleBodies, debrisInstances, backgroundElements, foregroundElements);
    return defaultLevel;
}

int GetFrameIndex(AnimationStates animationState, std::unordered_map<AnimationStates, int> frameCounts) {
    
    int frameIndex = 0;
    for (int i = 0; i < (int) animationState; i++) {
        frameIndex += frameCounts[(AnimationStates)i];
    }

    return frameIndex;
}

// MENU
Menu::Menu(std::string title, bool isPauseMenu, float buttonSpacing, Vector2 titlePosition, Vector2 buttonPosition, Vector2 subtextPosition, Vector2 imagePosition) :
    title(title), isPauseMenu(isPauseMenu), buttonSpacing(buttonSpacing), titlePosition(titlePosition), buttonPosition(buttonPosition), subtextPosition(subtextPosition), imagePosition(imagePosition), drawImages(true), loaded(false) {}

Menu::Menu(std::string title, bool isPauseMenu, float buttonSpacing, Vector2 titlePosition, Vector2 buttonPosition, Vector2 subtextPosition) :
    title(title), isPauseMenu(isPauseMenu), buttonSpacing(buttonSpacing), titlePosition(titlePosition), buttonPosition(buttonPosition), subtextPosition(subtextPosition), imagePosition(imagePosition), drawImages(false), loaded(false) {}

void Menu::AddOption(Option option) {
    options.push_back(option);
    optionPositions.push_back((Vector2){buttonPosition.x, buttonPosition.y + optionPositions.size() * buttonSpacing});
}

    void Menu::LoadMenu() {
    loaded = true;
}

void Menu::UnloadMenu() {
    loaded = false;
}

void Menu::Update() {
    if (loaded) {
        if (isPauseMenu) {
            ClearBackground(PAUSE_BG_COLOR);

            if (IsKeyPressed(KEY_ESCAPE)) {
                UnloadMenu();
            }
        } else {
            ClearBackground(MENU_BG_COLOR);
        }
        
        DrawText(title.c_str(), titlePosition.x, titlePosition.y, TITLE_SIZE, TITLE_COLOR);
        
        for (int i = 0; i < options.size(); i++) {
            if (options[i].hovered) {
                DrawText(options[i].text.c_str(), optionPositions[i].x, optionPositions[i].y, TEXT_SIZE, HOVER_COLOR);
            } else {
                DrawText(options[i].text.c_str(), optionPositions[i].x, optionPositions[i].y, TEXT_SIZE, TEXT_COLOR);
            }

            if (drawImages && options[i].hovered) {
                DrawTexture(options[i].image, imagePosition.x, imagePosition.y, WHITE);
            }

            if (options[i].subtext != "" && options[i].hovered) {
                DrawText(options[i].subtext.c_str(), subtextPosition.x, subtextPosition.y, SUBTEXT_SIZE, SUBTEXT_COLOR);
            }
        
            // Determine if hovered
            if (CheckCollisionPointRec(GetMousePosition(), (Rectangle){optionPositions[i].x, optionPositions[i].y, options[i].text.length() * TEXT_SIZE, TEXT_SIZE})) {
                options[i].hovered = true;
            } else {
                options[i].hovered = false;
            }

            // Handle clicks
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && options[i].hovered) {
                UnloadMenu();
            }
        }

    }
}

// CHARACTER
Character::Character(b2WorldId worldId, Vector2 initialPosition, Vector2 bodySize, Vector2 hurtBoxSize, float moveSpeed, float jumpForce, float maxHealth, float maxMedium,
     float mediumChargeRate, CharacterTypes characterType, MeleeWeapon equippedWeapon, Gun equippedGun, SpriteSettings spriteSettings, std::vector<Skill> availableSkills)
    : position(initialPosition), maxHealth(maxHealth), maxMedium(maxMedium), health(maxHealth), medium(maxMedium), chargeRate(mediumChargeRate), characterType(characterType), weapon(equippedWeapon), gun(equippedGun), world(worldId), ammo(equippedGun.maxAmmo),
    coyoteTimeCounter(0.0f), spriteSet(spriteSettings), moveSpeedPixelsPerSec(moveSpeed), jumpForcePixels(jumpForce), size(bodySize), hurtBox(hurtBoxSize), skills(availableSkills), activeSkill(availableSkills[0])
{   
    // 1. Define the Box2D Body Definition
    b2BodyDef bodyDef = b2DefaultBodyDef(); // Use default constructor for b2BodyDef
    bodyDef.type = b2_dynamicBody;

    // Convert initial position from pixels to Box2D meters (Box2D origin is center of body)
    // Raylib rect.x/y is top-left, Box2D body position is center.
    bodyDef.position = {(size.x / 2.0f) * METERS_PER_PIXEL + initialPosition.x * METERS_PER_PIXEL, (size.y / 2.0f) * METERS_PER_PIXEL + initialPosition.y * METERS_PER_PIXEL};

    bodyDef.linearDamping = 0.0f;
    bodyDef.angularDamping = 999999999999999.0f;

    bodyDef.name = "Character";
    bodyDef.userData = this;

    // Create the body in the Box2D world using b2CreateBody
    body = b2CreateBody(world, &bodyDef);

    // 2. Define the Box2D Shape (Rectangle)
    float radius = size.x * METERS_PER_PIXEL / 4.0f;
    
    b2ShapeDef playerShapeDef = b2DefaultShapeDef();
    playerShapeDef.material.friction = CHARACTER_FRICTION;
    b2Polygon playerBox = b2MakeRoundedBox(size.x * METERS_PER_PIXEL / 2.0f - radius, size.y * METERS_PER_PIXEL / 2.0f - radius, radius);

    b2CreatePolygonShape(body, &playerShapeDef, &playerBox);

    // Initialize raylib rect and position for drawing.
    rect = (Rectangle){ 0, 0, size.x, size.y };
    position = (Vector2){0, 0};

    b2Vec2 center = {0.0f, 0.0f};
    float mass = DEFAULT_CHARACTER_MASS;
    float rotationalInertia = 0.0f;
    b2MassData massData = {mass, center, rotationalInertia};

    b2Body_SetMassData(body, massData);

    // Set all timers and states
    for (int i = 0; i < (int) CharacterStates::Size; i++) {
        timers[(CharacterStates)i] = 0.0f;
    }

    for (int i = 0; i < (int) CharacterStates::Size; i++) {
        states[(CharacterStates)i] = false;
    }
}

// Destructor to clean up Box2D body
Character::~Character() {
    // Use b2DestroyBody to remove the body from the world
    b2DestroyBody(body);
    body = b2BodyId{}; // Reset the ID
    for (int i = 0; i < skills.size(); i++)
    {
        Emitter_Free(skills[i].particles);
    }
    
}

// Update Character's position and rect from Box2D body
void Character::Update() {
    b2Body_SetAngularVelocity(body, 0.0f);

    // Get the position from the Box2D body (in meters) using b2Body_GetPosition
    b2Vec2 bodyPos = b2Body_GetPosition(body);

    // Convert to pixel coordinates for raylib drawing.
    position.x = bodyPos.x * PIXELS_PER_METER;
    position.y = bodyPos.y * PIXELS_PER_METER;
    
    //Timers
    for (int i = 0; i < timers.size(); i++) {
        if (states[(CharacterStates)i]) {
            timers[(CharacterStates)i] -= GetFrameTime();
        }
    }

    if (timers[CharacterStates::WEAPON] <= 0) {
        states[CharacterStates::WEAPON] = false;
    } else {
        if (timers[CharacterStates::WEAPON] >= weapon.recoveryTime && timers[CharacterStates::WEAPON] <= weapon.recoveryTime + weapon.activeTime) {
            ProcessMeleeAttack();
        }
    }

    if (timers[CharacterStates::GUN] <= 0) {
        states[CharacterStates::GUN] = false;
    }

    if (timers[CharacterStates::RELOAD] <= 0 && states[CharacterStates::RELOAD]) {
        ammo = gun.maxAmmo;
        states[CharacterStates::RELOAD] = false;
    }

    if (timers[CharacterStates::SKILL] <= 0) {
        states[CharacterStates::SKILL] = false;
    } else {
        if (states[CharacterStates::SKILL] && timers[CharacterStates::SKILL] >= activeSkill.recoveryTime && timers[CharacterStates::SKILL] <= activeSkill.recoveryTime + activeSkill.activeTime) {
            ProcessSkill();
        }
    }

    if (timers[CharacterStates::STUN] <= 0) {
        states[CharacterStates::STUN] = false;
    }

    // --- Input handling  ---
    if (!states[CharacterStates::STUN]) {
        if (!states[CharacterStates::CHARGE]) {

        // Horizontal Movement
        if (CheckInput(InputTypes::LEFT, characterType)) {
            MoveLeft();
        } else if (CheckInput(InputTypes::RIGHT, characterType)) {
            MoveRight();
        } else {
            // Stop horizontal movement if no key is pressed; ensure impulses are conserved
            b2Vec2 currentVel = b2Body_GetLinearVelocity(body);

            if (currentVel.x >= moveSpeedPixelsPerSec * METERS_PER_PIXEL - VELOCITY_TOLERANCE) {
                currentVel.x -= moveSpeedPixelsPerSec * METERS_PER_PIXEL;
            } else if (currentVel.x <= -moveSpeedPixelsPerSec * METERS_PER_PIXEL + VELOCITY_TOLERANCE) {
                currentVel.x += moveSpeedPixelsPerSec * METERS_PER_PIXEL;
            }
            
            b2Body_SetLinearVelocity(body, currentVel); // Use b2Body_SetLinearVelocity
        }
        
        // --- Coyote Time Logic ---
        bool currentlyGrounded = IsGrounded();

        if (currentlyGrounded) {
            coyoteTimeCounter = COYOTE_TIME; // Reset coyote time if grounded
        } else {
            coyoteTimeCounter -= GetFrameTime(); // Decrement if not grounded
        }

        // Check for skill input
        if (CheckInput(InputTypes::SKILL, characterType)) {
            for (int i = 0; i < skills.size(); i++) {
                if (CheckInput(skills[i].input, characterType) && medium >= skills[i].mediumCost) {
                    StartSkill(skills[i]);
                    break;
                }
            }
        } else {
            // Jumping
            if (CheckInput(InputTypes::JUMP, characterType) && (currentlyGrounded || coyoteTimeCounter > 0)) {
                Jump();
            }

            // Shooting
            if (CheckInput(InputTypes::SHOOT, characterType) && !states[CharacterStates::GUN] && !states[CharacterStates::RELOAD] && ammo > 0) {
                Shoot();
            }

            // Melee Weapon
            if (CheckInput(InputTypes::ATTACK, characterType) && !states[CharacterStates::WEAPON]) {
                StartMeleeAttack();
            }
        }
    }
        if (CheckInput(InputTypes::CHARGE, characterType)) {
            states[CharacterStates::CHARGE] = true;
            Charge();
        } else {
            states[CharacterStates::CHARGE] = false;
        }
    }

    b2Body_SetAngularVelocity(body, 0.0f);
    b2Body_ApplyForceToCenter(body, {0, CHARACTER_GRAVITY_ADDON}, false);
}

void Character::SetPosition(Vector2 newPosition) {
    position = newPosition;
    // Update Box2D body's position
    b2Body_SetTransform(body, {newPosition.x * METERS_PER_PIXEL, newPosition.y * METERS_PER_PIXEL}, b2Body_GetRotation(body));
}

void Character::Draw() {
    // Determine character's animation state; jump has priority over run
    int selectedIndex = 0;

    if (!isFacingRight) {
        selectedIndex += spriteSet.totalFrames;
    }
    
    if (states[CharacterStates::STUN]) {
        animationState = AnimationStates::STUN;
    } else if (states[CharacterStates::CHARGE]) {
        animationState = AnimationStates::CHARGE;
    } else if (states[CharacterStates::GUN]) {
        animationState = AnimationStates::GUN;
    } else if (states[CharacterStates::WEAPON] && timers[CharacterStates::WEAPON] >= weapon.recoveryTime + weapon.activeTime) {
        animationState = AnimationStates::WINDUP;
    } else if (states[CharacterStates::WEAPON] && timers[CharacterStates::WEAPON] >= weapon.recoveryTime && timers[CharacterStates::WEAPON] <= weapon.activeTime + weapon.recoveryTime) {
        animationState = AnimationStates::ACTIVE;
    } else if (states[CharacterStates::WEAPON] && timers[CharacterStates::WEAPON] < weapon.recoveryTime) {
        animationState = AnimationStates::RECOVERY;
    } else if (coyoteTimeCounter <= 0) {
        animationState = AnimationStates::JUMP;
    } else if (CheckInput(InputTypes::LEFT, characterType) || CheckInput(InputTypes::RIGHT, characterType)) {
        animationState = AnimationStates::RUN;
    } else if (states[CharacterStates::SKILL]) {
        animationState = activeSkill.animationState;
    } else {
        animationState = AnimationStates::IDLE;
    }

    selectedIndex += GetFrameIndex(animationState, spriteSet.frameCounts);

    if (currentFrame + selectedIndex >= spriteSet.sprites.size() || currentFrame + selectedIndex < 0) {
        selectedIndex = 0;
        currentFrame = 0;
    }

    BeginShaderMode(spriteSet.shaders[animationState]);
    DrawTextureEx(spriteSet.sprites[currentFrame + selectedIndex], (Vector2){position.x - size.x / 2.0f + spriteSet.offset.x, 
        position.y - size.y / 2.0f + spriteSet.offset.y}, 0.0f, rect.height * spriteSet.scale / spriteSet.sprites[0].height, WHITE);
    EndShaderMode();
    
    // Update animation frame
    frameTimer += GetFrameTime();
    if (frameTimer >= 1.0f / spriteSet.framesPerSecond) {
        frameTimer = 0.0f;

        currentFrame = (currentFrame + 1) % spriteSet.frameCounts[animationState];
    }
}

// These functions apply forces/impulses to the Box2D body
void Character::MoveLeft() {
    isFacingRight = false;
    b2Vec2 vel = b2Body_GetLinearVelocity(body);

    if (states[CharacterStates::WEAPON]) {
        vel.x = -moveSpeedPixelsPerSec * METERS_PER_PIXEL / weapon.slowdownFactor;
    } else if (states[CharacterStates::GUN]) {
        vel.x = -moveSpeedPixelsPerSec * METERS_PER_PIXEL / gun.slowdownFactor; // Target velocity in meters/sec
    } else {
        vel.x = -moveSpeedPixelsPerSec * METERS_PER_PIXEL;
    }

    b2Body_SetLinearVelocity(body, vel);
}

void Character::MoveRight() {
    isFacingRight = true;
    b2Vec2 vel = b2Body_GetLinearVelocity(body);

    if (states[CharacterStates::WEAPON]) {
        vel.x = moveSpeedPixelsPerSec * METERS_PER_PIXEL / weapon.slowdownFactor;
    } else if (states[CharacterStates::GUN]) {
        vel.x = moveSpeedPixelsPerSec * METERS_PER_PIXEL / gun.slowdownFactor;
    } else {
        vel.x = moveSpeedPixelsPerSec * METERS_PER_PIXEL;
    }

    b2Body_SetLinearVelocity(body, vel);
}

void Character::Jump() {
    coyoteTimeCounter = 0;

    b2Vec2 currentVel = b2Body_GetLinearVelocity(body);
    currentVel.y = 0.0f;
    b2Body_SetLinearVelocity(body, currentVel); // Use b2Body_SetLinearVelocity

    b2Vec2 impulse = {0.0f, -jumpForcePixels * METERS_PER_PIXEL * b2Body_GetMass(body)};
    b2Body_ApplyLinearImpulse(body, impulse, {0, 0}, true);
}

void Character::StartMeleeAttack() {
    states[CharacterStates::WEAPON] = true;
    timers[CharacterStates::WEAPON] = weapon.windupTime + weapon.activeTime + weapon.recoveryTime;
}

void Character::ProcessMeleeAttack() {
    b2AABB aabb;

    if (isFacingRight) {
        aabb.lowerBound = (b2Vec2){(position.x + size.x / 2.0f) * METERS_PER_PIXEL + JITTER_TOLERANCE, (position.y - weapon.hitBox.y / 2.0f) * METERS_PER_PIXEL};
        aabb.upperBound = (b2Vec2){(position.x + size.x / 2.0f + weapon.hitBox.x) * METERS_PER_PIXEL + JITTER_TOLERANCE, (position.y + weapon.hitBox.y / 2.0f) * METERS_PER_PIXEL};
    } else {
        aabb.lowerBound = (b2Vec2){(position.x - size.x / 2.0f - weapon.hitBox.x) * METERS_PER_PIXEL - JITTER_TOLERANCE, (position.y - weapon.hitBox.y / 2.0f) * METERS_PER_PIXEL};
        aabb.upperBound = (b2Vec2){(position.x - size.x / 2.0f) * METERS_PER_PIXEL - JITTER_TOLERANCE, (position.y + weapon.hitBox.y / 2.0f) * METERS_PER_PIXEL};
    }

    b2World_OverlapAABB(world, aabb, b2DefaultQueryFilter(), OverlapCallback, this);
}

void Character::Shoot() {
    states[CharacterStates::GUN] = true;
    ammo -= 1;
    timers[CharacterStates::GUN] = gun.fireTime;

    b2Vec2 origin;
    b2Vec2 translation;

    if (isFacingRight) {
        origin = {position.x * METERS_PER_PIXEL + JITTER_TOLERANCE + size.x * METERS_PER_PIXEL / 2.0f, position.y * METERS_PER_PIXEL};
        translation = {gun.range * METERS_PER_PIXEL, 0.0f};
    } else {
        origin = {position.x * METERS_PER_PIXEL - JITTER_TOLERANCE - size.x * METERS_PER_PIXEL / 2.0f, position.y * METERS_PER_PIXEL};
        translation = {-gun.range * METERS_PER_PIXEL, 0.0f};
    }

    if (b2IsValidVec2(origin) && b2IsValidVec2(translation)) {
        RayCastContext context = {0};
        b2World_CastRay(world, origin, translation, b2DefaultQueryFilter(), RayCastCallback, &context);
        bool hitCharacter = HandleAttack(gun.damage, gun.energy, gun.knockback, gun.stunTime, b2Body_GetPosition(body), context.shape, true);

        if (hitCharacter) {
            medium = std::min(maxMedium, medium + gun.mediumAffinity);
        }
    }

    if (ammo == 0) {
        timers[CharacterStates::RELOAD] = gun.reloadTime;
        states[CharacterStates::RELOAD] = true;
    }
}

void Character::StartSkill(Skill skill) {
    //Apply force
    b2Body_SetLinearVelocity(body, {0, 0});
    b2Vec2 impulse = {skill.force.x * METERS_PER_PIXEL * b2Body_GetMass(body), -skill.force.y * METERS_PER_PIXEL * b2Body_GetMass(body)};
    b2Body_ApplyLinearImpulse(body, impulse, {0, 0}, true);

    medium -= skill.mediumCost;

    states[CharacterStates::SKILL] = true;
    timers[CharacterStates::SKILL] = skill.windupTime + skill.activeTime + skill.recoveryTime;

    activeSkill = skill;
}

void Character::ProcessSkill() {
    // Create hitboxes
    b2AABB upHitBox;
    b2AABB downHitBox;
    b2AABB forwardHitBox;
    b2AABB backwardHitBox;

    upHitBox.lowerBound = (b2Vec2){(position.x - hurtBox.x / 2.0f) * METERS_PER_PIXEL, (position.y + size.y / 2.0f + hurtBox.y / 2.0f) * METERS_PER_PIXEL};
    upHitBox.upperBound = (b2Vec2){(position.x + hurtBox.x / 2.0f) * METERS_PER_PIXEL, (position.y + size.y / 2.0f + hurtBox.y / 2.0f + activeSkill.upHitBox.y) * METERS_PER_PIXEL};

    downHitBox.lowerBound = (b2Vec2){(position.x - hurtBox.x / 2.0f) * METERS_PER_PIXEL, (position.y - size.y / 2.0f - hurtBox.y / 2.0f - activeSkill.downHitBox.y) * METERS_PER_PIXEL};
    downHitBox.upperBound = (b2Vec2){(position.x + hurtBox.x / 2.0f) * METERS_PER_PIXEL, (position.y - size.y / 2.0f - hurtBox.y / 2.0f) * METERS_PER_PIXEL};

    if (isFacingRight) {
        forwardHitBox.lowerBound = (b2Vec2){(position.x + size.x / 2.0f + hurtBox.x / 2.0f) * METERS_PER_PIXEL, (position.y - hurtBox.y / 2.0f) * METERS_PER_PIXEL};
        forwardHitBox.upperBound = (b2Vec2){(position.x + size.x / 2.0f + hurtBox.x / 2.0f + activeSkill.forwardHitBox.x) * METERS_PER_PIXEL, (position.y + hurtBox.y / 2.0f) * METERS_PER_PIXEL};

        backwardHitBox.lowerBound = (b2Vec2){(position.x - size.x / 2.0f - hurtBox.x / 2.0f - activeSkill.backwardHitBox.x) * METERS_PER_PIXEL, (position.y - hurtBox.y / 2.0f) * METERS_PER_PIXEL};
        backwardHitBox.upperBound = (b2Vec2){(position.x - size.x / 2.0f - hurtBox.x / 2.0f) * METERS_PER_PIXEL, (position.y + hurtBox.y / 2.0f) * METERS_PER_PIXEL};
    } else {
        forwardHitBox.lowerBound = (b2Vec2){(position.x - size.x / 2.0f - hurtBox.x / 2.0f - activeSkill.forwardHitBox.x) * METERS_PER_PIXEL, (position.y - hurtBox.y / 2.0f) * METERS_PER_PIXEL};
        forwardHitBox.upperBound = (b2Vec2){(position.x - size.x / 2.0f - hurtBox.x / 2.0f) * METERS_PER_PIXEL, (position.y + hurtBox.y / 2.0f) * METERS_PER_PIXEL};

        backwardHitBox.lowerBound = (b2Vec2){(position.x + size.x / 2.0f + hurtBox.x / 2.0f) * METERS_PER_PIXEL, (position.y - hurtBox.y / 2.0f) * METERS_PER_PIXEL};
        backwardHitBox.upperBound = (b2Vec2){(position.x + size.x / 2.0f + hurtBox.x / 2.0f + activeSkill.backwardHitBox.x) * METERS_PER_PIXEL, (position.y + hurtBox.y / 2.0f) * METERS_PER_PIXEL};
    }

    // Perform hitbox checks
    b2World_OverlapAABB(world, upHitBox, b2DefaultQueryFilter(), OverlapCallback, this);
    b2World_OverlapAABB(world, downHitBox, b2DefaultQueryFilter(), OverlapCallback, this);
    b2World_OverlapAABB(world, forwardHitBox, b2DefaultQueryFilter(), OverlapCallback, this);
    b2World_OverlapAABB(world, backwardHitBox, b2DefaultQueryFilter(), OverlapCallback, this);
}

void Character::Charge() {
    medium = std::min(GetMaxMedium(), medium + chargeRate * GetFrameTime());
}

bool Character::IsGrounded() {
    // Check 1: Vertical velocity is close to zero
    b2Vec2 currentVelocity = b2Body_GetLinearVelocity(body);
    bool isVerticalVelocityLow = (fabs(currentVelocity.y) < JITTER_TOLERANCE); // Using 0.1 m/s as threshold

    // Check 2: Player is contacting at least one other body
    int capacity = b2Body_GetContactCapacity(body);
    int numContacts = 0;

    if (capacity > 0) {
        // Allocate memory for contact data
        b2ContactData* contacts = (b2ContactData*)malloc(capacity * sizeof(b2ContactData));
        if (contacts) {
            numContacts = b2Body_GetContactData(body, contacts, capacity);
            free(contacts); // Free memory immediately after use
        } else {
            // Handle allocation failure if necessary
            std::cerr << "Warning: Failed to allocate memory for contacts in IsGrounded()." << std::endl;
            // If allocation fails, assume not grounded to prevent infinite jumps
            return false;
        }
    }

    bool isTouchingAnyBody = (numContacts > 0);

    // Both conditions must be true for the player to be considered grounded
    return isVerticalVelocityLow && isTouchingAnyBody;
}

void Character::SetStun(float stunTime) {
    states[CharacterStates::STUN] = true;
    timers[CharacterStates::STUN] = stunTime;
}

// LEVEL
void Level::UnloadLevel() {
    loaded = false;

    // Clear all pointers
    for (int i = 0; i < characters.size(); i++) {
        delete characters[i];
        characters[i] = nullptr;
    }

    for (int i = 0; i < destructibleBodies.size(); i++) {
        delete destructibleBodies[i];
        destructibleBodies[i] = nullptr;
    }

    for (int i = 0; i < freeDebrisInstances.size(); i++) {
        delete freeDebrisInstances[i];
        freeDebrisInstances[i] = nullptr;
    }

    characters.clear();
    destructibleBodies.clear();
    freeDebrisInstances.clear();

    b2DestroyWorld(world);
}

void Level::Update() {

    BeginTextureMode(renderTexture);

    // Camera/Background
    ClearBackground(BLACK);
    BeginMode2D(camera);

    Vector2 cameraPosition = camera.target;
    float zoom = camera.zoom;

    // Draw background elements
    for (int i = 0; i < backgroundElements.size(); i++) {
        float scale = backgroundElements[i].scale;
        float width = backgroundElements[i].texture.width * scale;
        float height = backgroundElements[i].texture.height * scale;
        float parallax = backgroundElements[i].parallax;
        Texture2D texture = backgroundElements[i].texture;

        Vector2 elementPosition = {(backgroundElements[i].offset.x - width / 2.0f) + cameraPosition.x * parallax, 
            (backgroundElements[i].offset.y - height / 2.0f) + cameraPosition.y * parallax};

        if (backgroundElements[i].loopX && backgroundElements[i].loopY) {
            Vector2 viewTopLeft = cameraPosition - (Vector2){(1/zoom) * SCREEN_WIDTH / 2, (1/zoom) * SCREEN_HEIGHT / 2};

            // 1. Calculate the starting offset (where the texture should start drawing)
            // 'textureWorldX' is the x-coordinate of the tile that should be at viewTopLeft.x
            float textureWorldX = elementPosition.x + (cameraPosition.x - SCREEN_WIDTH * (1 / zoom) / 2) * parallax;
            float offset_x = fmodf(viewTopLeft.x - textureWorldX, texture.width * scale);
            if (offset_x > 0) offset_x -= texture.width * scale; // Ensure negative offset if needed

            // 'textureWorldY' is the y-coordinate of the tile that should be at viewTopLeft.y
            float textureWorldY = elementPosition.y + (cameraPosition.y - SCREEN_HEIGHT * (1 / zoom) / 2) * parallax;
            float offset_y = fmodf(viewTopLeft.y - textureWorldY, texture.height * scale);
            if (offset_y > 0) offset_y -= texture.height * scale; // Ensure negative offset if needed

            // 2. Define the source rectangle (The key for tiling/wrapping)
            Rectangle sourceRec = {
                -offset_x - (SCREEN_HEIGHT * (1 / zoom) + texture.height * scale) / (2 * scale), // The negative offset in texture space (unscaled)
                -offset_y - (SCREEN_HEIGHT * (1 / zoom) + texture.height * scale) / (2 * scale),
                (SCREEN_WIDTH * (1 / zoom) + texture.width * scale) / (2 * scale), // Huge width in texture space
                (SCREEN_HEIGHT * (1 / zoom) + texture.height * scale) / (2 *scale) // Huge height in texture space
            };

            // 3. Draw a single quad that covers the screen
            DrawTexturePro(
                texture,
                sourceRec,
                (Rectangle){ viewTopLeft.x, viewTopLeft.y, SCREEN_WIDTH * (1 / zoom), SCREEN_HEIGHT * (1 / zoom) },
                {0, 0}, 
                0.0f, 
                WHITE
            );

        } else if (backgroundElements[i].loopX) {
            DrawTextureLoopX(texture, width, height, elementPosition, cameraPosition, scale, zoom);
        } else if (backgroundElements[i].loopY) {
            DrawTextureLoopY(texture, width, height, elementPosition, cameraPosition, scale, zoom);
        } else {
            DrawTextureEx(backgroundElements[i].texture, elementPosition, 0, scale, WHITE);
        }
    }
    
    // Get the current FPS
    int fps = GetFPS();
    // Convert the integer FPS to a string
    std::string fpsText = "FPS: " + std::to_string(fps);

    // Draw the FPS text on the screen
    // Arguments: text, posX, posY, fontSize, color
    DrawText(fpsText.c_str(), 10, 10, 20, DARKGRAY); // Top-left corner, font size 20, dark gray color

    if (loaded) {
        b2World_Step(world, GetFrameTime(), SUB_STEPS);

        for (int i = 0; i < destructibleBodies.size(); i++) {
            destructibleBodies[i]->Update();
            destructibleBodies[i]->Draw();
        }

        for (int i = 0; i < freeDebrisInstances.size(); i++) {
            freeDebrisInstances[i]->Update();
            freeDebrisInstances[i]->Draw();
        }
        
        for (int i = 0; i < characters.size(); i++) {
            if (characters[i]->health > 0) { 
                characters[i]->Update();
                characters[i]->Draw();
            }
        }
    }
    
    
    if (characters.size() < 2) {
        Vector2 newCameraPosition = (Vector2){ cameraPosition.x + CAM_X_INTERPOLATION * (characters[0]->GetPosition().x - cameraPosition.x), 
            cameraPosition.y + CAM_Y_INTERPOLATION * (characters[0]->GetPosition().y - cameraPosition.y) };
        camera.target = newCameraPosition;
    } else {
        Vector2 averageCameraPosition = (Vector2){characters[0]->GetPosition().x + characters[1]->GetPosition().x, characters[0]->GetPosition().y + characters[1]->GetPosition().y} / 2.0f;
        camera.target = averageCameraPosition;

        //Get distance between players
        float distance = Vector2Distance(characters[0]->GetPosition(), characters[1]->GetPosition());
        camera.zoom = camera.zoom + (std::min(CAM_MAX_ZOOM, CAM_ZOOM_RATE/distance) - camera.zoom) * CAM_ZOOM_INTERPOLATION;
    }

    // HUD
    // Define bar dimensions and padding
    float barWidth = 200.0f;
    float barHeight = 20.0f;
    float padding = 10.0f;

    // Iterate over all characters in the level
    for (const auto& character : characters) {
        if (character) { // Always check for nullptr
            Vector2 healthPosition;
            Vector2 mediumPosition;

            // Determine position based on character type
            switch (character->GetCharacterType()) {
                case CharacterTypes::PLAYER_1:
                    healthPosition = { padding, padding };
                    mediumPosition = { padding, padding + barHeight + padding };
                    break;
                case CharacterTypes::PLAYER_2:
                    healthPosition = { SCREEN_WIDTH / 2 - barWidth - padding, padding };
                    mediumPosition = { SCREEN_WIDTH / 2 - barWidth - padding, padding + barHeight + padding };
                    break;
                case CharacterTypes::CPU:
                    healthPosition = { SCREEN_WIDTH / 2 - barWidth - padding, padding };
                    mediumPosition = { SCREEN_WIDTH / 2 - barWidth - padding, padding + barHeight + padding };
                default:
                    // Default to top-left if type is unknown
                    healthPosition = { padding, padding };
                    mediumPosition = { padding, padding + barHeight + padding };
                    break;
            }

            int health = character->health;
            int mxHealth = character->GetMaxHealth();
            int medium = character->medium;
            int mxMedium = character->GetMaxMedium();

            healthPosition.x += cameraPosition.x - SCREEN_WIDTH / 4;
            healthPosition.y += cameraPosition.y - SCREEN_HEIGHT / 4;
            mediumPosition.x += cameraPosition.x - SCREEN_WIDTH / 4;
            mediumPosition.y += cameraPosition.y - SCREEN_HEIGHT / 4;

            // Draw background for health bar
            DrawRectangle(healthPosition.x, healthPosition.y, barWidth, barHeight, DARKGRAY);
            // Draw health bar (proportional to current health)
            float healthBarFill = (float)health / mxHealth * barWidth;
            DrawRectangle(healthPosition.x, healthPosition.y, healthBarFill, barHeight, GREEN);
            
            // Draw health text
            DrawText(TextFormat("HP: %d/%d", health, mxHealth), healthPosition.x, healthPosition.y, 16, WHITE);

            // Draw background for medium bar
            DrawRectangle(mediumPosition.x, mediumPosition.y, barWidth, barHeight, DARKGRAY);
            // Draw medium bar (proportional to current medium)
            float mediumBarFill = (float)medium / mxMedium * barWidth;
            DrawRectangle(mediumPosition.x, mediumPosition.y, mediumBarFill, barHeight, BLUE);
            
            // Draw medium text
            DrawText(TextFormat("Medium: %d/%d", medium, mxMedium), mediumPosition.x, mediumPosition.y, 16, WHITE);
        }
    }

    EndMode2D();
    EndTextureMode();
    
    BeginDrawing();

    ClearBackground(BLACK);
    BeginShaderMode(postProcessingShader);
    DrawTextureRec(renderTexture.texture, (Rectangle){ 0, 0, (float)renderTexture.texture.width, (float)-renderTexture.texture.height }, (Vector2){ 0, 0 }, WHITE);
    EndShaderMode();

    EndDrawing();
}

// DESTRUCTIBLE BODY

DestructibleBody::DestructibleBody(b2WorldId worldId, BodyMaterial material, Vector2 initialPosition, float initialRotation, bool dynamic, float width, float height, float squareSize)
    : world(worldId), material(material), triangles(0), position(initialPosition),
     rotation(initialRotation) // Initialize triangles to 0, will be calculated
{
    // Convert overall dimensions from pixels to meters
    float boxWidthMeters = width * METERS_PER_PIXEL;
    float boxHeightMeters = height * METERS_PER_PIXEL;
    float squareSizeMeters = squareSize * METERS_PER_PIXEL;

    // Calculate initial number of columns and rows based on desired squareSize
    int originalCols = static_cast<int>(std::round(boxWidthMeters / squareSizeMeters));
    int originalRows = static_cast<int>(std::round(boxHeightMeters / squareSizeMeters));

    // Ensure at least one row and one column
    if (originalCols < 1) originalCols = 1;
    if (originalRows < 1) originalRows = 1;

    // Calculate the 'snapped' cell dimensions
    float actualCellWidthMeters = boxWidthMeters / static_cast<float>(originalCols);
    float actualCellHeightMeters = boxHeightMeters / static_cast<float>(originalRows);

    // Update the 'squareSize' for the actual triangles based on the snapped dimensions.
    // We aim for cells that are as close to a perfect square as possible.
    // The 'effectiveSquareSize' is the average of the actual width and height of a cell.
    // This is useful if you want to know what "square size" was actually used.
    float effectiveSquareSizeMeters = (actualCellWidthMeters + actualCellHeightMeters) / 2.0f;

    // Now, determine the exact number of columns and rows based on these snapped dimensions.
    // These should ideally be originalCols and originalRows, but this recalculation
    // confirms the actual subdivision.
    int cols = originalCols;
    int rows = originalRows;

    // Update the `triangles` member based on the calculated `cols` and `rows`
    // Each rectangle (cell) forms 2 triangles.
    triangles = cols * rows * 2;

    // --- Body Creation ---
    b2BodyDef bodyDef = b2DefaultBodyDef();

    if (dynamic) {
        bodyDef.type = b2_dynamicBody;
    } else {
        bodyDef.type = b2_staticBody;
    }

    bodyDef.position = {initialPosition.x * METERS_PER_PIXEL, initialPosition.y * METERS_PER_PIXEL}; // Center of the overall box
    bodyDef.rotation = b2MakeRot(initialRotation * (PI / 180.0f));
    
    bodyDef.name = "DestructibleBody";
    bodyDef.userData = this;

    body = b2CreateBody(world, &bodyDef);

    // --- Shape Definition ---
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.material.friction = DEFAULT_FRICTION;
    shapeDef.material.restitution = DEFAULT_RESTITUTION;

    // --- Triangulation Logic ---
    float subRectWidthMeters = boxWidthMeters / static_cast<float>(cols);
    float subRectHeightMeters = boxHeightMeters / static_cast<float>(rows);

    float halfBoxWidthMeters = boxWidthMeters / 2.0f;
    float halfBoxHeightMeters = boxHeightMeters / 2.0f;

    int actualTriangleCount = 0; // To verify

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            // Calculate the local coordinates of the corners of the current sub-rectangle
            // Relative to the body's center (0,0)
            float localX_min = -halfBoxWidthMeters + c * subRectWidthMeters;
            float localY_min = -halfBoxHeightMeters + r * subRectHeightMeters;

            float localX_max = localX_min + subRectWidthMeters;
            float localY_max = localY_min + subRectHeightMeters;

            // Corners of the current sub-rectangle (relative to the body's center)
            b2Vec2 bl = {localX_min, localY_min}; // Bottom-left
            b2Vec2 br = {localX_max, localY_min}; // Bottom-right
            b2Vec2 tl = {localX_min, localY_max}; // Top-left
            b2Vec2 tr = {localX_max, localY_max}; // Top-right

            // Triangle 1: Bottom-left, Bottom-right, Top-left (CCW order)
            b2Vec2 vertices1[3] = {bl, br, tl};
            b2Hull triangle1Hull = b2ComputeHull(vertices1, 3);
            b2Polygon triangle1Polygon = b2MakePolygon(&triangle1Hull, COLLISION_MARGIN);
            b2CreatePolygonShape(body, &shapeDef, &triangle1Polygon);
            actualTriangleCount++;

            // Triangle 2: Top-right, Top-left, Bottom-right (CCW order)
            b2Vec2 vertices2[3] = {tl, tr, br};
            b2Hull triangle2Hull = b2ComputeHull(vertices2, 3);
            b2Polygon triangle2Polygon = b2MakePolygon(&triangle2Hull, COLLISION_MARGIN);
            b2CreatePolygonShape(body, &shapeDef, &triangle2Polygon);
            actualTriangleCount++;
        }
    }

    rect = (Rectangle){ position.x - width / 2.0f, position.y - height / 2.0f, width, height };
}

DestructibleBody::~DestructibleBody() {
    b2DestroyBody(body);
    body = b2BodyId{};
    //Emitter_Free(destroyedParticles);
}

void DestructibleBody::Update() {
    position = (Vector2){b2Body_GetPosition(body).x * PIXELS_PER_METER, b2Body_GetPosition(body).y * PIXELS_PER_METER};
    
    b2Rot rotationData = b2Body_GetTransform(body).q;
    rotation = b2Rot_GetAngle(rotationData);

    // Update debris
    for (int i = 0; i < debrisInstances.size(); i++) {
        if (!debrisInstances[i]->IsDestroyed()) {
            debrisInstances[i]->Update();
        }
    }

    // Collision detection and material destruction
    int contactCapacity = b2Body_GetContactCapacity(body);

    b2ContactData* contactData = (b2ContactData*)malloc(contactCapacity * sizeof(b2ContactData));
    int numContacts = b2Body_GetContactData(body, contactData, contactCapacity);

    if (contactCapacity > 0) {
        for (int i = 0; i < numContacts; i++)
        {
            b2ContactData* data = contactData + i;

            // Identify which shape is external
            b2ShapeId shapeA = data->shapeIdA;
            b2ShapeId shapeB = data->shapeIdB;

            if (b2Shape_IsValid(shapeA) && b2Shape_IsValid(shapeB)) {
                b2BodyId externalBody;
                b2ShapeId internalShape;

                if (b2Shape_GetBody(shapeA).index1 == body.index1 && b2Shape_GetBody(shapeA).generation == body.generation) {
                    externalBody = b2Shape_GetBody(shapeB);
                    internalShape = shapeA;
                } else {
                    externalBody = b2Shape_GetBody(shapeA);
                    internalShape = shapeB;
                }

                float collisionVelocity = data->manifold.points[0].normalVelocity;
                float collisionEnergy = 0.5 * b2Body_GetMass(externalBody) * collisionVelocity * collisionVelocity;

                if (collisionEnergy > material.energyCapacity && b2Shape_IsValid(internalShape)) {
                    DestroyShape(internalShape);
                }
            }
        }
    }
}

void DestructibleBody::Draw() {
    //Particles
    //Emitter_Update(destroyedParticles, GetFrameTime());

    // Update debris
    for (int i = 0; i < debrisInstances.size(); i++) {
        if (debrisInstances[i]->IsDestroyed()) {
            delete debrisInstances[i];
            debrisInstances[i] = nullptr;
            debrisInstances.erase(debrisInstances.begin() + i);
            i--;
        }
    }
    
    Color color = WHITE;

    // Batch draw debris with rlgl
    // Use rlgl for a single, optimized draw call for filled triangles
    rlBegin(RL_TRIANGLES);
    rlSetTexture(material.debrisTexture.id);
    rlColor4ub(color.r, color.g, color.b, color.a); // Set a single color for all triangles

    // Iterate through each shape attached to the body
    for (int i = 0; i < debrisInstances.size(); i++) {
        b2BodyId debrisBody = debrisInstances[i]->GetBodyId();
        float* vertexMagnitudes = debrisInstances[i]->GetVertexMagnitudes();
        float* vertexAngles = debrisInstances[i]->GetVertexAngles();

        b2Vec2 b2Position = b2Body_GetPosition(debrisBody);

        b2Rot rotationData = b2Body_GetTransform(debrisBody).q;
        float rotation = b2Rot_GetAngle(rotationData);

        // Get current orientation of vertices
        Vector2 vertexStates[3];

        // Transform each vertex to world coordinates
        for (int i = 0; i < 3; i++) {
            b2Vec2 vertexOffset;

            vertexOffset.x = vertexMagnitudes[i] * cos(vertexAngles[i] + rotation);
            vertexOffset.y = vertexMagnitudes[i] * sin(vertexAngles[i] + rotation);

            vertexStates[i] = (Vector2){(vertexOffset.x + b2Position.x) * PIXELS_PER_METER, (vertexOffset.y + b2Position.y) * PIXELS_PER_METER};
        }

        // Check for the winding order using the cross-product method
        float crossProduct = (vertexStates[1].x - vertexStates[0].x) * (vertexStates[2].y - vertexStates[0].y) -
                            (vertexStates[1].y - vertexStates[0].y) * (vertexStates[2].x - vertexStates[0].x);

        // If the cross product is negative, the winding order is clockwise, so we swap two vertices
        if (crossProduct > 0) {
            Vector2 temp = vertexStates[1];
            vertexStates[1] = vertexStates[2];
            vertexStates[2] = temp;
        }

        rlTexCoord2f(0.0f, 0.0f);
        rlVertex2f(vertexStates[0].x, vertexStates[0].y);

        rlTexCoord2f(0.5f, 1.0f);
        rlVertex2f(vertexStates[1].x, vertexStates[1].y);

        rlTexCoord2f(0.0f, 1.0f);
        rlVertex2f(vertexStates[2].x, vertexStates[2].y);
    }
    rlEnd();
    rlSetTexture(0);

    // Debris lines
    rlBegin(RL_LINES);
    rlColor4ub(BLACK.r, BLACK.g, BLACK.b, BLACK.a);

    // Iterate through each shape attached to the body
    for (int i = 0; i < debrisInstances.size(); i++) {
        b2BodyId debrisBody = debrisInstances[i]->GetBodyId();
        float* vertexMagnitudes = debrisInstances[i]->GetVertexMagnitudes();
        float* vertexAngles = debrisInstances[i]->GetVertexAngles();

        b2Vec2 b2Position = b2Body_GetPosition(debrisBody);

        b2Rot rotationData = b2Body_GetTransform(debrisBody).q;
        float rotation = b2Rot_GetAngle(rotationData);

        // Get current orientation of vertices
        Vector2 vertexStates[3];

        // Transform each vertex to world coordinates
        for (int i = 0; i < 3; i++) {
            b2Vec2 vertexOffset;

            vertexOffset.x = vertexMagnitudes[i] * cos(vertexAngles[i] + rotation);
            vertexOffset.y = vertexMagnitudes[i] * sin(vertexAngles[i] + rotation);

            vertexStates[i] = (Vector2){(vertexOffset.x + b2Position.x) * PIXELS_PER_METER, (vertexOffset.y + b2Position.y) * PIXELS_PER_METER};
        }

        rlVertex2f(vertexStates[0].x, vertexStates[0].y);
        rlVertex2f(vertexStates[1].x, vertexStates[1].y);

        rlVertex2f(vertexStates[1].x, vertexStates[1].y);
        rlVertex2f(vertexStates[2].x, vertexStates[2].y);

        rlVertex2f(vertexStates[2].x, vertexStates[2].y);
        rlVertex2f(vertexStates[0].x, vertexStates[0].y);
    }
    rlEnd();

    // Get the body's current world transform (position and rotation)
    b2Transform bodyTransform = b2Body_GetTransform(body);

    // Allocate memory to hold the b2ShapeIds.
    b2ShapeId* shapes = (b2ShapeId*)malloc(triangles * sizeof(b2ShapeId));

    if (shapes == nullptr) {
        std::cerr << "Error: Failed to allocate memory for shapeIds." << std::endl;
        return; // Exit if memory allocation fails
    }

    int actualTriangles = b2Body_GetShapes(body, shapes, triangles);
    
    // Use rlgl for a single, optimized draw call for filled triangles
    rlBegin(RL_TRIANGLES);
    rlSetTexture(material.texture.id);
    rlColor4ub(color.r, color.g, color.b, color.a); // Set a single color for all triangles

    // Iterate through each shape attached to the body
    for (int i = 0; i < actualTriangles; i++) {
        int textureIndex = (i + 1) % material.textureNumShapes;
        b2ShapeId shapeId = shapes[i];
        b2ShapeType type = b2Shape_GetType(shapeId);

        if (type == b2_polygonShape) {
            b2Polygon triangle = b2Shape_GetPolygon(shapeId);
            
            // Transform the three local vertices to world coordinates
            Vector2 raylibVertices[3];
            for (int v = 0; v < 3; v++) {
                b2Vec2 worldVertexB2 = b2TransformPoint(bodyTransform, triangle.vertices[v]);
                raylibVertices[v].x = worldVertexB2.x * PIXELS_PER_METER;
                raylibVertices[v].y = worldVertexB2.y * PIXELS_PER_METER;
            }

            // Check for the winding order using the cross-product method
            float crossProduct = (raylibVertices[1].x - raylibVertices[0].x) * (raylibVertices[2].y - raylibVertices[0].y) -
                                (raylibVertices[1].y - raylibVertices[0].y) * (raylibVertices[2].x - raylibVertices[0].x);

            // If the cross product is negative, the winding order is clockwise, so we swap two vertices
            if (crossProduct > 0) {
                Vector2 temp = raylibVertices[1];
                raylibVertices[1] = raylibVertices[2];
                raylibVertices[2] = temp;
            }

            // Now, draw the vertices with the guaranteed counter-clockwise winding order
                
            rlTexCoord2f(0.0f, 0.0f);
            rlVertex2f(raylibVertices[0].x, raylibVertices[0].y);

            rlTexCoord2f(0.5f, 1.0f);
            rlVertex2f(raylibVertices[1].x, raylibVertices[1].y);

            rlTexCoord2f(0.0f, 1.0f);
            rlVertex2f(raylibVertices[2].x, raylibVertices[2].y);
        }
    }
    rlEnd();
    rlSetTexture(0);

    // Body lines
    rlBegin(RL_LINES);
    rlColor4ub(BLACK.r, BLACK.g, BLACK.b, BLACK.a);

    // Iterate through each shape attached to the body
    for (int i = 0; i < actualTriangles; i++) {
        int textureIndex = (i + 1) % material.textureNumShapes;
        b2ShapeId shapeId = shapes[i];
        b2ShapeType type = b2Shape_GetType(shapeId);

        if (type == b2_polygonShape) {
            b2Polygon triangle = b2Shape_GetPolygon(shapeId);
            
            // Transform the three local vertices to world coordinates
            Vector2 raylibVertices[3];
            for (int v = 0; v < 3; v++) {
                b2Vec2 worldVertexB2 = b2TransformPoint(bodyTransform, triangle.vertices[v]);
                raylibVertices[v].x = worldVertexB2.x * PIXELS_PER_METER;
                raylibVertices[v].y = worldVertexB2.y * PIXELS_PER_METER;
            }

            // Check for the winding order using the cross-product method
            float crossProduct = (raylibVertices[1].x - raylibVertices[0].x) * (raylibVertices[2].y - raylibVertices[0].y) -
                                (raylibVertices[1].y - raylibVertices[0].y) * (raylibVertices[2].x - raylibVertices[0].x);

            // If the cross product is negative, the winding order is clockwise, so we swap two vertices
            if (crossProduct > 0) {
                Vector2 temp = raylibVertices[1];
                raylibVertices[1] = raylibVertices[2];
                raylibVertices[2] = temp;
            }

            // Now, draw the vertices with the guaranteed counter-clockwise winding order

            rlVertex2f(raylibVertices[0].x, raylibVertices[0].y);
            rlVertex2f(raylibVertices[1].x, raylibVertices[1].y);

            rlVertex2f(raylibVertices[1].x, raylibVertices[1].y);
            rlVertex2f(raylibVertices[2].x, raylibVertices[2].y);

            rlVertex2f(raylibVertices[2].x, raylibVertices[2].y);
            rlVertex2f(raylibVertices[0].x, raylibVertices[0].y);
        }
    }
    rlEnd();

    free(shapes);
}

void DestructibleBody::DestroyShape(b2ShapeId shape) {
    // Create debris in place of to-be-destroyed shape
    b2Vec2 debrisVertices[3];
    b2Polygon polygonShape = b2Shape_GetPolygon(shape);

    // Start emitting particles
    //destroyedParticles->config.origin = position;
    //Emitter_Burst(destroyedParticles);

    for (int j = 0; j < 3; j++) {
        debrisVertices[j] = {polygonShape.vertices[j].x - polygonShape.centroid.x, polygonShape.vertices[j].y - polygonShape.centroid.y};
    }

    b2Hull debrisHull = b2ComputeHull(debrisVertices, 3);
    b2Polygon debrisPolygon = b2MakePolygon(&debrisHull, COLLISION_MARGIN);

    float magnitude = sqrt(polygonShape.centroid.x * polygonShape.centroid.x + polygonShape.centroid.y * polygonShape.centroid.y);
    float debrisAngle = atan2(polygonShape.centroid.y, polygonShape.centroid.x) + rotation;

    b2Vec2 b2DebrisPosition = {position.x * METERS_PER_PIXEL + magnitude * cos(debrisAngle), position.y * METERS_PER_PIXEL + magnitude * sin(debrisAngle)};
    Vector2 debrisPosition = (Vector2){b2DebrisPosition.x * PIXELS_PER_METER, b2DebrisPosition.y * PIXELS_PER_METER};

    debrisInstances.push_back(new Debris(world, debrisPosition, rotation * (180.0f / PI), material.debrisEnergyCapacity, debrisPolygon));
    b2DestroyShape(shape, true);
}

// DEBRIS
Debris::Debris(b2WorldId worldId, Vector2 initialPosition, float initialRotation, float energyCapacity, b2Polygon polygonShape) 
: position(initialPosition), rotation(initialRotation), world(worldId), energyCapacity(energyCapacity) {
    // --- Body Creation ---
    b2BodyDef bodyDef = b2DefaultBodyDef();
    
    bodyDef.type = b2_dynamicBody;
    bodyDef.position = {initialPosition.x * METERS_PER_PIXEL, initialPosition.y * METERS_PER_PIXEL}; // Center of the overall box
    bodyDef.rotation = b2MakeRot(initialRotation * (PI / 180.0f));

    bodyDef.name = "Debris";
    bodyDef.userData = this;

    body = b2CreateBody(world, &bodyDef);

    // --- Shape Definition ---
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.material.friction = DEFAULT_FRICTION;
    shapeDef.material.restitution = DEFAULT_RESTITUTION;

    // --- Polygon Creation ---
    b2CreatePolygonShape(body, &shapeDef, &polygonShape);

    // --- Get Body Properties ---
    numVertices = polygonShape.count;

    for (int i = 0; i < numVertices; i++) {
        b2Vec2 b2Vertex = polygonShape.vertices[i];
        
        vertexMagnitudes[i] = sqrt(b2Vertex.x * b2Vertex.x + b2Vertex.y * b2Vertex.y);
        vertexAngles[i] = atan2(b2Vertex.y, b2Vertex.x);
    }

    // Make points counter-clockwise
    float temp = vertexMagnitudes[1];
    vertexMagnitudes[1] = vertexMagnitudes[2];
    vertexMagnitudes[2] = temp;

    temp = vertexAngles[1];
    vertexAngles[1] = vertexAngles[2];
    vertexAngles[2] = temp;
}

Debris::~Debris() {
    b2DestroyBody(body);
    body = b2BodyId{}; // Reset the ID
}

void Debris::Update() {
    b2Vec2 b2Position = b2Body_GetPosition(body);
    position = (Vector2){b2Position.x * PIXELS_PER_METER, b2Position.y * PIXELS_PER_METER};
    
    b2Rot rotationData = b2Body_GetTransform(body).q;
    rotation = b2Rot_GetAngle(rotationData);

    // Collision detection and material destruction
    int contactCapacity = b2Body_GetContactCapacity(body);

    b2ContactData* contactData = (b2ContactData*)malloc(contactCapacity * sizeof(b2ContactData));
    int numContacts = b2Body_GetContactData(body, contactData, contactCapacity);

    if (contactCapacity > 0) {
        for (int i = 0; i < numContacts; i++)
        {
            b2ContactData* data = contactData + i;

            // Identify which shape is external
            b2ShapeId shapeA = data->shapeIdA;
            b2ShapeId shapeB = data->shapeIdB;

            if (b2Shape_IsValid(shapeA) && b2Shape_IsValid(shapeB)) {
                b2BodyId externalBody;
                b2ShapeId internalShape;

                if (b2Shape_GetBody(shapeA).index1 == body.index1 && b2Shape_GetBody(shapeA).generation == body.generation) {
                    externalBody = b2Shape_GetBody(shapeB);
                    internalShape = shapeA;
                } else {
                    externalBody = b2Shape_GetBody(shapeA);
                    internalShape = shapeB;
                }
                
                float collisionVelocity = data->manifold.points[0].normalVelocity;
                float collisionEnergy = 0.5 * b2Body_GetMass(externalBody) * collisionVelocity * collisionVelocity;

                if (collisionEnergy > energyCapacity && b2Shape_IsValid(internalShape)) {
                    DestroyShape(internalShape);
                }
            }
        }
    }
}

void FreeDebris::Draw() {
    b2Vec2 b2Position = b2Body_GetPosition(body);

    b2Rot rotationData = b2Body_GetTransform(body).q;
    float rotation = b2Rot_GetAngle(rotationData);

    // Get current orientation of vertices
    Vector2 vertexStates[numVertices];

    // Transform each vertex to world coordinates
    for (int i = 0; i < numVertices; i++) {
        b2Vec2 vertexOffset;

        vertexOffset.x = vertexMagnitudes[i] * cos(vertexAngles[i] + rotation);
        vertexOffset.y = vertexMagnitudes[i] * sin(vertexAngles[i] + rotation);

        vertexStates[i] = (Vector2){(vertexOffset.x + b2Position.x) * PIXELS_PER_METER, (vertexOffset.y + b2Position.y) * PIXELS_PER_METER};
    }

    if (numVertices == 3) {
        // Draw the body
        DrawTriangle(vertexStates[0], vertexStates[1], vertexStates[2], BROWN);
        DrawTriangleLines(vertexStates[0], vertexStates[1], vertexStates[2], BLACK);

        //DrawRectangle(position.x - 10, position.y - 10, 20, 20, RED);
    } else if (numVertices == 4) {
        // Draw the body
        DrawRectanglePro((Rectangle){vertexStates[0].x, vertexStates[0].y, vertexStates[1].x - vertexStates[0].x, vertexStates[2].y - vertexStates[0].y}, (Vector2){0, 0}, 0.0f, RED);
        DrawRectangleLinesEx((Rectangle){vertexStates[0].x, vertexStates[0].y, vertexStates[1].x - vertexStates[0].x, vertexStates[2].y - vertexStates[0].y}, 4.0f, BLACK);
    }
}

void Debris::DestroyShape(b2ShapeId shape) {
    destroyed = true;
    b2DestroyShape(shape, false);
}

// SPRITE SETTINGS
SpriteSettings::SpriteSettings(Vector2 spriteOffset, std::string folderName, float spriteScale, float framesPerSecond, std::unordered_map<AnimationStates, int> frameCounts,
    std::unordered_map<AnimationStates, Shader> shaders) 
: offset(spriteOffset), scale(spriteScale), framesPerSecond(framesPerSecond), frameCounts(frameCounts), shaders(shaders) {
    totalFrames = std::accumulate(
        frameCounts.begin(), // Iterator to the beginning of the map
        frameCounts.end(),   // Iterator to the end of the map
        0,                   // Initial value of the sum
        [](int currentSum, const std::pair<const AnimationStates, int>& pair) {
            return currentSum + pair.second; // Add the value (pair.second) to the sum
        }
    );
    
    for (int i = 0; i < totalFrames; i++) {
        std::string fileName = "resources/sprites/characters/" + folderName + "/right/" + std::to_string(i) + ".png";
        Texture2D texture = LoadTexture(fileName.c_str());
        SetTextureFilter(texture, TEXTURE_FILTER_BILINEAR);
        sprites.push_back(texture);
    }

    for (int i = 0; i < totalFrames; i++) {
        std::string fileName = "resources/sprites/characters/" + folderName + "/left/" + std::to_string(i) + ".png";
        Texture2D texture = LoadTexture(fileName.c_str());
        SetTextureFilter(texture, TEXTURE_FILTER_BILINEAR);
        sprites.push_back(texture);
    }
}