#include "structures.h"
#include <iostream> // For potential debugging output (can remove in final build)
#include <cmath>    // For fabs (float absolute value)
#include <limits>   // For std::numeric_limits

// CHARACTER

Character::Character(b2WorldId worldId, float width, float height, float moveSpeed, float jumpForce, int maxHealth, int maxMedium, bool isPlayer)
    : mxHealth(maxHealth), mxMedium(maxMedium), health(maxHealth), medium(maxMedium), player(isPlayer), world(worldId), coyoteTimeCounter(0.0f) // Initialize worldRef and isGrounded
{
    moveSpeedPixelsPerSec = moveSpeed;
    jumpForcePixels = jumpForce;

    // 1. Define the Box2D Body Definition
    b2BodyDef bodyDef = b2DefaultBodyDef(); // Use default constructor for b2BodyDef
    bodyDef.type = b2_dynamicBody;

    // Convert initial position from pixels to Box2D meters (Box2D origin is center of body)
    // Raylib rect.x/y is top-left, Box2D body position is center.
    bodyDef.position = {(width / 2.0f) * METERS_PER_PIXEL, (height / 2.0f) * METERS_PER_PIXEL};

    bodyDef.linearDamping = 0.0f;
    bodyDef.angularDamping = 999999999999999.0f;

    // Create the body in the Box2D world using b2CreateBody
    body = b2CreateBody(world, &bodyDef);

    // 2. Define the Box2D Shape (Rectangle)
    float radius = height / 3.0f;
    b2Vec2 center1 = {0.0f, height * METERS_PER_PIXEL / 6.0f};
    b2Vec2 center2 = {0.0f, -height * METERS_PER_PIXEL / 6.0f};
    
    b2ShapeDef playerShapeDef = b2DefaultShapeDef();
    playerShapeDef.material.friction = BASE_FRICTION;
    b2Polygon playerBox = b2MakeBox(width * METERS_PER_PIXEL / 2.0f, height * METERS_PER_PIXEL / 2.0f);
    b2CreatePolygonShape(body, &playerShapeDef, &playerBox);

    // Initialize raylib rect and position for drawing.
    rect = (Rectangle){ 0, 0, width, height };
    position = (Vector2){0, 0};

    b2Vec2 center = {0.0f, 0.0f};
    float mass = 1.0f;
    float rotationalInertia = 0.0f;
    b2MassData massData = {mass, center, rotationalInertia};

    b2Body_SetMassData(body, massData);
}

// Destructor to clean up Box2D body
Character::~Character() {
    // Use b2DestroyBody to remove the body from the world
    b2DestroyBody(body);
    body = b2BodyId{}; // Reset the ID
}

// Update Character's position and rect from Box2D body
void Character::Update() {
    b2Body_SetAngularVelocity(body, 0.0f);

    // Get the position from the Box2D body (in meters) using b2Body_GetPosition
    b2Vec2 bodyPos = b2Body_GetPosition(body);

    // Convert to pixel coordinates for raylib drawing.
    position.x = bodyPos.x * PIXELS_PER_METER;
    position.y = bodyPos.y * PIXELS_PER_METER;

    rect.x = position.x - rect.width / 2.0f;
    rect.y = position.y - rect.height / 2.0f;

    // --- Coyote Time Logic ---
    bool currentlyGrounded = IsGrounded();

    if (currentlyGrounded) {
        coyoteTimeCounter = COYOTE_TIME; // Reset coyote time if grounded
    } else {
        coyoteTimeCounter -= GetFrameTime(); // Decrement if not grounded
    }

    // --- Input handling  ---

    // Horizontal Movement
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) {
        MoveLeft();
    } else if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) {
        MoveRight();
    } else {
        // Stop horizontal movement if no key is pressed
        b2Vec2 currentVel = b2Body_GetLinearVelocity(body);
        currentVel.x = 0.0f;
        b2Body_SetLinearVelocity(body, currentVel); // Use b2Body_SetLinearVelocity
    }

    // Jumping
    if ((IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) && (currentlyGrounded || coyoteTimeCounter > 0)) {
        Jump();
    }

    b2Body_SetAngularVelocity(body, 0.0f);
    b2Body_ApplyForceToCenter(body, {0, 30.0f}, false);
}

