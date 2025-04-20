// 3D Tic-Tac-Toe with Minimax AI and Raylib Visualization

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h> // For INT_MIN, INT_MAX
#include <ctype.h>  // For toupper
#include <time.h>   // For srand
#include <float.h>  // For FLT_MAX
#include "include/raylib.h" // Include Raylib header
#include "include/raymath.h" // Include Raymath header for 3D math
#include "include/rlgl.h"    // Include Raylib GL header for low-level matrix transformations
//#include "include/camera.h"  // Explicitly include camera header

#define SIZE 3
#define EMPTY ' '
#define WIN_SCORE 100
#define LOSS_SCORE -100
#define DRAW_SCORE 0

// --- Raylib Specific Defines ---
#define SCREEN_WIDTH 1060 // Increased width for better spacing
#define SCREEN_HEIGHT 740 // Increased height slightly
#define CELL_SIZE_3D 2.0f // Size of each cube cell in 3D space
#define GRID_SPACING_3D 0.5f // Spacing between cubes
#define MARKER_RADIUS (CELL_SIZE_3D * 0.35f) // Radius for O spheres
#define MARKER_CUBE_SIZE (CELL_SIZE_3D * 0.7f) // Size for X cubes

// --- Layout Defines ---
#define LAYER_DISPLAY_AREA_WIDTH 200 // Width allocated for each layer's 3D view
#define LAYER_DISPLAY_SPACING 50   // Space between layer views
#define TOTAL_DISPLAY_WIDTH (SIZE * LAYER_DISPLAY_AREA_WIDTH + (SIZE - 1) * LAYER_DISPLAY_SPACING)
#define DISPLAY_START_X ((SCREEN_WIDTH - TOTAL_DISPLAY_WIDTH) / 2.0f)
#define DISPLAY_START_Y 100.0f

#define BACKGROUND_COLOR CLITERAL(Color){ 25, 25, 35, 255 } // Dark blue-gray
#define GRID_COLOR CLITERAL(Color){ 80, 120, 200, 255 }    // Blue grid lines
#define HOVER_COLOR CLITERAL(Color){ 255, 255, 100, 100 }  // Yellow glow
#define X_COLOR CLITERAL(Color){ 45, 150, 240, 255 }       // Bright blue
#define O_COLOR CLITERAL(Color){ 240, 90, 90, 255 }        // Bright red
#define WIN_COLOR CLITERAL(Color){ 50, 200, 100, 255 }     // Bright green

// Add to global variables
float timeCounter = 0.0f; // For animations
Model xModel, oModel;     // 3D models for markers

// Global variables for player symbols
char USER_SYMBOL = 'X'; // Default, can be changed
char AI_SYMBOL = 'O';

// Variables to store winning line coordinates
Vector3 winningLineStart = {0}, winningLineMid = {0}, winningLineEnd = {0};
bool drawWinningLine = false;

// Game State Enum
typedef enum {
    SELECT_SYMBOL,
    SELECT_DIFFICULTY,
    PLAYER_TURN,
    AI_TURN,
    AI_THINKING,
    GAME_OVER
} GameScreen;

// Function declarations (Forward Declarations)
void InitializeBoard(char board[SIZE][SIZE][SIZE]);
bool IsValidMove(char board[SIZE][SIZE][SIZE], int layer, int row, int col);
bool IsBoardFull(char board[SIZE][SIZE][SIZE]);
char CheckWinner(char board[SIZE][SIZE][SIZE]);
int EvaluateBoard(char board[SIZE][SIZE][SIZE]);
int Minimax(char board[SIZE][SIZE][SIZE], int depth, bool isMaximizing, int maxDepth);
void GetAIMove(char board[SIZE][SIZE][SIZE], int ply, int *moveCount, int *bestL, int *bestR, int *bestC); // Modified to return move
void DrawBoard3D(char board[SIZE][SIZE][SIZE], Camera camera, int hoverL, int hoverR, int hoverC); // Added hover parameters
void DrawUI(GameScreen currentScreen, char winner, Font font, int difficulty, char selectedSymbol);
Vector3 GetCellCenter(int layer, int row, int col); // Helper to get 3D center of a cell

// Load 3D models for X and O
void LoadModels() {
    // X model (two intersecting cylinders)
    Mesh xMesh = GenMeshCylinder(0.1f, MARKER_CUBE_SIZE*1.4f, 8);
    xModel = LoadModelFromMesh(xMesh);
    xModel.transform = MatrixRotateZ(45*DEG2RAD);
    
    // O model (torus)
    Mesh oMesh = GenMeshTorus(MARKER_RADIUS*0.7f, MARKER_RADIUS*0.3f, 16, 16);
    oModel = LoadModelFromMesh(oMesh);
}

// Unload models
void UnloadModels() {
    UnloadModel(xModel);
    UnloadModel(oModel);
}

