// Yes I know.. Litterally copied from the tech demo game 3D selector.c.
// Don't get annoyed. Reusing code is good some times.

#define TR_3D
#include "../../../tread.h"

// --- Model Definitions ---
typedef enum {
  CUBE = 0,
  PYRAMID,
  TETRAHEDRON,
  OCTAHEDRON,
  NUM_MODELS   // Must be last, counts the number of models
} ModelType;

// Current selected model
static int current_model_index = CUBE;

// Rotation for each model (independent rotation for tech demo effect)
static TR_Vector3 cubeRotation = {0.0f, 0.0f, 0.0f};
static TR_Vector3 pyramidRotation = {0.0f, 0.0f, 0.0f};
static TR_Vector3 tetraRotation = {0.0f, 0.0f, 0.0f};
static TR_Vector3 octaRotation = {0.0f, 0.0f, 0.0f};

// Rotation speeds for each model
static const float rotationSpeedX = 0.02f;
static const float rotationSpeedY = 0.03f;
static const float rotationSpeedZ = 0.01f;

// --- Pyramid Data (Vertices and Faces for a square-based pyramid) ---
// Vertices: 4 for base, 1 for apex
static const TR_Vector3 pyramid_vertices[] = {
  {-0.5f, -0.5f, -0.5f}, // 0: Base front-left
  { 0.5f, -0.5f, -0.5f}, // 1: Base front-right
  { 0.5f, -0.5f,  0.5f}, // 2: Base back-right
  {-0.5f, -0.5f,  0.5f}, // 3: Base back-left
  { 0.0f,  0.5f,  0.0f}  // 4: Apex
};
// Faces (triangles): 2 for base, 4 for sides
static const TR_Triangle pyramid_faces[] = {
  // Base (two triangles)
  {{0, 1, 2}}, {{0, 2, 3}},
  // Sides
  {{0, 1, 4}}, // Front face
  {{1, 2, 4}}, // Right face
  {{2, 3, 4}}, // Back face
  {{3, 0, 4}}  // Left face
};
static const int num_pyramid_faces = sizeof(pyramid_faces) / sizeof(TR_Triangle);


// --- Tetrahedron Data (4 vertices, 4 triangular faces) ---
// Simplified vertices for a regular tetrahedron, centered around origin
static const TR_Vector3 tetra_vertices[] = {
  { 0.0f,  0.5f,  0.0f},  // 0: Apex (top)
  {-0.5f, -0.5f,  0.5f},  // 1: Base front-left
  { 0.5f, -0.5f,  0.5f},  // 2: Base front-right
  { 0.0f, -0.5f, -0.5f}   // 3: Base back
};
// Faces (4 triangles)
static const TR_Triangle tetra_faces[] = {
  {{0, 1, 2}}, // Front face (V0, V1, V2)
  {{0, 2, 3}}, // Right-back face (V0, V2, V3)
  {{0, 3, 1}}, // Left-back face (V0, V3, V1)
  {{1, 3, 2}}  // Base face (V1, V3, V2) - order V1, V3, V2 for consistent winding if considering normal
};
static const int num_tetra_faces = sizeof(tetra_faces) / sizeof(TR_Triangle);


// --- Octahedron Data (Approximation of a Sphere) ---
// Vertices: North pole, East, North, West, South, South pole
static const TR_Vector3 octa_vertices[] = {
  { 0.0f,  0.0f,  1.0f}, // 0: North Pole
  { 1.0f,  0.0f,  0.0f}, // 1: East
  { 0.0f,  1.0f,  0.0f}, // 2: North (positive Y)
  {-1.0f,  0.0f,  0.0f}, // 3: West
  { 0.0f, -1.0f,  0.0f}, // 4: South (negative Y)
  { 0.0f,  0.0f, -1.0f}  // 5: South Pole
};
// Faces (8 triangles for an octahedron)
static const TR_Triangle octa_faces[] = {
  // Top 4 faces (connecting to North Pole)
  {{0, 1, 2}}, // Pole-East-North
  {{0, 2, 3}}, // Pole-North-West
  {{0, 3, 4}}, // Pole-West-South
  {{0, 4, 1}}, // Pole-South-East

  // Bottom 4 faces (connecting to South Pole) - careful with winding order
  {{5, 2, 1}}, // SouthPole-North-East
  {{5, 3, 2}}, // SouthPole-West-North
  {{5, 4, 3}}, // SouthPole-South-West
  {{5, 1, 4}}  // SouthPole-East-South
};
static const int num_octa_faces = sizeof(octa_faces) / sizeof(TR_Triangle);