void Character::Draw() {
    DrawRectangleRec(rect, BLUE); // Example color
}

// These functions apply forces/impulses to the Box2D body
void Character::MoveLeft() {
    b2Vec2 vel = b2Body_GetLinearVelocity(body);
    vel.x = -moveSpeedPixelsPerSec * METERS_PER_PIXEL; // Target velocity in meters/sec
    b2Body_SetLinearVelocity(body, vel);
}

void Character::MoveRight() {
    b2Vec2 vel = b2Body_GetLinearVelocity(body);
    vel.x = moveSpeedPixelsPerSec * METERS_PER_PIXEL; // Target velocity in meters/sec
    b2Body_SetLinearVelocity(body, vel);
}

void Character::Jump() {
        // Apply an impulse upwards. Impulse is applied instantly.
        // Box2D v3 uses b2Body_ApplyLinearImpulse.
        // It takes the impulse vector and the point of application (center of mass is common).
        // The last argument is 'wake', set to true to wake the body if it's sleeping.
        b2Vec2 impulse = {0.0f, -jumpForcePixels * METERS_PER_PIXEL * b2Body_GetMass(body)};
        b2Body_ApplyLinearImpulse(body, impulse, {0, 0}, true);
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

// LEVEL

Level::Level(b2WorldId worldId, Character& playerToLoad) : world(worldId), player(playerToLoad) {};

void Level::LoadLevel(Vector2 spawnPoint) {
    player.position = spawnPoint; 
    b2Body_SetTransform(player.body, {spawnPoint.x * METERS_PER_PIXEL, spawnPoint.y * METERS_PER_PIXEL}, b2MakeRot(0.0f));
    destructibleBodies.push_back(std::unique_ptr<DestructibleBody>(new DestructibleBody(world, (Vector2){250.0f, 200.0f}, false, 0.0f, 500.0f, 100.0f, 999999999999.0f, 1.0f, 15.0f)));
    destructibleBodies.push_back(std::unique_ptr<DestructibleBody>(new DestructibleBody(world, (Vector2){250.0f, 0.0f}, true, 1.0f, 500.0f, 100.0f, 1.0f, 100.0f, 15.0f)));

    loaded = true;
}

void Level::UnloadLevel() {
    loaded = false;

    player.~Character();

    for (int i = 0; i < destructibleBodies.size(); i++) {
        destructibleBodies[i]->~DestructibleBody();
    }

    for (int i = 0; i < freeDebrisInstances.size(); i++) {
        freeDebrisInstances[i]->~Debris();
    }

    destructibleBodies.clear();
    freeDebrisInstances.clear();

    b2DestroyWorld(world);
}

void Level::Update() {
    if (loaded) {
        b2World_Step(world, dt, SUB_STEPS);
        player.Update();
        player.Draw();

        for (int i = 0; i < destructibleBodies.size(); i++) {
            destructibleBodies[i]->Update();
        }

        for (int i = 0; i < freeDebrisInstances.size(); i++) {
            freeDebrisInstances[i]->Update();
        }
    }
}

// DESTRUCTIBLE BODY

DestructibleBody::DestructibleBody(b2WorldId worldId, Vector2 initialPosition, float initialRotation, bool dynamic, float width, float height, float energyCapacity, float debrisEnergyCapacity, float squareSize)
    : world(worldId), triangles(0), energyCap(energyCapacity), debrisEnergyCap(debrisEnergyCapacity), position(initialPosition) // Initialize triangles to 0, will be calculated
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
}