// --- Main Game Function ---
int main(void) {
    // Initialization
    //--------------------------------------------------------------------------------------
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "3D Tic-Tac-Toe - Raylib (3D View)");
    SetTargetFPS(60);
    srand(time(NULL)); // Seed random number generator for AI move randomization

    char board[SIZE][SIZE][SIZE];
    InitializeBoard(board);

    int moveCount = 0;
    char winner = EMPTY;
    bool userStarts = true;
    int difficulty = 2;
    GameScreen currentScreen = SELECT_SYMBOL;

    Font font = GetFontDefault();

    int aiBestL = -1, aiBestR = -1, aiBestC = -1;

    // Define the camera to look at the scene
    Camera3D camera = { 0 };
    // Store the default camera state for reset
    Vector3 defaultCameraPosition = { 8.0f, 8.0f, 8.0f }; // Closer to the center
    Vector3 defaultCameraTarget = { 0.0f, 0.0f, 0.0f };   // Target the absolute center

    camera.position = defaultCameraPosition;
    camera.target = defaultCameraTarget;
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    // In main(), add after InitWindow:
    LoadModels();
    //SetCameraMode(camera, CAMERA_CUSTOM); // For manual control

    // Variables for mouse picking
    int hoverLayer = -1, hoverRow = -1, hoverCol = -1;

    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose()) {
        // Update
        //----------------------------------------------------------------------------------

        // --- Remove Debug Mouse Buttons ---
        // if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) printf("Debug: Left Mouse Down\n");
        // if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) printf("Debug: Middle Mouse Down\n");
        // --- End Debug ---

        // UpdateCamera(&camera, CAMERA_ORBITAL); // Use orbital camera controls

        // --- Manual Orbital Camera Update ---
        Vector2 mouseDelta = GetMouseDelta();
        float zoomInput = GetMouseWheelMove();
        float rotateSpeed = 0.005f; // Sensitivity for rotation
        float panSpeed = 0.08f;    // Sensitivity for panning
        float zoomSpeed = 1.2f;     // Sensitivity for zoom

        // Rotation (Left Mouse Button)
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            // Calculate rotation angles based on mouse delta
            float yawAngle = -mouseDelta.x * rotateSpeed;
            float pitchAngle = -mouseDelta.y * rotateSpeed;

            // Get the vector from target to position
            Vector3 targetToPos = Vector3Subtract(camera.position, camera.target);

            // Rotate around the global UP axis (Y) for yaw
            targetToPos = Vector3RotateByAxisAngle(targetToPos, camera.up, yawAngle);

            // Calculate the camera's right vector (perpendicular to view direction and up)
            Vector3 right = Vector3Normalize(Vector3CrossProduct(Vector3Subtract(camera.target, camera.position), camera.up));
            // Ensure right vector is valid (avoid issues when looking straight up/down)
            if (fabsf(Vector3DotProduct(right, right)) < 0.001f) {
                 right = Vector3Normalize(Vector3CrossProduct((Vector3){0.0f, 0.0f, 1.0f}, camera.up)); // Use Z if view is aligned with up
                 if (fabsf(Vector3DotProduct(right, right)) < 0.001f) {
                     right = (Vector3){1.0f, 0.0f, 0.0f}; // Use X as last resort
                 }
            }

            // Rotate around the camera's right axis for pitch
            targetToPos = Vector3RotateByAxisAngle(targetToPos, right, pitchAngle);

            // Calculate the new camera position
            camera.position = Vector3Add(camera.target, targetToPos);

            // Optional: Prevent camera from flipping over the top/bottom (pitch lock)
            // This requires checking the angle between the new forward vector and the up vector
            // For simplicity, we'll omit the pitch lock for now, but it can be added if needed.
        }

        // Panning (Middle Mouse Button)
        if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
             // Calculate pan movement based on camera orientation and mouse delta
             Vector3 right = Vector3Normalize(Vector3CrossProduct(Vector3Subtract(camera.target, camera.position), camera.up));
             Vector3 upActual = Vector3Normalize(Vector3CrossProduct(right, Vector3Subtract(camera.target, camera.position))); // Use calculated up for panning
             Vector3 panMove = Vector3Add(Vector3Scale(right, -mouseDelta.x * panSpeed), Vector3Scale(upActual, mouseDelta.y * panSpeed));

             // Apply pan by moving both position and target
             camera.position = Vector3Add(camera.position, panMove);
             camera.target = Vector3Add(camera.target, panMove);
        }

        // Zoom (Mouse Wheel)
        if (zoomInput != 0) {
             // Move camera position towards/away from target
             Vector3 targetToPos = Vector3Subtract(camera.position, camera.target);
             float distance = Vector3Length(targetToPos);
             float newDistance = distance - zoomInput * zoomSpeed;
             if (newDistance < 1.0f) newDistance = 1.0f; // Prevent zooming too close
             camera.position = Vector3Add(camera.target, Vector3Scale(Vector3Normalize(targetToPos), newDistance));
        }
        // --- End Manual Update ---


        // Add camera reset functionality
        if (IsKeyPressed(KEY_C)) {
            camera.position = defaultCameraPosition;
            camera.target = defaultCameraTarget;
        }

        // --- Restore Game Logic --- 
        switch (currentScreen) {
            case SELECT_SYMBOL:
                if (IsKeyPressed(KEY_X)) {
                    USER_SYMBOL = 'X'; AI_SYMBOL = 'O'; userStarts = true; currentScreen = SELECT_DIFFICULTY;
                } else if (IsKeyPressed(KEY_O)) {
                    USER_SYMBOL = 'O'; AI_SYMBOL = 'X'; userStarts = false; currentScreen = SELECT_DIFFICULTY;
                }
                break;

            case SELECT_DIFFICULTY:
                if (IsKeyPressed(KEY_ONE) || IsKeyPressed(KEY_KP_1)) { difficulty = 1; currentScreen = userStarts ? PLAYER_TURN : AI_TURN; }
                if (IsKeyPressed(KEY_TWO) || IsKeyPressed(KEY_KP_2)) { difficulty = 2; currentScreen = userStarts ? PLAYER_TURN : AI_TURN; }
                if (IsKeyPressed(KEY_THREE) || IsKeyPressed(KEY_KP_3)) { difficulty = 3; currentScreen = userStarts ? PLAYER_TURN : AI_TURN; }
                if (IsKeyPressed(KEY_FOUR) || IsKeyPressed(KEY_KP_4)) { difficulty = 4; currentScreen = userStarts ? PLAYER_TURN : AI_TURN; }
                break;

            case PLAYER_TURN:
                {
                    // Reset hover state
                    hoverLayer = -1; hoverRow = -1; hoverCol = -1;
                    Ray mouseRay = GetMouseRay(GetMousePosition(), camera);
                    float closestHitDist = FLT_MAX;

                    for (int l = 0; l < SIZE; l++) {
                        for (int r = 0; r < SIZE; r++) {
                            for (int c = 0; c < SIZE; c++) {
                                if (board[l][r][c] == EMPTY) { // Only check empty cells
                                    Vector3 cellCenter = GetCellCenter(l, r, c);
                                    // Bounding box for picking
                                    BoundingBox cellBox = {
                                        (Vector3){ cellCenter.x - CELL_SIZE_3D / 2.0f, cellCenter.y - CELL_SIZE_3D / 2.0f, cellCenter.z - CELL_SIZE_3D / 2.0f },
                                        (Vector3){ cellCenter.x + CELL_SIZE_3D / 2.0f, cellCenter.y + CELL_SIZE_3D / 2.0f, cellCenter.z + CELL_SIZE_3D / 2.0f }
                                    };

                                    RayCollision collision = GetRayCollisionBox(mouseRay, cellBox);

                                    if (collision.hit && collision.distance < closestHitDist) {
                                        closestHitDist = collision.distance;
                                        hoverLayer = l;
                                        hoverRow = r;
                                        hoverCol = c;
                                    }
                                }
                            }
                        }
                    }

                    // Check for click on hovered cell
                    if (hoverLayer != -1 && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        if (IsValidMove(board, hoverLayer, hoverRow, hoverCol)) {
                            board[hoverLayer][hoverRow][hoverCol] = USER_SYMBOL;
                            moveCount++;
                            winner = CheckWinner(board);
                            // DEBUG: Print winning line start X if win detected
                            //if (drawWinningLine) {
                            //    printf("DEBUG: Win detected! winningLineStart.x = %f\n", winningLineStart.x);
                            //}
                            if (winner != EMPTY || IsBoardFull(board)) {
                                currentScreen = GAME_OVER;
                            } else {
                                currentScreen = AI_TURN;
                            }
                        }
                    }
                }
                break;

            case AI_TURN:
                currentScreen = AI_THINKING;
                aiBestL = -1; aiBestR = -1; aiBestC = -1;
                GetAIMove(board, difficulty, &moveCount, &aiBestL, &aiBestR, &aiBestC);
                if (aiBestL != -1 && IsValidMove(board, aiBestL, aiBestR, aiBestC)) {
                     board[aiBestL][aiBestR][aiBestC] = AI_SYMBOL;
                     moveCount++;
                     winner = CheckWinner(board);
                     if (winner != EMPTY || IsBoardFull(board)) {
                         currentScreen = GAME_OVER;
                     } else {
                         currentScreen = PLAYER_TURN;
                     }
                } else {
                     printf("AI Error: Could not find a valid move!\n");
                     currentScreen = GAME_OVER;
                }
                break;

             case AI_THINKING:
                 // Brief state, transitions immediately after GetAIMove
                 break;

            case GAME_OVER:
                if (IsKeyPressed(KEY_R)) {
                    InitializeBoard(board); 
                    moveCount = 0;
                    winner = EMPTY;
                    drawWinningLine = false;
                    currentScreen = SELECT_SYMBOL; 
                }
                break;
        }
        // --- END Restore Game Logic ---

        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode3D(camera); // Start 3D drawing
            DrawBoard3D(board, camera, hoverLayer, hoverRow, hoverCol); // Pass hover state
        EndMode3D(); // End 3D drawing

        // Draw 2D UI elements on top
        DrawUI(currentScreen, winner, font, difficulty, USER_SYMBOL);
        DrawFPS(SCREEN_WIDTH - 90, 10); // Show FPS

        EndDrawing();
        //----------------------------------------------------------------------------------
    }
    UnloadModels();
    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow();
    //--------------------------------------------------------------------------------------

    return 0;
}