void run_lib_app() {
  const int screenWidth = 80;
  const int screenHeight = 25;
  TR_InitWindow(screenWidth, screenHeight, "tread.h - 3D Character Selector");
  TR_SetTargetFPS(60);

  // Common position and size for all models
  TR_Vector3 modelPosition = {0.0f, 0.0f, 0.0f};
  TR_Vector3 modelSize = {2.0f, 2.0f, 2.0f}; // Scale for visibility

  while (!TR_WindowShouldClose()) {
    // --- Update ---
    // Update rotation for all models (even if not displayed, so they're ready when selected)
    cubeRotation.x = fmodf(cubeRotation.x + rotationSpeedX, 2.0f * (float)M_PI);
    cubeRotation.y = fmodf(cubeRotation.y + rotationSpeedY, 2.0f * (float)M_PI);
    cubeRotation.z = fmodf(cubeRotation.z + rotationSpeedZ, 2.0f * (float)M_PI);

    pyramidRotation.x = fmodf(pyramidRotation.x + rotationSpeedY, 2.0f * (float)M_PI); // Different speed for pyramid
    pyramidRotation.y = fmodf(pyramidRotation.y + rotationSpeedZ, 2.0f * (float)M_PI);
    pyramidRotation.z = fmodf(pyramidRotation.z + rotationSpeedX, 2.0f * (float)M_PI);

    tetraRotation.x = fmodf(tetraRotation.x + rotationSpeedX, 2.0f * (float)M_PI);
    tetraRotation.y = fmodf(tetraRotation.y + rotationSpeedZ, 2.0f * (float)M_PI);
    tetraRotation.z = fmodf(tetraRotation.z + rotationSpeedY, 2.0f * (float)M_PI);

    octaRotation.x = fmodf(octaRotation.x + rotationSpeedZ, 2.0f * (float)M_PI);
    octaRotation.y = fmodf(octaRotation.y + rotationSpeedX, 2.0f * (float)M_PI);
    octaRotation.z = fmodf(octaRotation.z + rotationSpeedY, 2.0f * (float)M_PI);

    // Handle model selection input
    int key = TR_GetKeyPressed();
    if (key != 0) {
      if (key == 'a' || key == TR_KEY_LEFT) {
        current_model_index--;
        if (current_model_index < 0) current_model_index = NUM_MODELS - 1;
      } else if (key == 'd' || key == TR_KEY_RIGHT) {
        current_model_index++;
        if (current_model_index >= NUM_MODELS) current_model_index = 0;
      } else if (key == 'q' || key == 27) { // Q or ESC to quit
        TR_CloseWindow(); // Signal to main loop to exit
      }
    }

    // --- Draw ---
    TR_BeginDrawing();
    TR_ClearBackground(DARKBLUE); // Clear with a dark background

    // Define a common View-Projection matrix for all models
    TR_Matrix4x4 view = TR_MatrixTranslate(0.0f, 0.0f, -5.0f); // Move camera back
    float aspect_ratio = (float)TR_GetScreenWidth() / TR_GetScreenHeight();
    aspect_ratio *= 0.5f; // Compensate for character aspect ratio
    TR_Matrix4x4 projection = TR_MatrixPerspective(45.0f * (float)M_PI / 180.0f, aspect_ratio, 0.1f, 100.0f);


    // Draw the selected model
    switch (current_model_index) {
      case CUBE: {
        TR_DrawCubeWireframe3D(modelPosition, modelSize, cubeRotation, YELLOW);
        TR_DrawText("Model: Cube", 1, 1, 10, RAYWHITE, BLACK);
        break;
      }
      case PYRAMID: {
        TR_Matrix4x4 model = TR_MatrixIdentity();
        model = TR_MatrixMultiply(model, TR_MatrixScale(modelSize.x, modelSize.y, modelSize.z));
        model = TR_MatrixMultiply(model, TR_MatrixRotateX(pyramidRotation.x));
        model = TR_MatrixMultiply(model, TR_MatrixRotateY(pyramidRotation.y));
        model = TR_MatrixMultiply(model, TR_MatrixRotateZ(pyramidRotation.z));
        model = TR_MatrixMultiply(model, TR_MatrixTranslate(modelPosition.x, modelPosition.y, modelPosition.z));
        TR_Matrix4x4 mvp_matrix = TR_MatrixMultiply(TR_MatrixMultiply(model, view), projection);

        for (int i = 0; i < num_pyramid_faces; ++i) {
          TR_Triangle face = pyramid_faces[i];
          TR_DrawTriangle3DWireframe(
            pyramid_vertices[face.v[0]],
            pyramid_vertices[face.v[1]],
            pyramid_vertices[face.v[2]],
            mvp_matrix,
            GREEN
          );
        }
        TR_DrawText("Model: Pyramid", 1, 1, 10, RAYWHITE, BLACK);
        break;
      }
      case TETRAHEDRON: {
        TR_Matrix4x4 model = TR_MatrixIdentity();
        model = TR_MatrixMultiply(model, TR_MatrixScale(modelSize.x, modelSize.y, modelSize.z));
        model = TR_MatrixMultiply(model, TR_MatrixRotateX(tetraRotation.x));
        model = TR_MatrixMultiply(model, TR_MatrixRotateY(tetraRotation.y));
        model = TR_MatrixMultiply(model, TR_MatrixRotateZ(tetraRotation.z));
        model = TR_MatrixMultiply(model, TR_MatrixTranslate(modelPosition.x, modelPosition.y, modelPosition.z));
        TR_Matrix4x4 mvp_matrix = TR_MatrixMultiply(TR_MatrixMultiply(model, view), projection);

        for (int i = 0; i < num_tetra_faces; ++i) {
          TR_Triangle face = tetra_faces[i];
          TR_DrawTriangle3DWireframe(
            tetra_vertices[face.v[0]],
            tetra_vertices[face.v[1]],
            tetra_vertices[face.v[2]],
            mvp_matrix,
            MAGENTA
          );
        }
        TR_DrawText("Model: Tetrahedron", 1, 1, 10, RAYWHITE, BLACK);
        break;
      }
      case OCTAHEDRON: {
        TR_Matrix4x4 model = TR_MatrixIdentity();
        model = TR_MatrixMultiply(model, TR_MatrixScale(modelSize.x, modelSize.y, modelSize.z));
        model = TR_MatrixMultiply(model, TR_MatrixRotateX(octaRotation.x));
        model = TR_MatrixMultiply(model, TR_MatrixRotateY(octaRotation.y));
        model = TR_MatrixMultiply(model, TR_MatrixRotateZ(octaRotation.z));
        model = TR_MatrixMultiply(model, TR_MatrixTranslate(modelPosition.x, modelPosition.y, modelPosition.z));
        TR_Matrix4x4 mvp_matrix = TR_MatrixMultiply(TR_MatrixMultiply(model, view), projection);

        for (int i = 0; i < num_octa_faces; ++i) {
          TR_Triangle face = octa_faces[i];
          TR_DrawTriangle3DWireframe(
            octa_vertices[face.v[0]],
            octa_vertices[face.v[1]],
            octa_vertices[face.v[2]],
            mvp_matrix,
            CYAN
          );
        }
        TR_DrawText("Model: Octahedron", 1, 1, 10, RAYWHITE, BLACK);
        break;
      }
    }

    TR_DrawText("Use A/D or Left/Right Arrows to change model", 1, 3, 10, LIGHTGRAY, BLACK);
    TR_DrawText("ESC/Q to quit", 1, 4, 10, LIGHTGRAY, BLACK);

    TR_EndDrawing();
  }

  TR_CloseWindow();
}