void DestructibleBody::Update() {
    position = (Vector2){b2Body_GetPosition(body).x * PIXELS_PER_METER, b2Body_GetPosition(body).y * PIXELS_PER_METER};
    
    b2Rot rotationData = b2Body_GetTransform(body).q;
    float rotation = b2Rot_GetAngle(rotationData);

    // Update debris
    for (int i = 0; i < debrisInstances.size(); i++) {
        if (!debrisInstances[i]->destroyed) {
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

                if (collisionEnergy > energyCap && b2Shape_IsValid(internalShape)) {
                    // Create debris in place of to-be-destroyed shape
                    b2Vec2 debrisVertices[3];
                    b2Polygon polygonShape = b2Shape_GetPolygon(internalShape);

                    for (int j = 0; j < 3; j++) {
                        debrisVertices[j] = {polygonShape.vertices[j].x - polygonShape.centroid.x, polygonShape.vertices[j].y - polygonShape.centroid.y};
                    }

                    b2Hull debrisHull = b2ComputeHull(debrisVertices, 3);
                    b2Polygon debrisPolygon = b2MakePolygon(&debrisHull, COLLISION_MARGIN);

                    float magnitude = sqrt(polygonShape.centroid.x * polygonShape.centroid.x + polygonShape.centroid.y * polygonShape.centroid.y);
                    float debrisAngle = atan2(polygonShape.centroid.y, polygonShape.centroid.x) + rotation;

                    b2Vec2 b2DebrisPosition = {position.x * METERS_PER_PIXEL + magnitude * cos(debrisAngle), position.y * METERS_PER_PIXEL + magnitude * sin(debrisAngle)};
                    Vector2 debrisPosition = (Vector2){b2DebrisPosition.x * PIXELS_PER_METER, b2DebrisPosition.y * PIXELS_PER_METER};

                    debrisInstances.push_back(std::unique_ptr<Debris>(new Debris(world, debrisPosition, rotation * (180.0f / PI), debrisEnergyCap, debrisPolygon)));
                    b2DestroyShape(internalShape, true);
                }
            }
        }
    }

    // Drawing

    // Get the body's current world transform (position and rotation)
    b2Transform bodyTransform = b2Body_GetTransform(body);

    // Allocate memory to hold the b2ShapeIds.
    b2ShapeId* shapes = (b2ShapeId*)malloc(triangles * sizeof(b2ShapeId));

    if (shapes == nullptr) {
        std::cerr << "Error: Failed to allocate memory for shapeIds." << std::endl;
        return; // Exit if memory allocation fails
    }
        
    int actualTriangles = b2Body_GetShapes(body, shapes, triangles);

    // Iterate through each shape attached to the body
    for (int i = 0; i < actualTriangles; i++) {
        b2ShapeId shapeId = shapes[i];
        b2ShapeType type = b2Shape_GetType(shapeId);

        // Check if the shape is a polygon (which our triangles are)
        if (type == b2_polygonShape) {
            b2Polygon triangle = b2Shape_GetPolygon(shapeId);
            
            b2Vec2 worldVerticesB2[3];
            // Array to store the final vertices in Raylib pixel coordinates
            Vector2 raylibVertices[3];

            // Transform each local vertex of the triangle to world coordinates
            for (int v = 0; v < 3; ++v) {
                // Apply the body's transform (position + rotation) to the local vertex
                worldVerticesB2[v] = b2TransformPoint(bodyTransform, triangle.vertices[v]);

                // Convert the world coordinates from Box2D meters to Raylib pixels
                raylibVertices[v].x = worldVerticesB2[v].x * PIXELS_PER_METER;
                raylibVertices[v].y = worldVerticesB2[v].y * PIXELS_PER_METER;
            }

            // Make points counter-clockwise
            Vector2 temp = raylibVertices[1];
            raylibVertices[1] = raylibVertices[2];
            raylibVertices[2] = temp;
            
            DrawTriangle(raylibVertices[0], raylibVertices[1], raylibVertices[2], BROWN);
            DrawTriangleLines(raylibVertices[0], raylibVertices[1], raylibVertices[2], BLACK);
        }
    }

    free(shapes);
}

// DEBRIS
Debris::Debris(b2WorldId worldId, Vector2 initialPosition, float initialRotation, float energyCapacity, b2Polygon polygonShape) 
: position(initialPosition), world(worldId), energyCap(energyCapacity) {
    // --- Body Creation ---
    b2BodyDef bodyDef = b2DefaultBodyDef();
    
    bodyDef.type = b2_dynamicBody;
    bodyDef.position = {initialPosition.x * METERS_PER_PIXEL, initialPosition.y * METERS_PER_PIXEL}; // Center of the overall box
    bodyDef.rotation = b2MakeRot(initialRotation * (PI / 180.0f));

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
    float rotation = b2Rot_GetAngle(rotationData);

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

                if (collisionEnergy > energyCap && b2Shape_IsValid(internalShape)) {
                    destroyed = true;
                    b2DestroyShape(internalShape, false);
                }
            }
        }
    }
    
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