// --- Helper Functions ---

// Scores a single line based on player symbols and empty spaces
int ScoreLine(char c1, char c2, char c3, char player) {
    int playerCount = 0;
    int emptyCount = 0;

    if (c1 == player) playerCount++; else if (c1 == EMPTY) emptyCount++;
    if (c2 == player) playerCount++; else if (c2 == EMPTY) emptyCount++;
    if (c3 == player) playerCount++; else if (c3 == EMPTY) emptyCount++;

    // Check if the opponent has any symbols in the line
    char opponent = (player == USER_SYMBOL) ? AI_SYMBOL : USER_SYMBOL;
    if ((c1 == opponent) || (c2 == opponent) || (c3 == opponent)) {
        return 0; // Line is blocked by opponent, no potential value for 'player'
    }


    if (playerCount == 2 && emptyCount == 1) {
        return 10; // High value for imminent win/block opportunity
    } else if (playerCount == 1 && emptyCount == 2) {
        return 1;  // Low value for starting a line
    } else {
        return 0;  // No potential for this player in this line
    }
}

// Calculates the total heuristic score for a player on the board
int CalculateTotalHeuristic(char board[SIZE][SIZE][SIZE], char player) {
    int totalScore = 0;

    // Check lines within each XY plane (fixed layer i)
    for (int i = 0; i < SIZE; i++) {
        // Rows
        totalScore += ScoreLine(board[i][0][0], board[i][0][1], board[i][0][2], player);
        totalScore += ScoreLine(board[i][1][0], board[i][1][1], board[i][1][2], player);
        totalScore += ScoreLine(board[i][2][0], board[i][2][1], board[i][2][2], player);
        // Columns
        totalScore += ScoreLine(board[i][0][0], board[i][1][0], board[i][2][0], player);
        totalScore += ScoreLine(board[i][0][1], board[i][1][1], board[i][2][1], player);
        totalScore += ScoreLine(board[i][0][2], board[i][1][2], board[i][2][2], player);
        // Diagonals
        totalScore += ScoreLine(board[i][0][0], board[i][1][1], board[i][2][2], player);
        totalScore += ScoreLine(board[i][0][2], board[i][1][1], board[i][2][0], player);
    }

    // Check lines within each XZ plane (fixed row j) - Diagonals only
    for (int j = 0; j < SIZE; j++) {
        totalScore += ScoreLine(board[0][j][0], board[1][j][1], board[2][j][2], player);
        totalScore += ScoreLine(board[0][j][2], board[1][j][1], board[2][j][0], player);
    }

    // Check lines within each YZ plane (fixed column k) - Diagonals only
     for (int k = 0; k < SIZE; k++) {
        totalScore += ScoreLine(board[0][0][k], board[1][1][k], board[2][2][k], player);
        totalScore += ScoreLine(board[0][2][k], board[1][1][k], board[2][0][k], player);
    }

    // Check columns across layers (vertical lines)
    for (int j = 0; j < SIZE; j++) {
        for (int k = 0; k < SIZE; k++) {
            totalScore += ScoreLine(board[0][j][k], board[1][j][k], board[2][j][k], player);
        }
    }

    // Check 3D diagonals
    totalScore += ScoreLine(board[0][0][0], board[1][1][1], board[2][2][2], player);
    totalScore += ScoreLine(board[0][0][2], board[1][1][1], board[2][2][0], player);
    totalScore += ScoreLine(board[0][2][0], board[1][1][1], board[2][0][2], player);
    totalScore += ScoreLine(board[0][2][2], board[1][1][1], board[2][0][0], player);

    return totalScore;
}

// Calculate the 3D center position of a cell
Vector3 GetCellCenter(int layer, int row, int col) {
    float totalSize = SIZE * CELL_SIZE_3D + (SIZE - 1) * GRID_SPACING_3D;
    float offset = totalSize / 2.0f - CELL_SIZE_3D / 2.0f;

    float x = (col * (CELL_SIZE_3D + GRID_SPACING_3D)) - offset;
    float y = (row * (CELL_SIZE_3D + GRID_SPACING_3D)) - offset;
    float z = (layer * (CELL_SIZE_3D + GRID_SPACING_3D)) - offset;

    return (Vector3){ x, y, z };
}

void InitializeBoard(char board[SIZE][SIZE][SIZE]) {
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            for (int k = 0; k < SIZE; k++) {
                board[i][j][k] = EMPTY;
            }
        }
    }
}

bool IsValidMove(char board[SIZE][SIZE][SIZE], int layer, int row, int col) {
    return layer >= 0 && layer < SIZE &&
           row >= 0 && row < SIZE &&
           col >= 0 && col < SIZE &&
           board[layer][row][col] == EMPTY;
}

bool IsBoardFull(char board[SIZE][SIZE][SIZE]) {
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            for (int k = 0; k < SIZE; k++) {
                if (board[i][j][k] == EMPTY) {
                    return false;
                }
            }
        }
    }
    return true;
}

char CheckWinner(char board[SIZE][SIZE][SIZE]) {
    char players[] = {USER_SYMBOL, AI_SYMBOL};
    drawWinningLine = false; // Reset before checking

    for (int p = 0; p < 2; p++) {
        char player = players[p];

        // Check lines within each XY plane (fixed layer i)
        for (int i = 0; i < SIZE; i++) { 
            // Check Rows (across columns k)
            if (board[i][0][0] == player && board[i][0][1] == player && board[i][0][2] == player) { winningLineStart=GetCellCenter(i,0,0); winningLineMid=GetCellCenter(i,0,1); winningLineEnd=GetCellCenter(i,0,2); drawWinningLine=true; return player; }
            if (board[i][1][0] == player && board[i][1][1] == player && board[i][1][2] == player) { winningLineStart=GetCellCenter(i,1,0); winningLineMid=GetCellCenter(i,1,1); winningLineEnd=GetCellCenter(i,1,2); drawWinningLine=true; return player; }
            if (board[i][2][0] == player && board[i][2][1] == player && board[i][2][2] == player) { winningLineStart=GetCellCenter(i,2,0); winningLineMid=GetCellCenter(i,2,1); winningLineEnd=GetCellCenter(i,2,2); drawWinningLine=true; return player; }
            // Check Columns (across rows j)
            if (board[i][0][0] == player && board[i][1][0] == player && board[i][2][0] == player) { winningLineStart=GetCellCenter(i,0,0); winningLineMid=GetCellCenter(i,1,0); winningLineEnd=GetCellCenter(i,2,0); drawWinningLine=true; return player; }
            if (board[i][0][1] == player && board[i][1][1] == player && board[i][2][1] == player) { winningLineStart=GetCellCenter(i,0,1); winningLineMid=GetCellCenter(i,1,1); winningLineEnd=GetCellCenter(i,2,1); drawWinningLine=true; return player; }
            if (board[i][0][2] == player && board[i][1][2] == player && board[i][2][2] == player) { winningLineStart=GetCellCenter(i,0,2); winningLineMid=GetCellCenter(i,1,2); winningLineEnd=GetCellCenter(i,2,2); drawWinningLine=true; return player; }
            // Check Diagonals
            if (board[i][0][0] == player && board[i][1][1] == player && board[i][2][2] == player) { winningLineStart=GetCellCenter(i,0,0); winningLineMid=GetCellCenter(i,1,1); winningLineEnd=GetCellCenter(i,2,2); drawWinningLine=true; return player; } // Main diag
            if (board[i][0][2] == player && board[i][1][1] == player && board[i][2][0] == player) { winningLineStart=GetCellCenter(i,0,2); winningLineMid=GetCellCenter(i,1,1); winningLineEnd=GetCellCenter(i,2,0); drawWinningLine=true; return player; } // Anti diag
        }

        // Check lines within each XZ plane (fixed row j)
        for (int j = 0; j < SIZE; j++) {
             // Check Diagonals
             if (board[0][j][0] == player && board[1][j][1] == player && board[2][j][2] == player) { winningLineStart=GetCellCenter(0,j,0); winningLineMid=GetCellCenter(1,j,1); winningLineEnd=GetCellCenter(2,j,2); drawWinningLine=true; return player; } // Main diag
             if (board[0][j][2] == player && board[1][j][1] == player && board[2][j][0] == player) { winningLineStart=GetCellCenter(0,j,2); winningLineMid=GetCellCenter(1,j,1); winningLineEnd=GetCellCenter(2,j,0); drawWinningLine=true; return player; } // Anti diag
        }

        // Check lines within each YZ plane (fixed column k)
        for (int k = 0; k < SIZE; k++) {
             // Check Diagonals
             if (board[0][0][k] == player && board[1][1][k] == player && board[2][2][k] == player) { winningLineStart=GetCellCenter(0,0,k); winningLineMid=GetCellCenter(1,1,k); winningLineEnd=GetCellCenter(2,2,k); drawWinningLine=true; return player; } // Main diag
             if (board[0][2][k] == player && board[1][1][k] == player && board[2][0][k] == player) { winningLineStart=GetCellCenter(0,2,k); winningLineMid=GetCellCenter(1,1,k); winningLineEnd=GetCellCenter(2,0,k); drawWinningLine=true; return player; } // Anti diag
        }


        // Check columns across layers (vertical lines) - Some redundancy, but clear
        for (int j = 0; j < SIZE; j++) { // Row
            for (int k = 0; k < SIZE; k++) { // Column
                if (board[0][j][k] == player && board[1][j][k] == player && board[2][j][k] == player) {
                    winningLineStart = GetCellCenter(0, j, k);
                    winningLineMid = GetCellCenter(1, j, k);
                    winningLineEnd = GetCellCenter(2, j, k);
                    drawWinningLine = true;
                    return player;
                }
            }
        }

        // Check 3D diagonals (4 main space diagonals)
        if (board[0][0][0] == player && board[1][1][1] == player && board[2][2][2] == player) { winningLineStart=GetCellCenter(0,0,0); winningLineMid=GetCellCenter(1,1,1); winningLineEnd=GetCellCenter(2,2,2); drawWinningLine=true; return player; }
        if (board[0][0][2] == player && board[1][1][1] == player && board[2][2][0] == player) { winningLineStart=GetCellCenter(0,0,2); winningLineMid=GetCellCenter(1,1,1); winningLineEnd=GetCellCenter(2,2,0); drawWinningLine=true; return player; }
        if (board[0][2][0] == player && board[1][1][1] == player && board[2][0][2] == player) { winningLineStart=GetCellCenter(0,2,0); winningLineMid=GetCellCenter(1,1,1); winningLineEnd=GetCellCenter(2,0,2); drawWinningLine=true; return player; }
        if (board[0][2][2] == player && board[1][1][1] == player && board[2][0][0] == player) { winningLineStart=GetCellCenter(0,2,2); winningLineMid=GetCellCenter(1,1,1); winningLineEnd=GetCellCenter(2,0,0); drawWinningLine=true; return player; }

    } // End player loop p

    return EMPTY; // No winner found
}

// --- AI Logic ---

// Evaluate the board state for the Minimax algorithm
int EvaluateBoard(char board[SIZE][SIZE][SIZE]) {
    char winner = CheckWinner(board); // Use the comprehensive CheckWinner

    if (winner == AI_SYMBOL) {
        return WIN_SCORE; // AI wins immediately
    } else if (winner == USER_SYMBOL) {
        return LOSS_SCORE; // User wins immediately
    } else {
        // No immediate winner, evaluate based on the new heuristic
        int aiScore = CalculateTotalHeuristic(board, AI_SYMBOL);
        int userScore = CalculateTotalHeuristic(board, USER_SYMBOL);

        // Return the difference in heuristic scores
        return aiScore - userScore;
    }
}

// Minimax algorithm
int Minimax(char board[SIZE][SIZE][SIZE], int depth, bool isMaximizing, int maxDepth) {
    // Check for immediate win/loss/draw first
    char immediateWinner = CheckWinner(board); 
    if (immediateWinner == AI_SYMBOL) return WIN_SCORE - depth; // Prioritize faster wins
    if (immediateWinner == USER_SYMBOL) return LOSS_SCORE + depth; // Prioritize blocking faster losses
    if (IsBoardFull(board)) return DRAW_SCORE; // Draw
    if (depth == maxDepth) return EvaluateBoard(board); // Evaluate heuristic at max depth

    if (isMaximizing) { // AI's turn (Maximizer)
        int bestScore = INT_MIN;
        for (int i = 0; i < SIZE; i++) {
            for (int j = 0; j < SIZE; j++) {
                for (int k = 0; k < SIZE; k++) {
                    if (board[i][j][k] == EMPTY) {
                        board[i][j][k] = AI_SYMBOL;
                        int score = Minimax(board, depth + 1, false, maxDepth);
                        board[i][j][k] = EMPTY; // Undo move
                        bestScore = (score > bestScore) ? score : bestScore;
                         // Alpha-beta pruning could be added here
                    }
                }
            }
        }
        return bestScore;
    } else { // User's turn (Minimizer)
        int bestScore = INT_MAX;
        for (int i = 0; i < SIZE; i++) {
            for (int j = 0; j < SIZE; j++) {
                for (int k = 0; k < SIZE; k++) {
                    if (board[i][j][k] == EMPTY) {
                        board[i][j][k] = USER_SYMBOL;
                        int score = Minimax(board, depth + 1, true, maxDepth);
                        board[i][j][k] = EMPTY; // Undo move
                        bestScore = (score < bestScore) ? score : bestScore;
                         // Alpha-beta pruning could be added here
                    }
                }
            }
        }
        return bestScore;
    }
}

void GetAIMove(char board[SIZE][SIZE][SIZE], int ply, int *moveCount, int *bestL, int *bestR, int *bestC) {
    int bestScore = INT_MIN;
    *bestL = -1; *bestR = -1; *bestC = -1;
    typedef struct { int l, r, c; } Move;
    Move bestMoves[SIZE*SIZE*SIZE];
    int numBestMoves = 0;

    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            for (int k = 0; k < SIZE; k++) {
                if (board[i][j][k] == EMPTY) {
                    board[i][j][k] = AI_SYMBOL;
                    int moveScore = Minimax(board, 0, false, ply);
                    board[i][j][k] = EMPTY;

                    if (moveScore > bestScore) {
                        bestScore = moveScore;
                        numBestMoves = 0;
                        bestMoves[numBestMoves++] = (Move){i, j, k};
                    } else if (moveScore == bestScore) {
                         bestMoves[numBestMoves++] = (Move){i, j, k};
                    }
                }
            }
        }
    }

    if (numBestMoves > 0) {
        int randomIndex = rand() % numBestMoves;
        *bestL = bestMoves[randomIndex].l;
        *bestR = bestMoves[randomIndex].r;
        *bestC = bestMoves[randomIndex].c;
    }
}


// --- Raylib Drawing Functions ---

/*void DrawBoard3D(char board[SIZE][SIZE][SIZE], Camera camera, int hoverL, int hoverR, int hoverC) {
    timeCounter += GetFrameTime();
    
    // Draw gradient background
    ClearBackground(BACKGROUND_COLOR);
    
    // Draw stars in background
    for(int i = 0; i < 200; i++) {
        DrawCubeV((Vector3){
            (float)GetRandomValue(-50, 50),
            (float)GetRandomValue(-50, 50),
            (float)GetRandomValue(-50, 50)
        }, (Vector3){0.05f, 0.05f, 0.05f}, WHITE);
    }
    
    // Draw glowing grid base
    float gridSize = SIZE * (CELL_SIZE_3D + GRID_SPACING_3D);
    DrawGrid((int)gridSize, 1.0f);
    
    // Draw layers with different colors
    for(int l = 0; l < SIZE; l++) {
        for(int r = 0; r < SIZE; r++) {
            for(int c = 0; c < SIZE; c++) {
                Vector3 cellCenter = GetCellCenter(l, r, c);
                bool isHovered = (l == hoverL && r == hoverR && c == hoverC);
                
                // Draw cell container with glowing effect
                if(isHovered && board[l][r][c] == EMPTY) {
                    DrawCubeWiresV(cellCenter, (Vector3){CELL_SIZE_3D, CELL_SIZE_3D, CELL_SIZE_3D}, 
                                  ColorAlpha(HOVER_COLOR, 0.5f + sinf(timeCounter*8)*0.2f));
                    DrawCubeV(cellCenter, (Vector3){CELL_SIZE_3D, CELL_SIZE_3D, CELL_SIZE_3D},
                             ColorAlpha(HOVER_COLOR, 0.1f));
                }
                
                // Draw markers with animations
                if(board[l][r][c] != EMPTY) {
                    float scale = 1.0f + sinf(timeCounter*4 + l + r + c)*0.05f;
                    Color markerColor = (board[l][r][c] == USER_SYMBOL) ? X_COLOR : O_COLOR;
                    
                    if(board[l][r][c] == 'X') {
                        // Draw X with 3D model
                        DrawModel(xModel, cellCenter, MARKER_CUBE_SIZE*scale, markerColor);
                    } else {
                        // Draw O with 3D model
                        DrawModel(oModel, cellCenter, MARKER_CUBE_SIZE*scale, markerColor);
                    }
                    
                    // Add particle glow
                    DrawSphereEx(cellCenter, MARKER_RADIUS*1.2f, 8, 8, 
                                 ColorAlpha(markerColor, 0.2f));
                }
            }
        }
    }
    
    // Enhanced winning line
    if(drawWinningLine) {
        float thickness = 0.2f + sinf(timeCounter*8)*0.1f;
        DrawCylinderEx(winningLineStart, winningLineMid, thickness, thickness, 16, WIN_COLOR);
        DrawCylinderEx(winningLineMid, winningLineEnd, thickness, thickness, 16, WIN_COLOR);
        
        // Add sparkles along the line
        Vector3 dir = Vector3Normalize(Vector3Subtract(winningLineEnd, winningLineStart));
        float lineLength = Vector3Distance(winningLineStart, winningLineEnd);
        for(float t = 0; t < lineLength; t += 0.5f) {
            Vector3 pos = Vector3Add(winningLineStart, Vector3Scale(dir, t));
            DrawSphere(pos, 0.1f + sinf(timeCounter*12 + t)*0.05f, ColorAlpha(GOLD, 0.8f));
        }
    }
}*/

void DrawBoard3D(char board[SIZE][SIZE][SIZE], Camera camera, int hoverL, int hoverR, int hoverC) {
    // Draw a ground grid
    //DrawGrid(12, 1.0f); 

    ClearBackground(BACKGROUND_COLOR);

    // Draw stars in background
    for(int i = 0; i < 200; i++) {
        DrawCubeV((Vector3){
            (float)GetRandomValue(-50, 50),
            (float)GetRandomValue(-50, 50),
            (float)GetRandomValue(-50, 50)
        }, (Vector3){0.05f, 0.05f, 0.05f}, WHITE);
    }

    // Draw glowing grid base
    float gridSize = SIZE * (CELL_SIZE_3D + GRID_SPACING_3D);
    //DrawGrid((int)gridSize, 1.0f);

    // Calculate grid boundaries for drawing outer lines
    float totalGridDim = SIZE * CELL_SIZE_3D + (SIZE - 1) * GRID_SPACING_3D;
    float halfGrid = totalGridDim / 2.0f;
    Vector3 minBound = { -halfGrid, -halfGrid, -halfGrid };
    Vector3 maxBound = { halfGrid, halfGrid, halfGrid };

    // Draw the outer bounding box of the 3x3x3 grid
    DrawBoundingBox((BoundingBox){minBound, maxBound}, DARKGRAY);


    // Draw individual cell outlines and markers
    for (int l = 0; l < SIZE; l++) {
        for (int r = 0; r < SIZE; r++) {
            for (int c = 0; c < SIZE; c++) {
                Vector3 cellCenter = GetCellCenter(l, r, c);
                Color cellColor = LIGHTGRAY;
                bool isHovered = (l == hoverL && r == hoverR && c == hoverC);

                // Draw the cell wireframe (cube)
                if (isHovered && board[l][r][c] == EMPTY) {
                    cellColor = YELLOW;
                    DrawCubeWiresV(cellCenter, (Vector3){CELL_SIZE_3D, CELL_SIZE_3D, CELL_SIZE_3D}, cellColor);
                    DrawCubeV(cellCenter, (Vector3){CELL_SIZE_3D, CELL_SIZE_3D, CELL_SIZE_3D}, Fade(YELLOW, 0.2f)); // Slightly stronger hover fill
                } else {
                    DrawCubeWiresV(cellCenter, (Vector3){CELL_SIZE_3D, CELL_SIZE_3D, CELL_SIZE_3D}, cellColor);
                }

                // Draw the player marker (X or O) if the cell is not empty
                if (board[l][r][c] != EMPTY) {
                    Color symbolColor = (board[l][r][c] == USER_SYMBOL) ? BLUE : RED;
                    if (board[l][r][c] == 'X') {
                        // Draw X using basic DrawCubeV with transformations
                        float length = MARKER_CUBE_SIZE * 1.4f; // Make bars slightly longer to intersect nicely
                        float thick = MARKER_CUBE_SIZE * 0.15f;
                        Vector3 size = { length, thick, thick };

                        rlPushMatrix(); // Save current matrix state
                            rlTranslatef(cellCenter.x, cellCenter.y, cellCenter.z); // Move to cell center
                            rlRotatef(45.0f, 0.0f, 0.0f, 1.0f); // Rotate for first bar
                            DrawCubeV((Vector3){0,0,0}, size, symbolColor); // Draw centered at origin
                            DrawCubeWiresV((Vector3){0,0,0}, size, ColorAlpha(symbolColor, 0.7f));
                        rlPopMatrix(); // Restore matrix state

                        rlPushMatrix(); // Save current matrix state again
                            rlTranslatef(cellCenter.x, cellCenter.y, cellCenter.z); // Move to cell center
                            rlRotatef(-45.0f, 0.0f, 0.0f, 1.0f); // Rotate for second bar
                            DrawCubeV((Vector3){0,0,0}, size, symbolColor); // Draw centered at origin
                            DrawCubeWiresV((Vector3){0,0,0}, size, ColorAlpha(symbolColor, 0.7f));
                        rlPopMatrix(); // Restore matrix state

                    } else { // Symbol is 'O'
                        DrawSphere(cellCenter, MARKER_RADIUS, symbolColor);
                        DrawSphereWires(cellCenter, MARKER_RADIUS, 16, 16, ColorAlpha(symbolColor, 0.5f)); // Increased segments for smoother sphere wires
                    }
                }
            }
        }
    }
    // Draw the winning line if applicable
    if(drawWinningLine) {
        float thickness = 0.2f + sinf(timeCounter*8)*0.1f;
        DrawCylinderEx(winningLineStart, winningLineMid, thickness, thickness, 16, WIN_COLOR);
        DrawCylinderEx(winningLineMid, winningLineEnd, thickness, thickness, 16, WIN_COLOR);
        
        // Add sparkles along the line
        Vector3 dir = Vector3Normalize(Vector3Subtract(winningLineEnd, winningLineStart));
        float lineLength = Vector3Distance(winningLineStart, winningLineEnd);
        for(float t = 0; t < lineLength; t += 0.5f) {
            Vector3 pos = Vector3Add(winningLineStart, Vector3Scale(dir, t));
            DrawSphere(pos, 0.1f + sinf(timeCounter*12 + t)*0.05f, ColorAlpha(GOLD, 0.8f));
        }
    }
}

// Update UI drawing function
void DrawUI(GameScreen currentScreen, char winner, Font font, int difficulty, char selectedSymbol) {
    // Draw semi-transparent panel
    DrawRectangle(10, 10, 400, 100, ColorAlpha(BLACK, 0.7f));
    
    int fontSize = 20;
    float spacing = 1.5f;
    Vector2 pos = {20, 20};
    
    switch(currentScreen) {
        case SELECT_SYMBOL:
            DrawTextEx(font, "CHOOSE YOUR SYMBOL", pos, fontSize*1.5, spacing, WHITE);
            pos.y += 40;
            DrawTextEx(font, TextFormat("[X]    [O] "), pos, fontSize, spacing, WHITE);
            break;
            
        case SELECT_DIFFICULTY:
            DrawTextEx(font, "SELECT DIFFICULTY", pos, fontSize*1.5, spacing, WHITE);
            pos.y += 40;
            DrawTextEx(font, TextFormat("[1] Beginner  [2] Medium"), pos, fontSize, spacing, WHITE);
            pos.y += 30;
            DrawTextEx(font, TextFormat("[3] Hard      [4] Expert"), pos, fontSize, spacing, WHITE);
            break;
            
        case GAME_OVER: {
            const char* resultText = (winner == USER_SYMBOL) ? "VICTORY!" : 
                                    (winner == AI_SYMBOL) ? "DEFEAT!" : "DRAW!";
            Color resultColor = (winner == USER_SYMBOL) ? X_COLOR : 
                               (winner == AI_SYMBOL) ? O_COLOR : GRAY;
            
            // Animated text
            float scale = 1.0f + sinf(timeCounter*4)*0.1f;
            Vector2 textSize = MeasureTextEx(font, resultText, fontSize*3, spacing);
            Vector2 textPos = {SCREEN_WIDTH/2 - textSize.x/2, SCREEN_HEIGHT/2 - textSize.y/2};
            
            DrawTextEx(font, resultText, textPos, fontSize*3, spacing, 
                      ColorAlpha(resultColor, 0.8f + sinf(timeCounter*8)*0.2f));
            DrawTextEx(font, "PRESS [R] TO RESTART", (Vector2){SCREEN_WIDTH/2 - 140, textPos.y + 80}, 
                      fontSize, spacing, WHITE);
        } break;
            
        default:
            DrawTextEx(font, TextFormat("DIFFICULTY: %d", difficulty), pos, fontSize, spacing, WHITE);
            pos.y += 30;
            DrawTextEx(font, TextFormat("TURN: %c", (currentScreen == PLAYER_TURN) ? USER_SYMBOL : AI_SYMBOL), 
                      pos, fontSize, spacing, WHITE);
            break;
    }
}